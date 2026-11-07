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

#include <QAbstractListModel>

#include <zxkit/debug/Breakpoints.h>

#include <unordered_map>
#include <unordered_set>
#include <vector>

class BreakpointsModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("use breakpointsModel context property instead")
    enum {
        Id = Qt::UserRole + 1,
        Type,
        Value,
        Ignore,
        Life,
        Condition,
        Commands,
        AbsoluteAddress,
        Enabled,
        Hits
    };

public:
    enum BreakpointType {
        BreakOnExecute = int(zxkit::BreakpointType::Execute),
        BreakOnRead = int(zxkit::BreakpointType::MemoryRead),
        BreakOnWrite = int(zxkit::BreakpointType::MemoryWrite),
        BreakOnPortRead = int(zxkit::BreakpointType::PortRead),
        BreakOnPortWrite = int(zxkit::BreakpointType::PortWrite),
        BreakOnNextReg = int(zxkit::BreakpointType::NextRegWrite),
        BreakOnTime = int(zxkit::BreakpointType::RasterLine),
        BreakOnEvent = int(zxkit::BreakpointType::Event)
    };
    Q_ENUM(BreakpointType)

    enum BreakpointLife { Permanent = 0, Oneshot };
    Q_ENUM(BreakpointLife)

private:
    struct DebuggerBreakpoint
    {
        int id = 0;
        BreakpointType type = BreakOnExecute;
        uint16_t address = 0;
        uint16_t mask = 0xFFFF;
        int page = -1;
        QString bankName;
        bool enabled = true;
        quint64 hits = 0;
        size_t ignore = 0;
        BreakpointLife life = Permanent;
        QByteArray condition;
        QByteArray commands;
    };

public:
    using Addresses = std::unordered_map<uint16_t, std::unordered_set<int>>;

public:
    explicit BreakpointsModel(QObject *parent = nullptr);
    void breakpointsUpdated();

    const Addresses &addresses() const
    {
        return m_addresses;
    }

    // QAbstractItemModel interface
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

private:
    QVariant breakPointValue(const DebuggerBreakpoint &bp) const;

private:
    std::vector<DebuggerBreakpoint> m_breakPoints;
    Addresses m_addresses;
};
