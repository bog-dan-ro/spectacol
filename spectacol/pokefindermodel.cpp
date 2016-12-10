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

#include "pokefindermodel.h"

#include "breakpointsmodel.h"
#include "fuseemulator.h"
#include "qmlui.h"

extern "C"  {
# include <memory_pages.h>
# include <pokefinder/pokefinder.h>
}

#define MAX_POSSIBLE 20

PokeFinderModel::PokeFinderModel(QObject *parent)
    : FuseListModel(parent)
{
}

void PokeFinderModel::update()
{
    {
        BreakpointsModel *breakpointsModel = g_fuseEmulator->breakpointsModel();
        std::lock_guard<std::mutex> lock(m_mutex);
        std::lock_guard<std::mutex> lockBreakpoints(breakpointsModel->breakpointsMutex());
        m_dataTmp.clear();
        const auto &addresses = breakpointsModel->addresses();
        if (pokefinder_count && pokefinder_count <= MAX_POSSIBLE) {
            for (size_t page = 0; page < MEMORY_PAGES_IN_16K * SPECTRUM_RAM_PAGES; page++ ) {
                memory_page *mapping = &memory_map_ram[page];
                const uint16_t bank = mapping->page_num;

                for (size_t offset = 0; offset < MEMORY_PAGE_SIZE; offset++) {
                    if( ! (pokefinder_impossible[page][offset/8] & 1 << (offset & 7)) ) {
                        const uint16_t bank_offset = mapping->offset + offset;
                        const uint8_t value = mapping->page[offset];
                        debugger_breakpoint_address addr;
                        addr.source = memory_source_ram;
                        addr.page = bank;
                        addr.offset = bank_offset;
                        auto it = addresses.find(addr);
                        uint8_t breakpoints = NoBreakpoint;
                        if (it != addresses.end()) {
                            if (it->second.find(DEBUGGER_BREAKPOINT_TYPE_READ) != it->second.end())
                                breakpoints = OnRead;

                            if (it->second.find(DEBUGGER_BREAKPOINT_TYPE_WRITE) != it->second.end())
                                breakpoints |= OnWrite;
                        }
                        m_dataTmp.emplace_back(PokeFinderData(bank, bank_offset, value, breakpoints));
                    }
                }
            }
        }
    }
    callFunction([this]{
        bool changed = m_dataTmp.size() == m_data.size();
        if (!changed)
            beginResetModel();
        std::lock_guard<std::mutex> lock(m_mutex);
        m_data = std::move(m_dataTmp);
        m_dataTmp.clear();
        if (!changed)
            endResetModel();
        else
            emit dataChanged(index(0), index(m_data.size()-1));
    });
}

int PokeFinderModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_data.size();
}

QVariant PokeFinderModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || size_t(index.row()) >= m_data.size())
        return QVariant();

    switch (role) {
    case Page:
        return m_data[index.row()].bank;

    case Offset:
        return m_data[index.row()].offset;

    case OffsetText:
        return formatNumber(m_data[index.row()].offset);

    case Value:
        return formatNumber(m_data[index.row()].value);

    case Breakpoint:
        switch (m_data[index.row()].breakpoints) {
        case 1:
            return QLatin1Literal("Break on Read");
        case 2:
            return QLatin1Literal("Break on Write");
        case 3:
            return QLatin1Literal("Break on R/W");
        default:
            return QLatin1Literal("");
        }
        break;
    }

    return QVariant();
}

QHash<int, QByteArray> PokeFinderModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[Page] = "page";
    roles[Offset] = "offset";
    roles[OffsetText] = "offsetText";
    roles[Value] = "value";
    roles[Breakpoint] = "breakpoint";
    return roles;
}
