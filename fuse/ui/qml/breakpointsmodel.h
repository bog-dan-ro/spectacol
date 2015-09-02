#ifndef BEAKPOINTSMODEL_H
#define BEAKPOINTSMODEL_H

#include "fuselistmodel.h"

#include <vector>
#include <mutex>
#include <unordered_map>

#include <debugger/breakpoint.h>

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
        Commands
    };

public:
    enum BreakpointType {
        Execute = DEBUGGER_BREAKPOINT_TYPE_EXECUTE,
        Read,
        Write,
        PortRead,
        PortWrite,
        Time,
        Event
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
    typedef std::unordered_map<u_int32_t, std::pair<debugger_breakpoint_type, debugger_breakpoint_address>> Addresses;

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
