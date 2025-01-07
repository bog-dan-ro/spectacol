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


#include "fuseemulator.h"
#include "qmlui.h"

extern "C"  {
# include <pokefinder/pokemem.h>
}

namespace {

struct trainer_t *getTrainer(int index)
{
    auto item = trainer_list;
    for (;item && index; --index)
        item = item->next;

    if (index || !item)
        return nullptr;

    return reinterpret_cast<trainer_t*>(item->data);
}

int trainers()
{
    int count = 0;
    GSList *item = trainer_list;
    for (;item; ++count)
        item = item->next;
    return count;
}

}

PokeModel::PokeModel()
{
    pokeEvent([this]{
        pokemem_autoload_pokfile();
        update();
    });
}

int PokeModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return trainers();
}

QVariant PokeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0)
        return {};
    auto trainer = getTrainer(index.row());
    if (!trainer)
        return {};
    switch (role) {
    case Label:
            return QLatin1String(trainer->name);
    case Active:
            return bool(trainer->active);
    case Disabled:
            return bool(trainer->disabled);
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
    callFunction([this]{
        beginResetModel();
        endResetModel();
    });
}

void PokeModel::addPoke(int bank, int address, int value)
{
    if (bank < 0 || bank > 64) {
        // Pentagon 1024 has 65 memory pages
        FuseEmulator::instance().showMessage(tr("Invalid bank: use an integer from 0 to 64"), FuseEmulator::Error);
        return;
    }

    if (address < 0 || address > 0xffff) {
        emit FuseEmulator::instance().showMessage(tr("Invalid address: use an integer from 0 to 65535"), FuseEmulator::Error);
        return;
    }

    if (bank == 8 && address < 0x4000) {
        FuseEmulator::instance().showMessage(tr("Invalid address: use an integer from 16384 to 65535"), FuseEmulator::Error);
        return;
    }

    pokeEvent([this, bank, address, value]{
        auto trainer = pokemem_trainer_list_add(bank, address, value);
        if (!trainer->disabled)
            pokemem_trainer_activate(trainer);
        callFunction([this] {
            beginResetModel();
            endResetModel();
        });
    });
}

void PokeModel::setActive(int row, bool active)
{
    pokeEvent([this, row, active] {
        if (auto trainer = getTrainer(row)) {
            if (active)
                pokemem_trainer_activate(trainer);
            else
                pokemem_trainer_deactivate(trainer);
            callFunction([this, row] {
                emit dataChanged(index(row), index(row));
            });
        }
    });
}

void PokeModel::clear()
{
    pokeEvent([this]{
        pokemem_clear();
        callFunction([this] {
            beginResetModel();
            endResetModel();
        });
    });
}
