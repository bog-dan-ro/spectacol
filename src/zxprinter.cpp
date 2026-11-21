/*
    Copyright (c) 2015-2026, BogDan Vatra <bogdan@kde.org>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "zxemulator.h"

#include <zxkit/machines/SinclairMachines.h>
#include <zxkit/peripherals/expansion/ZxPrinter.h>

#include <QBuffer>
#include <QDateTime>
#include <QDir>
#include <QFontDatabase>
#include <QFontMetricsF>
#include <QImage>
#include <QPageLayout>
#include <QPainter>
#include <QPdfWriter>
#include <QSettings>
#include <QUrl>

#include <algorithm>
#include <cstring>
#include <span>
#include <vector>

using namespace Qt::StringLiterals;
using namespace std::literals;

namespace {

constexpr int printerPdfResolution = 254; // 10 pixels/mm.
constexpr int printWidth = 920; // 92 mm of printable width.
constexpr int dotsPerRow = zxkit::ZxPrinterPaper::kWidth;
constexpr int glyphSize = zxkit::ZxPrinterPaper::kGlyphSize;

QPageLayout printerPageLayout()
{
    return { QPageSize(QPageSize::A4), QPageLayout::Portrait, QMarginsF(12, 12, 12, 12),
             QPageLayout::Millimeter };
}

struct PrinterPage
{
    std::size_t firstRow;
    std::span<const zxkit::ZxPrinterPaper::Row> rows;
    std::span<const zxkit::ZxPrinterPaper::TextLine> textLines;
};

std::vector<PrinterPage> printerPages(const zxkit::ZxPrinterPaper &paper)
{
    const auto viewport = printerPageLayout().paintRectPixels(printerPdfResolution);
    const auto rowsPerPage = viewport.height() * dotsPerRow / printWidth;
    std::vector<PrinterPage> pages;
    if (rowsPerPage < glyphSize)
        return pages;

    const auto paperRows = paper.rows();
    auto textLines = paper.textLines();
    for (std::size_t firstRow = 0; firstRow < paperRows.size();) {
        auto endRow = std::min(firstRow + rowsPerPage, paperRows.size());
        auto endText = std::ranges::lower_bound(textLines, endRow, {},
                                                &zxkit::ZxPrinterPaper::TextLine::row);
        if (endText != textLines.begin() && (endText - 1)->row + glyphSize > endRow)
            endRow = (--endText)->row;
        const auto textCount = std::size_t(endText - textLines.begin());
        pages.push_back({ firstRow, paperRows.subspan(firstRow, endRow - firstRow),
                          textLines.first(textCount) });
        textLines = textLines.subspan(textCount);
        firstRow = endRow;
    }
    return pages;
}

QImage printerPageImage(std::span<const zxkit::ZxPrinterPaper::Row> rows)
{
    QImage image(dotsPerRow, int(rows.size()), QImage::Format_Mono);
    if (image.isNull())
        return {};
    image.setColorTable({ qRgb(255, 255, 255), qRgb(0, 0, 0) });
    for (int y = 0; y < image.height(); ++y)
        std::memcpy(image.scanLine(y), rows[y].data(), rows[y].size());
    return image;
}

QByteArray printerPdf(const zxkit::ZxPrinterPaper &paper)
{
    QByteArray bytes;
    QBuffer buffer(&bytes);
    if (!buffer.open(QIODevice::WriteOnly))
        return {};

    QPdfWriter writer(&buffer);
    writer.setTitle(ZxEmulator::tr("ZX Printer"));
    writer.setResolution(printerPdfResolution);
    writer.setPageLayout(printerPageLayout());
    QPainter painter;
    if (!painter.begin(&writer))
        return {};
    painter.setRenderHint(QPainter::LosslessImageRendering);

    const qreal dotScale = qreal(printWidth) / dotsPerRow;
    const auto viewport = painter.viewport();
    const auto left = (viewport.width() - printWidth) / 2;
    const auto pages = printerPages(paper);
    if (pages.empty())
        return {};

    QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    font.setPixelSize(glyphSize);
    const QFontMetricsF metrics(font, &writer);
    const qreal cellAdvance = metrics.horizontalAdvance(u'M');
    // A Spectrum space is a whole square cell. Shorter word spaces in the text
    // layer let PDF readers distinguish words from separate columns.
    font.setWordSpacing(-metrics.horizontalAdvance(u' ') / 2);
    painter.setFont(font);
    painter.setPen(Qt::black);
    const qreal horizontalScale = glyphSize * dotScale / cellAdvance;
    const qreal verticalScale = (glyphSize - 1) * dotScale / (metrics.ascent() + metrics.descent());

    for (const auto &page : pages) {
        if (page.firstRow != 0 && !writer.newPage())
            return {};
        const auto image = printerPageImage(page.rows);
        if (image.isNull())
            return {};

        painter.drawImage(QRectF(left, 0, printWidth, image.height() * dotScale), image);

        // Invisible text provides search and selection at the original glyph positions.
        painter.save();
        painter.setOpacity(0);
        painter.translate(left, 0);
        painter.scale(horizontalScale, verticalScale);
        for (const auto &line : page.textLines) {
            QString text;
            for (const char16_t character : line.characters)
                text += character == 0 ? u' ' : character;
            const qreal y =
                    (line.row - page.firstRow + 0.5) * dotScale / verticalScale + metrics.ascent();
            for (qsizetype column = 0; column < text.size();) {
                if (text[column] == u' ') {
                    ++column;
                    continue;
                }
                // Anchor each group of words on its first printed cell. Longer
                // gaps can contain graphics or separate columns.
                auto end = text.indexOf(u"  "_s, column);
                if (end < 0)
                    end = text.size() - (text.endsWith(u' ') ? 1 : 0);
                painter.drawText(QPointF(column * cellAdvance, y),
                                 text.sliced(column, end - column));
                column = end;
            }
        }
        painter.restore();
    }
    if (!painter.end())
        return {};
    return bytes;
}

} // namespace

QVariantMap ZxEmulator::zxPrinterStatus() const
{
    const auto core = emulator().readLock();
    const auto &machine = core->machine();
    const bool hasEditorPrint = !machine.is48kMode()
            && (dynamic_cast<const zxkit::Spectrum128kMachine *>(&machine)
                || dynamic_cast<const zxkit::SpectrumPlus3Machine *>(&machine));
    return { { u"available"_s, zxkit::ZxPrinterPeripheral::availableOn(machine) },
             { u"hasEditorPrint"_s, hasEditorPrint },
             { u"enabled"_s, m_zxPrinterEnabled },
             { u"rows"_s, m_zxPrinterPaper ? int(m_zxPrinterPaper->rows().size()) : 0 },
             { u"hasText"_s, m_zxPrinterPaper && !m_zxPrinterPaper->text().empty() },
             { u"truncated"_s, m_zxPrinterPaper && m_zxPrinterPaper->truncated() } };
}

QVariantMap ZxEmulator::zxPrinterPreviewPage(int index) const
{
    QImage image;
    int pageCount;
    {
        const auto core = emulator().readLock();
        if (!m_zxPrinterPaper)
            return {};
        const auto pages = printerPages(*m_zxPrinterPaper);
        pageCount = int(pages.size());
        if (index < 0 || index >= pageCount)
            return {};
        image = printerPageImage(pages[index].rows);
    }

    QByteArray png;
    QBuffer buffer(&png);
    if (!buffer.open(QIODevice::WriteOnly) || !image.save(&buffer, "PNG"))
        return { { u"pageCount"_s, pageCount } };
    return { { u"pageCount"_s, pageCount },
             { u"sourceSize"_s, image.size() },
             { u"source"_s, u"data:image/png;base64,"_s + QString::fromLatin1(png.toBase64()) } };
}

bool ZxEmulator::setZxPrinterEnabled(bool enabled)
{
    if (m_zxPrinterEnabled == enabled)
        return true;
    if (!withEmulator([&](zxkit::Emulator &core) {
            if (core.rzx().isRecording() || core.rzx().isReviewing() || core.rzx().isPlaying())
                throw zxkit::PeripheralException(
                        "Stop the RZX session before changing the ZX Printer connection"sv);
            if (enabled) {
                if (!m_zxPrinterPaper)
                    m_zxPrinterPaper = std::make_shared<zxkit::ZxPrinterPaper>();
                core.attachPeripheral(
                        std::make_unique<zxkit::ZxPrinterPeripheral>(m_zxPrinterPaper));
            } else {
                core.detachPeripheral(zxkit::ZxPrinterPeripheral::kId);
            }
        }))
        return false;

    m_zxPrinterEnabled = enabled;
    QSettings().setValue(u"Peripherals/zxPrinter"_s, enabled);
    return true;
}

void ZxEmulator::clearZxPrinterPaper()
{
    auto core = emulator().writeLock();
    if (auto device = core->findPeripheral(zxkit::ZxPrinterPeripheral::kId))
        static_cast<zxkit::ZxPrinterPeripheral &>(device->get()).clearPaper();
    else if (m_zxPrinterPaper)
        m_zxPrinterPaper->clear();
}

QString ZxEmulator::zxPrinterExportPath(const QString &format) const
{
    const QString date = QDateTime::currentDateTime().toString(u"yyyyMMdd-HHmmss-zzz"_s);
    return QDir(dataPath()).filePath(u"Printouts/ZX Printer "_s + date + u'.' + format);
}

bool ZxEmulator::exportZxPrinter(const QString &filePath, const QString &format)
{
    zxkit::ZxPrinterPaper paper;
    {
        const auto core = emulator().readLock();
        if (m_zxPrinterPaper)
            paper = *m_zxPrinterPaper;
    }
    if (paper.rows().empty() || (format == u"txt"_s && paper.text().empty())) {
        showMessage(tr("There is no ZX Printer output for this format."), Error);
        return false;
    }

    const auto write = [&](std::span<const uint8_t> bytes) {
        if (bytes.empty()) {
            showMessage(tr("Could not encode the ZX Printer output."), Error);
            return false;
        }
        const QUrl url(filePath);
        QString path = url.isLocalFile() ? url.toLocalFile() : filePath;
        const QString suffix = u'.' + format;
        if (!path.startsWith(u"content://"_s) && !path.endsWith(suffix, Qt::CaseInsensitive))
            path += suffix;
        if (!writeFileBytes(path, bytes))
            return false;
        showMessage(tr("Printout saved to '%1'.").arg(path));
        return true;
    };

    if (format == u"pbm"_s)
        return write(paper.toPbm());
    if (format == u"png"_s)
        return write(paper.toPng());
    if (format == u"txt"_s)
        return write(
                { reinterpret_cast<const uint8_t *>(paper.text().data()), paper.text().size() });
    if (format == u"pdf"_s) {
        const auto bytes = printerPdf(paper);
        return write({ reinterpret_cast<const uint8_t *>(bytes.constData()),
                       std::size_t(bytes.size()) });
    }
    showMessage(tr("Unsupported ZX Printer export format."), Error);
    return false;
}
