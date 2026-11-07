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

#include "pokefindermodel.h"

#include "breakpointsmodel.h"
#include "zxemulator.h"
#include "qmlui.h"

using namespace Qt::StringLiterals;

#define MAX_POSSIBLE 20

PokeFinderModel::PokeFinderModel(QObject *parent) : QAbstractListModel(parent) { }

void PokeFinderModel::update()
{
    std::vector<zxkit::PokeCandidate> candidates;
    {
        const auto emulator = ZxEmulator::instance().emulator().readLock();
        const auto &pf = emulator->debugger().pokeFinder();
        if (pf.count() > 0 && pf.count() <= MAX_POSSIBLE)
            candidates = pf.results(MAX_POSSIBLE);
    }

    std::vector<PokeFinderData> data;
    const auto &addresses = ZxEmulator::instance().breakpointsModel()->addresses();
    for (const auto &cand : candidates) {
        uint8_t breakpoints = NoBreakpoint;
        const uint16_t absAddr = uint16_t(0x4000 + cand.offset);
        if (auto it = addresses.find(absAddr); it != addresses.end()) {
            if (it->second.contains(BreakpointsModel::BreakOnRead))
                breakpoints = OnRead;
            if (it->second.contains(BreakpointsModel::BreakOnWrite))
                breakpoints |= OnWrite;
        }
        data.emplace_back(
                PokeFinderData(cand.page16k, cand.offset, cand.currentValue, breakpoints));
    }

    const bool sameSize = data.size() == m_data.size();
    if (!sameSize)
        beginResetModel();
    m_data = std::move(data);
    if (!sameSize)
        endResetModel();
    else if (!m_data.empty())
        emit dataChanged(index(0), index(int(m_data.size()) - 1));
}

int PokeFinderModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_data.size();
}

QVariant PokeFinderModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || size_t(index.row()) >= m_data.size())
        return {};

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
            return u"Break on Read"_s;
        case 2:
            return u"Break on Write"_s;
        case 3:
            return u"Break on R/W"_s;
        default:
            return u""_s;
        }
        break;
    }

    return {};
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
