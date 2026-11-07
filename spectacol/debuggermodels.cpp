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

#include "debuggermodels.h"
#include <QVariantList>
#include <algorithm>
#include <limits>
#include <zxkit/machines/ZxNextMachine.h>

using namespace Qt::StringLiterals;
namespace {
QString hex(quint32 value, int digits = 4)
{
    return u"$%1"_s.arg(value, digits, 16, u'0').toUpper();
}
QString byte(uint8_t value)
{
    return u"%1"_s.arg(value, 2, 16, u'0').toUpper();
}
} // namespace

DebuggerMemoryModel::DebuggerMemoryModel(const zxkit::Shared<zxkit::Emulator> &core,
                                         QObject *parent)
    : QAbstractListModel(parent), m_core(core)
{
    refresh();
}
int DebuggerMemoryModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : (m_count + m_columns - 1) / m_columns;
}
QHash<int, QByteArray> DebuggerMemoryModel::roleNames() const
{
    static const QHash<int, QByteArray> roles = { { Address, "address" },
                                                  { AddressText, "addressText" },
                                                  { Cells, "cells" },
                                                  { Ascii, "ascii" } };
    return roles;
}
QVariant DebuggerMemoryModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.column() != 0 || index.row() < 0 || index.row() >= rowCount())
        return {};
    const int start = index.row() * m_columns;
    const int count = (std::min)(m_columns, m_count - start);
    switch (role) {
    case Address:
        return m_base + start;
    case AddressText:
        return hex(m_base + start, m_bank < 0 ? 4 : 6);
    case Cells: {
        QVariantList cells;
        for (int i = 0; i < count; ++i)
            cells.append(byte(m_bytes[start + i]));
        return cells;
    }
    case Ascii: {
        QString text;
        for (int i = 0; i < count; ++i)
            text.append(m_bytes[start + i] >= 32 && m_bytes[start + i] < 127
                                ? QChar(m_bytes[start + i])
                                : u'.');
        return text;
    }
    }
    return {};
}
QString DebuggerMemoryModel::addressText() const
{
    return hex(address(), m_bank < 0 ? 4 : 6);
}
bool DebuggerMemoryModel::parseHex(const QString &text, quint32 limit, quint32 &value)
{
    auto trimmed = text.trimmed();
    if (trimmed.startsWith(u'$'))
        trimmed.remove(0, 1);
    else if (trimmed.startsWith(u"0x"_s, Qt::CaseInsensitive))
        trimmed.remove(0, 2);
    bool ok = false;
    const auto parsed = trimmed.toULongLong(&ok, 16);
    if (!ok || trimmed.startsWith(u'-') || parsed > limit)
        return false;
    value = quint32(parsed);
    return true;
}
bool DebuggerMemoryModel::fail(const QString &text)
{
    m_error = text;
    emit errorChanged();
    return false;
}
void DebuggerMemoryModel::refresh()
{
    QStringList banks = { tr("CPU address space") };
    std::array<uint8_t, 256> bytes{};
    bool writable = false;
    int bankId = m_bank, size = 0, base = 0, count = 0, cursor = 0;
    {
        const auto core = m_core.readLock();
        for (const auto &bank : core->mmu().banks())
            banks.append(QString::fromUtf8(bank.name().data(), qsizetype(bank.name().size()))
                         + u" (%1 KiB)"_s.arg(bank.size() / 1024));
        if (bankId <= 0 || bankId >= banks.size())
            bankId = -1;
        size = bankId < 0
                ? 65536
                : int(std::min<std::size_t>(core->mmu().bank(zxkit::BankId(bankId)).size(),
                                            (std::numeric_limits<int>::max)()));
        base = std::clamp(m_base, 0, (std::max)(0, size - 1)) & ~255;
        count = (std::min)(256, size - base);
        cursor = std::clamp(m_cursor, 0, (std::max)(0, count - 1));
        if (bankId < 0) {
            for (int i = 0; i < count; ++i)
                bytes[i] = core->mmu().peek(uint16_t(base + i));
            writable = core->mmu()
                               .effectiveMapping(unsigned(base + cursor) >> zxkit::Mmu::kPageShift)
                               .writable;
        } else {
            const auto &bank = core->mmu().bank(zxkit::BankId(bankId));
            for (int i = 0; i < count; ++i)
                bytes[i] = bank.page1k(unsigned(base + i) / 1024)[(base + i) & 1023];
            writable = bank.isWritable();
        }
    }
    beginResetModel();
    m_bank = bankId;
    m_size = size;
    m_base = base;
    m_count = count;
    m_cursor = cursor;
    m_bytes = bytes;
    m_banks = banks;
    m_writable = writable;
    endResetModel();
    emit banksChanged();
    emit cursorChanged();
}
void DebuggerMemoryModel::setBankId(int bank)
{
    if (bank == m_bank)
        return;
    if (bank != -1 && (bank <= 0 || bank >= m_banks.size())) {
        fail(tr("Unknown memory bank"));
        return;
    }
    m_bank = bank;
    m_base = m_cursor = 0;
    refresh();
}
void DebuggerMemoryModel::setColumns(int columns)
{
    if (columns != 4 && columns != 8 && columns != 16)
        return;
    if (m_columns == columns)
        return;
    beginResetModel();
    m_columns = columns;
    endResetModel();
    emit columnsChanged();
}
void DebuggerMemoryModel::setCursor(int cursor)
{
    if (cursor >= 0 && cursor < m_count)
        moveCursor(cursor - m_cursor);
}
void DebuggerMemoryModel::moveCursor(int delta)
{
    const int next =
            int(std::clamp<qint64>(qint64(address()) + delta, 0, (std::max)(0, m_size - 1)));
    const int base = next & ~255;
    m_cursor = next - base;
    m_base = base;
    refresh();
}
bool DebuggerMemoryModel::goTo(const QString &text)
{
    quint32 address;
    if (!parseHex(text, quint32((std::max)(0, m_size - 1)), address))
        return fail(tr("Address is outside the selected memory bank"));
    m_base = int(address) & ~255;
    m_cursor = int(address) & 255;
    refresh();
    m_error.clear();
    emit errorChanged();
    return true;
}
void DebuggerMemoryModel::setEditable(bool editable)
{
    if (m_editable == editable)
        return;
    m_editable = editable;
    emit editableChanged();
}
bool DebuggerMemoryModel::writeByte(const QString &text, bool allowRom)
{
    quint32 value;
    if (!m_editable)
        return fail(tr("Pause the debugger before editing memory"));
    if (!parseHex(text, 255, value))
        return fail(tr("Enter one hexadecimal byte (00–FF)"));
    QString error;
    {
        auto core = m_core.writeLock();
        if (m_bank < 0) {
            const auto &map =
                    core->mmu().effectiveMapping(unsigned(address()) >> zxkit::Mmu::kPageShift);
            if (!map.writable && (!allowRom || !map.bankId))
                error = tr("This address is read-only or unmapped");
            else
                core->mmu().poke(uint16_t(address()), uint8_t(value), allowRom);
        } else if (std::size_t(m_bank) > core->mmu().banks().size()) {
            error = tr("Memory bank changed; refresh the view");
        } else {
            auto &bank = core->mmu().bank(zxkit::BankId(m_bank));
            if (std::size_t(address()) >= bank.size())
                error = tr("Memory bank changed; refresh the view");
            else if (!bank.isWritable() && !allowRom)
                error = tr("Enable ROM editing to change this bank");
            else
                bank.page1k(unsigned(address()) / 1024)[address() & 1023] = uint8_t(value);
        }
    }
    if (!error.isEmpty())
        return fail(error);
    m_error.clear();
    emit errorChanged();
    refresh();
    return true;
}

