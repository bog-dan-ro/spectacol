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

using namespace Qt::StringLiterals;
using namespace std::literals;

namespace {

QByteArray printerPdf(const zxkit::ZxPrinterPaper &paper)
{
    QByteArray bytes;
    QBuffer buffer(&bytes);
    if (!buffer.open(QIODevice::WriteOnly))
        return {};

    QPdfWriter writer(&buffer);
    writer.setTitle(ZxEmulator::tr("ZX Printer"));
    writer.setResolution(254); // 10 dots/mm
    writer.setPageLayout(QPageLayout(QPageSize(QPageSize::A4), QPageLayout::Portrait,
                                     QMarginsF(12, 12, 12, 12), QPageLayout::Millimeter));
    QPainter painter;
    if (!painter.begin(&writer))
        return {};
    painter.setRenderHint(QPainter::LosslessImageRendering);

    constexpr int paperWidth = 920; // 92 mm of printable width on the ZX Printer.
    constexpr int dotsPerRow = zxkit::ZxPrinterPaper::kWidth;
    constexpr int glyphSize = zxkit::ZxPrinterPaper::kGlyphSize;
    const qreal dotScale = qreal(paperWidth) / dotsPerRow;
    const auto viewport = painter.viewport();
    const auto left = (viewport.width() - paperWidth) / 2;
    const auto rowsPerPage = viewport.height() * dotsPerRow / paperWidth;
    if (rowsPerPage < glyphSize)
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

    const auto paperRows = paper.rows();
    auto textLines = paper.textLines();
    for (std::size_t firstRow = 0; firstRow < paperRows.size();) {
        auto endRow = std::min(firstRow + rowsPerPage, paperRows.size());
        auto endText = std::ranges::lower_bound(textLines, endRow, {},
                                                &zxkit::ZxPrinterPaper::TextLine::row);
        if (endText != textLines.begin() && (endText - 1)->row + glyphSize > endRow)
            endRow = (--endText)->row;
        const auto pageRows = int(endRow - firstRow);
        QImage image(dotsPerRow, pageRows, QImage::Format_Mono);
        if (image.isNull())
            return {};
        image.setColorTable({ qRgb(255, 255, 255), qRgb(0, 0, 0) });
        for (int y = 0; y < pageRows; ++y)
            std::memcpy(image.scanLine(y), paperRows[firstRow + y].data(),
                        paperRows[firstRow + y].size());

        painter.drawImage(QRectF(left, 0, paperWidth, pageRows * dotScale), image);

        // Invisible text provides search and selection at the original glyph positions.
        painter.save();
        painter.setOpacity(0);
        painter.translate(left, 0);
        painter.scale(horizontalScale, verticalScale);
        for (auto line = textLines.begin(); line != endText; ++line) {
            QString text;
            for (const char16_t character : line->characters)
                text += character == 0 ? u' ' : character;
            const qreal y =
                    (line->row - firstRow + 0.5) * dotScale / verticalScale + metrics.ascent();
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

        textLines = textLines.subspan(endText - textLines.begin());
        firstRow = endRow;
        if (firstRow < paperRows.size() && !writer.newPage())
            return {};
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
