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

#include <QCoreApplication>

#include <algorithm>

#include "breakpointsmodel.h"
#include "zxemulator.h"
#include "qmlui.h"

using namespace Qt::StringLiterals;

#define MAX_POSSIBLE 20

PokeFinderModel::PokeFinderModel(QObject *parent) : QAbstractListModel(parent) { }

zxkit::Breakpoint PokeFinderModel::locationBreakpoint(int page16k, int offset,
                                                      zxkit::BreakpointType type,
                                                      zxkit::BankId ramBank)
{
    zxkit::Breakpoint bp;
    bp.type = type;
    bp.page1k = uint16_t(page16k * 16 + ((offset & 0x3FFF) >> 10));
    bp.bankId = ramBank;
    bp.address = uint16_t(offset & 1023);
    bp.mask = 1023;
    return bp;
}

bool PokeFinderModel::sameLocation(const zxkit::Breakpoint &a, const zxkit::Breakpoint &b)
{
    return a.type == b.type && a.address == b.address && a.mask == b.mask && a.page1k == b.page1k
            && a.bankId == b.bankId && !a.condition && !b.condition;
}

void PokeFinderModel::update()
{
    std::vector<PokeFinderData> data;
    {
        const auto emulator = ZxEmulator::instance().emulator().readLock();
        const auto &pf = emulator->debugger().pokeFinder();
        if (pf.count() > 0 && pf.count() <= MAX_POSSIBLE) {
            const auto ramBank = emulator->machine().ramBankId();
            const auto &breakpoints = emulator->debugger().breakpoints().breakpoints();
            const auto has = [&](const zxkit::PokeCandidate &cand, zxkit::BreakpointType type) {
                const auto wanted = locationBreakpoint(cand.page16k, cand.offset, type, ramBank);
                return std::ranges::any_of(breakpoints, [&](const zxkit::Breakpoint &bp) {
                    return bp.enabled && sameLocation(bp, wanted);
                });
            };
            for (const auto &cand : pf.results(MAX_POSSIBLE)) {
                uint8_t flags = NoBreakpoint;
                if (has(cand, zxkit::BreakpointType::MemoryRead))
                    flags |= OnRead;
                if (has(cand, zxkit::BreakpointType::MemoryWrite))
                    flags |= OnWrite;
                data.emplace_back(cand.page16k, cand.offset, cand.currentValue, flags);
            }
        }
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
        case OnRead:
            return QCoreApplication::translate("PokeFinderModel", "On read");
        case OnWrite:
            return QCoreApplication::translate("PokeFinderModel", "On write");
        case OnRead | OnWrite:
            return QCoreApplication::translate("PokeFinderModel", "On read and write");
        default:
            return QString();
        }
        break;

    case BreakFlags:
        return int(m_data[index.row()].breakpoints);
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
    roles[BreakFlags] = "breakFlags";
    return roles;
}
