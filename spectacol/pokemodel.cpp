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

#include "zxemulator.h"
#include "qmlui.h"

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
    if (!index.isValid() || size_t(index.row()) >= m_trainers.size())
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
    static QHash<int, QByteArray> roles = {
        {Label, "label"},
        {Active, "active"},
        {Disabled, "disabled"}
    };
    return roles;
}

void PokeModel::update()
{
    beginResetModel();
    endResetModel();
}

void PokeModel::addPoke(int bank, int address, int value)
{
    if (bank < 0 || bank > 64) {
        ZxEmulator::instance().showMessage(tr("Invalid bank: use an integer from 0 to 64"), ZxEmulator::Error);
        return;
    }

    if (address < 0 || address > 0xffff) {
        emit ZxEmulator::instance().showMessage(tr("Invalid address: use an integer from 0 to 65535"), ZxEmulator::Error);
        return;
    }

    if (bank == 8 && address < 0x4000) {
        ZxEmulator::instance().showMessage(tr("Invalid address: use an integer from 16384 to 65535"), ZxEmulator::Error);
        return;
    }

    Trainer trn;
    trn.name = u"Custom (%1, %2)"_s.arg(address).arg(value);
    trn.active = true;
    beginResetModel();
    m_trainers.push_back(trn);
    endResetModel();

    ZxEmulator::instance().pokeMemory(address, bank, value);
}

void PokeModel::setActive(int row, bool active)
{
    if (row < 0 || size_t(row) >= m_trainers.size())
        return;
    m_trainers[row].active = active;
    emit dataChanged(index(row), index(row), {Active});
}

void PokeModel::clear()
{
    beginResetModel();
    m_trainers.clear();
    endResetModel();
}
