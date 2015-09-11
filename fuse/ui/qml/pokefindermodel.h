/*
    Copyright (c) 2015, BogDan Vatra <bogdan@kde.org>

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

#ifndef POKEFINDERMODEL_H
#define POKEFINDERMODEL_H

#include "fuselistmodel.h"

#include <vector>
#include <mutex>

class PokeFinderModel : public FuseListModel
{
    enum {
        Bank = Qt::UserRole + 1,
        Offset,
        OffsetText,
        Value
    };

public:
    PokeFinderModel(QObject *parent);
    void update();

protected:
    // QAbstractItemModel interface
    int rowCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QHash<int, QByteArray> roleNames() const;

private:
    struct PokeFinderData {
        PokeFinderData(uint16_t bank, uint16_t offset, uint8_t value)
            : bank(bank), offset(offset), value(value) {}
        uint16_t bank;
        uint16_t offset;
        uint8_t value;
    };

    mutable std::mutex m_mutex;
    std::vector<PokeFinderData> m_data, m_dataTmp;
};

#endif // POKEFINDERMODEL_H
