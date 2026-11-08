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

#include "debugsymbols.h"

#include <QFile>
#include <QRegularExpression>
#include <QUrl>
#include <algorithm>

using namespace Qt::StringLiterals;

int DebugSymbols::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_visible.size());
}
QHash<int, QByteArray> DebugSymbols::roleNames() const
{
    return {
        { Label, "label" }, { Address, "address" }, { Location, "location" }, { Page, "page" }
    };
}
QVariant DebugSymbols::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.column() || index.row() < 0 || index.row() >= rowCount())
        return {};
    const auto &label = m_labels[m_visible[index.row()]];
    switch (role) {
    case Label:
        return label.name;
    case Address:
        return label.address;
    case Page:
        return label.page;
    case Location:
        return u"$%1"_s.arg(label.address, 4, 16, u'0').toUpper()
                + (label.page < 0 ? QString{} : tr(" · page %1").arg(label.page));
    }
    return {};
}
void DebugSymbols::refilter()
{
    beginResetModel();
    m_visible.clear();
    for (int i = 0; i < m_labels.size(); ++i)
        if (m_labels[i].name.contains(m_search, Qt::CaseInsensitive))
            m_visible.append(i);
    endResetModel();
    emit changed();
}
void DebugSymbols::setSearch(const QString &text)
{
    if (text != m_search) {
        m_search = text;
        refilter();
    }
}
bool DebugSymbols::fail(const QString &error)
{
    m_error = error;
    emit changed();
    return false;
}
void DebugSymbols::clear()
{
    beginResetModel();
    m_path.clear();
    m_error.clear();
    m_search.clear();
    m_labels.clear();
    m_visible.clear();
    m_tables.clear();
    m_pageSize = 16384;
    m_pageCount = 0;
    endResetModel();
    emit changed();
}
bool DebugSymbols::load(const QString &path)
{
    const QUrl url(path);
    QFile file(url.isLocalFile() ? url.toLocalFile() : path);
    if (!file.open(QIODevice::ReadOnly))
        return fail(tr("Cannot open symbols: %1").arg(file.errorString()));
    if (file.size() > 16 * 1024 * 1024)
        return fail(tr("Symbol file exceeds 16 MiB"));
    const auto bytes = file.readAll();
    if (file.error() != QFileDevice::NoError)
        return fail(tr("Cannot read symbols"));
    auto text = QString::fromUtf8(bytes);
    if (text.startsWith(QChar(0xfeff)))
        text.remove(0, 1);
    const bool sld = text.contains(u'|');
    QList<LabelData> labels;
    QMap<int, zxkit::SymbolTable> tables;
    int pageSize = 16384, pageCount = 0, locations = 0, lineNumber = 0;
    const QRegularExpression symPattern(
            uR"(^([^\s:;]+)\s*:?\s+(?:EQU\s+|=\s*)?(\$[0-9a-f]+|0x[0-9a-f]+|[0-9a-f]+h|[0-9]+)\s*(?:;.*)?$)"_s,
            QRegularExpression::CaseInsensitiveOption);
    auto invalid = [&] {
        return fail(tr("Invalid symbol record at line %1; previous symbols kept").arg(lineNumber));
    };
    for (auto line : text.split(u'\n')) {
        ++lineNumber;
        line = line.trimmed();
        if (line.isEmpty() || line.startsWith(u';') || line.startsWith(u'#'))
            continue;
        if (!sld) {
            const auto match = symPattern.match(line);
            if (!match.hasMatch())
                return invalid();
            auto value = match.captured(2);
            int base = 10;
            if (value.startsWith(u'$')) {
                value.remove(0, 1);
                base = 16;
            } else if (value.startsWith(u"0x"_s, Qt::CaseInsensitive)) {
                value.remove(0, 2);
                base = 16;
            } else if (value.endsWith(u'h', Qt::CaseInsensitive)) {
                value.chop(1);
                base = 16;
            }
            bool ok = false;
            const auto address = value.toULongLong(&ok, base);
            if (!ok)
                return invalid();
            if (address > 65535)
                continue; // assembler constants are not addresses
            const auto name = match.captured(1);
            labels.append({ name, uint16_t(address), -1 });
            tables[-1].addSymbol(uint16_t(address), name.toStdString());
            continue;
        }
        if (line.startsWith(u"||"_s))
            continue;
        if (line.startsWith(u"|SLD.data.version|"_s)) {
            if (line != u"|SLD.data.version|1"_s)
                return fail(tr("Unsupported SLD version"));
            continue;
        }
        if (line.startsWith(u'|'))
            continue; // extensible control records
        const auto cols = line.split(u'|');
        if (cols.size() != 8 && cols.size() != 7)
            return invalid();
        const int shift = cols.size() == 8 ? 1 : 0;
        const auto type = cols[5 + shift];
        if (type == u"Z"_s) {
            const QRegularExpression memory(
                    uR"(^pages.size:(\d+),pages.count:(\d+),slots.count:(\d+)(?:,.*)?$)"_s);
            const auto match = memory.match(cols[6 + shift]);
            if (!match.hasMatch())
                return invalid();
            const int size = match.captured(1).toInt(), count = match.captured(2).toInt();
            if ((size != 8192 && size != 16384 && size != 65536) || count < 1 || count > 65536)
                return invalid();
            if ((!tables.isEmpty()) && (size != pageSize || count != pageCount))
                return fail(tr("Multiple SLD memory models in one file are unsupported"));
            pageSize = size;
            pageCount = count;
            continue;
        }
        if (type != u"T"_s && type != u"L"_s)
            continue;
        bool lineOk = false, pageOk = false, addressOk = false;
        const auto sourceLine = cols[1].section(u':', 0, 0).toULongLong(&lineOk);
        const int page = cols[3 + shift].toInt(&pageOk);
        const auto address = cols[4 + shift].toLongLong(&addressOk);
        if (!lineOk || sourceLine > UINT32_MAX || !pageOk || page < -1 || page > 65535
            || !addressOk)
            return invalid();
        if (type == u"L"_s && cols[6 + shift].split(u',').contains(u"+equ"_s))
            continue;
        if (address < 0 || address > 65535)
            return invalid();
        if (type == u"L"_s) {
            const auto fields = cols[6 + shift].split(u',').mid(0, 3);
            if (std::none_of(fields.cbegin(), fields.cend(), [](const QString &name) {
                    return !name.isEmpty() && !name.startsWith(u'+');
                }))
                return invalid();
        }
        auto &table = tables[page];
        const auto utf8 = line.toUtf8();
        table.loadSldText(std::string_view(utf8.constData(), std::size_t(utf8.size())));
        if (type == u"T"_s) {
            if (!sourceLine)
                return invalid();
            ++locations;
        } else {
            const auto symbol = table.findSymbol(uint16_t(address));
            if (!symbol || symbol->empty())
                return invalid();
            labels.append({ QString::fromUtf8(*symbol), uint16_t(address), page });
        }
    }
    if (labels.empty() && !locations)
        return fail(tr("No addresses or source mappings found"));
    std::sort(labels.begin(), labels.end(),
              [](const auto &a, const auto &b) { return a.name < b.name; });
    beginResetModel();
    m_labels = std::move(labels);
    m_tables = std::move(tables);
    m_pageSize = pageSize;
    m_pageCount = pageCount;
    m_path = file.fileName();
    m_error.clear();
    m_search.clear();
    m_visible.clear();
    for (int i = 0; i < m_labels.size(); ++i)
        m_visible.append(i);
    endResetModel();
    emit changed();
    return true;
}
bool DebugSymbols::mapped(const zxkit::Emulator &core, uint16_t address, int page) const
{
    if (page < 0)
        return true;
    if (m_pageCount == 4 && m_pageSize == 16384)
        return address / 16384 == page;
    const auto &mapping = core.mmu().effectiveMapping(address / 1024);
    if (mapping.bankId != core.machine().ramBankId())
        return false;
    return int(mapping.readPage1k * 1024 / m_pageSize) == page;
}
QString DebugSymbols::annotation(const zxkit::Emulator &core, uint16_t address) const
{
    QStringList result;
    for (auto it = m_tables.cbegin(); it != m_tables.cend(); ++it) {
        if (!mapped(core, address, it.key()))
            continue;
        if (const auto name = it->findSymbol(address))
            result.append(QString::fromUtf8(*name));
        if (const auto source = it->findSourceLocation(address))
            result.append(u"%1:%2"_s.arg(QString::fromStdString(source->file)).arg(source->line));
    }
    return result.join(u" · "_s);
}
int DebugSymbols::resolve(const zxkit::Emulator &core, const QString &label) const
{
    int address = -1;
    bool known = false;
    for (const auto &entry : m_labels) {
        if (entry.name != label)
            continue;
        known = true;
        if (!mapped(core, entry.address, entry.page))
            continue;
        if (address >= 0 && address != entry.address)
            return -2;
        address = entry.address;
    }
    return address < 0 && known ? -2 : address;
}
int DebugSymbols::rowAddress(const zxkit::Emulator &core, int row) const
{
    if (row < 0 || row >= m_visible.size())
        return -1;
    const auto &label = m_labels[m_visible[row]];
    return mapped(core, label.address, label.page) ? int(label.address) : -1;
}
