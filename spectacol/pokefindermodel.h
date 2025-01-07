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

#include "fuselistmodel.h"

#include <vector>
#include <mutex>

class PokeFinderModel : public FuseListModel
{
    enum {
        Page = Qt::UserRole + 1,
        Offset,
        OffsetText,
        Value,
        Breakpoint
    };

public:
    explicit PokeFinderModel(QObject *parent = nullptr);
    void update();

protected:
    // QAbstractItemModel interface
    int rowCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QHash<int, QByteArray> roleNames() const;

private:
    enum {
        NoBreakpoint = 0,
        OnRead = 1,
        OnWrite = 2
    };
    struct PokeFinderData {
        PokeFinderData(uint16_t bank, uint16_t offset, uint8_t value, uint8_t breakpoints = NoBreakpoint)
            : bank(bank), offset(offset), value(value), breakpoints(breakpoints) {}
        uint16_t bank;
        uint16_t offset;
        uint8_t value;
        uint8_t breakpoints;
    };

    mutable std::mutex m_mutex;
    std::vector<PokeFinderData> m_data, m_dataTmp;
};
