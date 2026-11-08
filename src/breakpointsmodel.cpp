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

#include "breakpointsmodel.h"
#include "zxemulator.h"
#include "qmlui.h"

using namespace Qt::StringLiterals;

BreakpointsModel::BreakpointsModel(QObject *parent) : QAbstractListModel(parent)
{
    qRegisterMetaType<BreakpointType>("BreakpointType");
}

void BreakpointsModel::breakpointsUpdated()
{
    std::vector<DebuggerBreakpoint> breakpoints;
    Addresses addresses;
    {
        const auto emulator = ZxEmulator::instance().emulator().readLock();
        for (const auto &bp : emulator->debugger().breakpoints().breakpoints()) {
            DebuggerBreakpoint dbp;
            dbp.id = int(bp.id);
            dbp.type = BreakpointType(bp.type);
            dbp.address = bp.address;
            dbp.mask = bp.mask;
            dbp.page = bp.page1k.has_value() ? int(*bp.page1k) : -1;
            dbp.enabled = bp.enabled;
            dbp.hits = bp.hitCount;
            if (bp.bankId && *bp.bankId <= emulator->mmu().banks().size()) {
                const auto name = emulator->mmu().bank(*bp.bankId).name();
                dbp.bankName = QString::fromUtf8(name.data(), qsizetype(name.size()));
            }
            dbp.life = bp.temporary ? Oneshot : Permanent;
            breakpoints.push_back(dbp);

            switch (dbp.type) {
            case BreakOnExecute:
            case BreakOnRead:
            case BreakOnWrite:
                if (bp.enabled && bp.mask == 0xFFFF && !bp.page1k && !bp.bankId)
                    addresses[bp.address].insert(int(dbp.type));
                break;
            default:
                break;
            }
        }
    }

    beginResetModel();
    m_breakPoints = std::move(breakpoints);
    m_addresses = std::move(addresses);
    endResetModel();
}

int BreakpointsModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_breakPoints.size());
}

QVariant breakpointType(BreakpointsModel::BreakpointType type)
{
    switch (type) {
    case BreakpointsModel::BreakOnExecute:
        return u"X"_s;
    case BreakpointsModel::BreakOnRead:
        return u"R"_s;
    case BreakpointsModel::BreakOnWrite:
        return u"W"_s;
    case BreakpointsModel::BreakOnPortRead:
        return u"PR"_s;
    case BreakpointsModel::BreakOnPortWrite:
        return u"PW"_s;
    case BreakpointsModel::BreakOnNextReg:
        return u"NR"_s;
    case BreakpointsModel::BreakOnTime:
        return u"T"_s;
    case BreakpointsModel::BreakOnEvent:
        return u"E"_s;
    }
    return {};
}

QVariant BreakpointsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.column() != 0 || index.row() < 0
        || size_t(index.row()) >= m_breakPoints.size())
        return {};

    const auto &value = m_breakPoints[index.row()];
    switch (role) {
    case Qt::DisplayRole:
    case Id:
        return value.id;
    case Type:
        return breakpointType(value.type);
    case Value:
        return breakPointValue(value);
    case Ignore:
        return QString::number(value.ignore);
    case Life:
        return (value.life == Permanent) ? QString::fromUtf8("∞") : QString::fromUtf8("1");
    case Condition:
        return value.condition;
    case Commands:
        return value.commands;
    case Enabled:
        return value.enabled;
    case Hits:
        return QVariant::fromValue(value.hits);
    case AbsoluteAddress:
        return int(value.address);
    }
    return {};
}

QHash<int, QByteArray> BreakpointsModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[Id] = "breakpointId";
    roles[Type] = "type";
    roles[Value] = "value";
    roles[Ignore] = "ignore";
    roles[Life] = "life";
    roles[Condition] = "condition";
    roles[Commands] = "commands";
    roles[AbsoluteAddress] = "absoluteAddress";
    roles[Enabled] = "active";
    roles[Hits] = "hits";
    return roles;
}

QVariant BreakpointsModel::breakPointValue(const BreakpointsModel::DebuggerBreakpoint &bp) const
{
    switch (bp.type) {
    case BreakOnExecute:
    case BreakOnRead:
    case BreakOnWrite:
        if (bp.page < 0)
            return formatNumber(bp.address)
                    + (bp.mask == 0xFFFF ? QString{} : tr(" / mask %1").arg(formatNumber(bp.mask)));
        return bp.bankName + u':'
                + QString::number(bp.page * 1024 + (bp.address & 1023), 16)
                          .rightJustified(6, u'0')
                          .toUpper();

    case BreakOnPortRead:
    case BreakOnPortWrite:
        return formatNumber(bp.mask) + u':' + formatNumber(bp.address) + u':';

    case BreakOnNextReg:
        return formatNumber(bp.address);
    case BreakOnTime:
        return tr("Line %1").arg(bp.address);
    case BreakOnEvent:
        return bp.address ? tr("NMI") : tr("IRQ");
    }
    return {};
}
