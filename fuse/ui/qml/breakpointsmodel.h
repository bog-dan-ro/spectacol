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

#ifndef BEAKPOINTSMODEL_H
#define BEAKPOINTSMODEL_H

#include "fuselistmodel.h"

#include <vector>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

#include <debugger/breakpoint.h>

namespace std
{
    template<>
    struct hash<debugger_breakpoint_address>
    {
        typedef debugger_breakpoint_address argument_type;
        typedef std::size_t result_type;

        result_type operator()(argument_type const& s) const
        {
            return s.source << 24 | s.page << 16 | s.offset;
        }
    };

    template<>
    struct equal_to<debugger_breakpoint_address>
    {
        typedef debugger_breakpoint_address argument_type;
        bool
        operator()(const argument_type &a, const argument_type &b) const
        {
            return a.source == b.source && a.page == b.page && a.offset == b.offset;
        }
    };
}

class BreakpointsModel : public FuseListModel
{
    Q_OBJECT
    Q_ENUMS(BreakpointType BreakpointLife)
    enum {
        Id = Qt::UserRole + 1,
        Type,
        Value,
        Ignore,
        Life,
        Condition,
        Commands,
        AbsoluteAddress
    };

public:
    enum BreakpointType {
        BreakOnExecute = DEBUGGER_BREAKPOINT_TYPE_EXECUTE,
        BreakOnRead = DEBUGGER_BREAKPOINT_TYPE_READ,
        BreakOnWrite = DEBUGGER_BREAKPOINT_TYPE_WRITE,
        BreakOnPortRead = DEBUGGER_BREAKPOINT_TYPE_PORT_READ,
        BreakOnPortWrite = DEBUGGER_BREAKPOINT_TYPE_PORT_WRITE,
        BreakOnTime = DEBUGGER_BREAKPOINT_TYPE_TIME,
        BreakOnEvent = DEBUGGER_BREAKPOINT_TYPE_EVENT
    };

    enum BreakpointLife {
        Permanent = DEBUGGER_BREAKPOINT_LIFE_PERMANENT,
        Oneshot
    };

private:
    struct DebuggerEvent {
        explicit DebuggerEvent(const debugger_event_t &event)
        {
            type = event.type;
            detail = event.detail;
        }
        QByteArray type;
        QByteArray detail;
    };

    struct DebuggerBreakpoint {
        explicit DebuggerBreakpoint(debugger_breakpoint *bp);
        ~DebuggerBreakpoint();

        int id;
        BreakpointType type;
        union debugger_breakpoint_value {
          debugger_breakpoint_address address;
          debugger_breakpoint_port port;
          debugger_breakpoint_time time;
          DebuggerEvent *event;
        } value;
        size_t ignore;
        BreakpointLife life;
        QByteArray condition;
        QByteArray commands;
    };

public:
    typedef std::unordered_map<debugger_breakpoint_address, std::unordered_set<int>> Addresses;

public:
    BreakpointsModel(QObject *parent);
    void breakpointsUpdated();

    std::mutex &breakpointsMutex() const { return m_mutex; }
    const Addresses &addresses() const { return m_addresses; }

    // QAbstractItemModel interface
    int rowCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QHash<int, QByteArray> roleNames() const;

private:
    QVariant breakPointValue(const DebuggerBreakpoint &bp) const;

private:
    mutable std::mutex m_mutex;
    std::vector<DebuggerBreakpoint> m_breakPoints, m_breakPointsTmp;
    Addresses m_addresses;
};

#endif // BEAKPOINTSMODEL_H
