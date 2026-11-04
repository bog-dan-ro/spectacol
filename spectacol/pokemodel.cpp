/*
    Copyright (c) 2015-2025, BogDan Vatra <bogdan@kde.org>

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

#include "pokemodel.h"

#include "qmlui.h"
#include "zxemulator.h"

using namespace Qt::StringLiterals;

PokeModel::PokeModel() = default;

int PokeModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return int(m_trainers.size());
}

QVariant PokeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.column() != 0 || size_t(index.row()) >= m_trainers.size())
        return {};
    const auto &trainer = m_trainers[index.row()];
    switch (role) {
    case Label:
        return trainer.name;
    case Active:
        return trainer.active;
    case Disabled:
        return trainer.disabled;
    default:
        return {};
    }
}

QHash<int, QByteArray> PokeModel::roleNames() const
{
    static QHash<int, QByteArray> roles = { { Label, "label" }, { Active, "active" }, { Disabled, "disabled" } };
    return roles;
}

void PokeModel::update()
{
    beginResetModel();
    endResetModel();
}

namespace {
// Bank 0 denotes the CPU's effective write mapping; positive IDs are physical MMU banks.
QString writePoke(zxkit::Emulator &core, int bankId, int address, uint8_t value, uint8_t *previous = nullptr)
{
    if (address < 0)
        return QObject::tr("Invalid address");
    auto &mmu = core.mmu();
    if (!bankId) {
        if (address > 65535)
            return QObject::tr("Address is outside CPU memory");
        const auto &mapping = mmu.effectiveMapping(unsigned(address) / 1024);
        if (!mapping.writable || !mapping.writeBankId)
            return QObject::tr("This CPU address is read-only or unmapped");
        // Save the byte from the write bank, which can differ from the visible read bank.
        if (previous)
            *previous = mmu.bank(mapping.writeBankId).page1k(mapping.writePage1k)[address & 1023];
        mmu.poke(uint16_t(address), value);
    } else {
        if (bankId < 0 || std::size_t(bankId) > mmu.banks().size())
            return QObject::tr("Memory banks changed; choose a bank again");
        auto &bank = mmu.bank(zxkit::BankId(bankId));
        if (!bank.isWritable())
            return QObject::tr("The selected bank is read-only");
        if (std::size_t(address) >= bank.size())
            return QObject::tr("Address is outside the selected bank");
        auto &byte = bank.page1k(unsigned(address) / 1024)[address & 1023];
        if (previous)
            *previous = byte;
        byte = value;
    }
    return {};
}
}

void PokeModel::refreshBanks()
{
    QStringList names { tr("CPU address space") };
    std::vector<Bank> banks { { 0, 65535 } };
    {
        const auto core = ZxEmulator::instance().emulator().readLock();
        for (const auto &bank : core->mmu().banks()) {
            if (!bank.isWritable() || !bank.size())
                continue;
            names.append(QString::fromUtf8(bank.name().data(), qsizetype(bank.name().size())) + u" (%1 KiB)"_s.arg(bank.size() / 1024));
            banks.push_back({ int(bank.id()), int(bank.size() - 1) });
        }
    }
    m_banks = std::move(banks);
    m_bankNames = std::move(names);
    m_selectedBank = 0;
    emit banksChanged();
    emit selectedBankChanged();
}
void PokeModel::setSelectedBank(int index)
{
    if (index < 0 || std::size_t(index) >= m_banks.size() || index == m_selectedBank)
        return;
    m_selectedBank = index;
    emit selectedBankChanged();
}
int PokeModel::addressLimit() const
{
    return m_selectedBank >= 0 && std::size_t(m_selectedBank) < m_banks.size() ? m_banks[m_selectedBank].maximum : 0;
}
bool PokeModel::addPoke(int bank, int address, int value)
{
    auto &host = ZxEmulator::instance();
    if (bank < 0 || std::size_t(bank) >= m_banks.size() || value < 0 || value > 255) {
        host.showMessage(tr("Choose a writable bank and a byte from 0 to 255"), ZxEmulator::Error);
        return false;
    }
    Trainer trainer;
    trainer.bank = m_banks[bank].id;
    trainer.address = address;
    trainer.value = uint8_t(value);
    QString error;
    {
        auto core = host.emulator().writeLock();
        trainer.machine = core->machine().id();
        error = writePoke(*core, trainer.bank, address, trainer.value, &trainer.original);
    }
    if (!error.isEmpty()) {
        host.showMessage(error, ZxEmulator::Error);
        return false;
    }
    trainer.name = u"%1 : $%2 = $%3"_s.arg(m_bankNames[bank]).arg(QString::number(address, 16).toUpper()).arg(QString::number(value, 16).rightJustified(2, u'0').toUpper());
    trainer.active = true;
    const int row = int(m_trainers.size());
    beginInsertRows({}, row, row);
    m_trainers.push_back(std::move(trainer));
    endInsertRows();
    host.debuggerRefresh();
    return true;
}
void PokeModel::setActive(int row, bool active)
{
    if (row < 0 || std::size_t(row) >= m_trainers.size())
        return;
    auto &trainer = m_trainers[row];
    if (trainer.active == active || trainer.disabled)
        return;
    auto &host = ZxEmulator::instance();
    QString error;
    {
        auto core = host.emulator().writeLock();
        if (core->machine().id() != trainer.machine)
            error = tr("This poke belongs to another machine");
        else
            error = writePoke(*core, trainer.bank, trainer.address, active ? trainer.value : trainer.original);
    }
    if (!error.isEmpty()) {
        trainer.disabled = true;
        host.showMessage(error, ZxEmulator::Error);
    } else
        trainer.active = active;
    emit dataChanged(index(row), index(row), { Active, Disabled });
    host.debuggerRefresh();
}

void PokeModel::clear()
{
    beginResetModel();
    m_trainers.clear();
    endResetModel();
}
