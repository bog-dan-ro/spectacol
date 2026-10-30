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

}

PokeModel::PokeModel()
{
    QPointer<PokeModel> self = this;
    pokeEvent([self]{
        pokemem_autoload_pokfile();
        publishTrainers(self);
    });
}

int PokeModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return m_trainers.size();
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

void PokeModel::publishTrainers(const QPointer<PokeModel> &self)
{
    auto trainers = std::make_shared<Trainers>();
    for (GSList *item = trainer_list; item; item = item->next) {
        auto trainer = reinterpret_cast<trainer_t*>(item->data);
        trainers->push_back({QLatin1String(trainer->name), bool(trainer->active), bool(trainer->disabled)});
    }
    FuseEmulator::instance().callFunction([self, trainers]{
        if (!self)
            return;
        self->beginResetModel();
        self->m_trainers = std::move(*trainers);
        self->endResetModel();
    });
}

void PokeModel::update()
{
    QPointer<PokeModel> self = this;
    pokeEvent([self]{
        publishTrainers(self);
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

    QPointer<PokeModel> self = this;
    pokeEvent([self, bank, address, value]{
        auto trainer = pokemem_trainer_list_add(bank, address, value);
        if (!trainer->disabled)
            pokemem_trainer_activate(trainer);
        publishTrainers(self);
    });
}

void PokeModel::setActive(int row, bool active)
{
    QPointer<PokeModel> self = this;
    pokeEvent([self, row, active] {
        auto trainer = getTrainer(row);
        if (!trainer)
            return;
        if (active)
            pokemem_trainer_activate(trainer);
        else
            pokemem_trainer_deactivate(trainer);
        // FUSE refuses to activate a disabled trainer, report what it did
        const bool isActive = trainer->active;
        FuseEmulator::instance().callFunction([self, row, isActive] {
            if (!self || size_t(row) >= self->m_trainers.size())
                return;
            self->m_trainers[row].active = isActive;
            emit self->dataChanged(self->index(row), self->index(row), {Active});
        });
    });
}

void PokeModel::clear()
{
    QPointer<PokeModel> self = this;
    pokeEvent([self]{
        pokemem_clear();
        publishTrainers(self);
    });
}