DebuggerStateModel::DebuggerStateModel(const zxkit::Shared<zxkit::Emulator> &core, Kind kind,
                                       QObject *parent)
    : QAbstractListModel(parent), m_core(core), m_kind(kind)
{
    refresh();
}
int DebuggerStateModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_rows.size());
}
QHash<int, QByteArray> DebuggerStateModel::roleNames() const
{
    static const QHash<int, QByteArray> roles = { { Address, "address" },
                                                  { AddressText, "addressText" },
                                                  { ValueText, "valueText" },
                                                  { Details, "details" } };
    return roles;
}
QVariant DebuggerStateModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.column() != 0 || index.row() < 0 || index.row() >= rowCount())
        return {};
    const auto &row = m_rows[index.row()];
    switch (role) {
    case Address:
        return row.address;
    case AddressText:
        return row.location;
    case ValueText:
        return row.value;
    case Details:
        return row.details;
    }
    return {};
}
void DebuggerStateModel::refresh()
{
    std::vector<Row> rows;
    bool available = true;
    {
        const auto core = m_core.readLock();
        if (m_kind == Kind::NextRegisters) {
            const auto *next = dynamic_cast<const zxkit::ZxNextMachine *>(&core->machine());
            available = next != nullptr;
            if (next)
                for (int reg = 0; reg < 256; ++reg) {
                    QString description;
                    switch (reg) {
                    case 0:
                        description = tr("Machine ID");
                        break;
                    case 1:
                        description = tr("Core version");
                        break;
                    case 2:
                        description = tr("Reset");
                        break;
                    case 3:
                        description = tr("Machine type");
                        break;
                    case 7:
                        description = tr("CPU speed");
                        break;
                    case 0x0E:
                        description = tr("Core sub-version");
                        break;
                    default:
                        if (reg >= 0x50 && reg <= 0x57)
                            description = tr("MMU slot %1 (8 KiB page)").arg(reg - 0x50);
                        break;
                    }
                    rows.push_back({ reg, hex(reg, 2), hex(next->nextReg().peek(uint8_t(reg)), 2),
                                     description });
                }
        } else {
            for (unsigned slot = 0; slot < zxkit::Mmu::kSlotCount; ++slot) {
                const auto &map = core->mmu().effectiveMapping(slot);
                const auto name = [&](zxkit::BankId id) {
                    if (!id)
                        return tr("Unmapped");
                    const auto text = core->mmu().bank(id).name();
                    return QString::fromUtf8(text.data(), qsizetype(text.size()));
                };
                rows.push_back(
                        { int(slot * 1024), hex(slot * 1024) + u"–"_s + hex(slot * 1024 + 1023),
                          name(map.bankId) + u" : "_s + hex(quint32(map.readPage1k) * 1024, 6),
                          (map.writable ? tr("Write %1 : %2")
                                                  .arg(name(map.writeBankId),
                                                       hex(quint32(map.writePage1k) * 1024, 6))
                                        : tr("Read-only"))
                                  + (map.contended ? tr(" · contended") : QString{}) });
            }
        }
    }
    beginResetModel();
    m_rows = std::move(rows);
    m_available = available;
    endResetModel();
    emit availableChanged();
}
