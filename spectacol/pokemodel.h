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

#pragma once

#include <QQmlEngine>

#include "fuselistmodel.h"

class PokeModel : public FuseListModel
{
    Q_OBJECT
    QML_ELEMENT

public:
    enum Roles {
        Label = Qt::DisplayRole,
        Active = Qt::UserRole + 1,
        Disabled
    };
    Q_ENUM(Roles)

public:
    PokeModel();

    // QAbstractItemModel interface
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

public slots:
    void update();
    void addPoke(int bank, int address, int value);
    void setActive(int index, bool active = true);
    void clear();

};
