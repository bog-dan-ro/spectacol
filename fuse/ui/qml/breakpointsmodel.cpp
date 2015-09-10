#include "breakpointsmodel.h"
#include "qmlui.h"

#include <set>

static inline uint32_t absoluteAddress(uint16_t page, uint16_t address)
{
    uint32_t absAdd = 0;
    switch (page) {
    case 5:
        absAdd = 0x4000;
        break;
    case 2:
        absAdd = 0x8000;
    default:
        absAdd = 0xc000;
        break;
    }
    return absAdd + address;
}

BreakpointsModel::BreakpointsModel(QObject *parent)
    : FuseListModel(parent)
{
    qRegisterMetaType<BreakpointType>("BreakpointType");
}

void BreakpointsModel::breakpointsUpdated()
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_breakPointsTmp.clear();
        for (GSList *ptr = debugger_breakpoints; ptr; ptr = ptr->next ) {
            debugger_breakpoint *bp = reinterpret_cast<debugger_breakpoint *>(ptr->data);
            m_breakPointsTmp.emplace_back(DebuggerBreakpoint(bp));
            switch (bp->type) {
            case DEBUGGER_BREAKPOINT_TYPE_EXECUTE:
            case DEBUGGER_BREAKPOINT_TYPE_READ:
            case DEBUGGER_BREAKPOINT_TYPE_WRITE:
                if (bp->value.address.source == memory_source_any ||
                    bp->value.address.source == memory_source_rom)
                        m_addresses[bp->value.address.offset] = {bp->type, bp->value.address};
                else if (bp->value.address.source == memory_source_ram)
                    m_addresses[absoluteAddress(bp->value.address.page, bp->value.address.offset)] = {bp->type, bp->value.address};
                    break;
                break;
            default:
                break;
            }
        }
    }

    callFunction([this](){
        beginResetModel();
        std::lock_guard<std::mutex> lock(m_mutex);
        m_breakPoints = std::move(m_breakPointsTmp);
        m_breakPointsTmp.clear();
        endResetModel();
    });
}

int BreakpointsModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_breakPoints.size();
}

QVariant BreakpointsModel::data(const QModelIndex &index, int role) const
{
    if (size_t(index.row()) >= m_breakPoints.size())
        return QVariant();

    const auto &value = m_breakPoints[index.row()];
    switch (role) {
    case Qt::DisplayRole:
    case Id:
        return value.id;
    case Type:
        return value.type;
    case Value:
        return breakPointValue(value);
    case Ignore:
        return QString::number(value.ignore);
    case Life:
        return value.life;
    case Condition:
        return value.condition;
    case Commands:
        return value.commands;
    }
    return QVariant();
}

QHash<int, QByteArray> BreakpointsModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[Id] = "Id";
    roles[Type] = "Type";
    roles[Value] = "Value";
    roles[Ignore] = "Ignore";
    roles[Life] = "Life";
    roles[Condition] = "Condition";
    roles[Commands] = "Commands";
    return roles;
}

QVariant BreakpointsModel::breakPointValue(const BreakpointsModel::DebuggerBreakpoint &bp) const
{
    switch (bp.type) {
    case Execute:
    case Read:
    case Write:
        if (bp.value.address.source== memory_source_any)
            return formatNumber(bp.value.address.offset);
        return QLatin1String(memory_source_description(bp.value.address.source)) + QLatin1Char(':') +
                formatNumber(bp.value.address.page) + QLatin1Char(':') +
                formatNumber(bp.value.address.offset) + QLatin1Char(':');

    case PortRead:
    case PortWrite:
        return  formatNumber(bp.value.port.mask) + QLatin1Char(':') +
                formatNumber(bp.value.port.port) + QLatin1Char(':');

    case Time:
        return QString::number(bp.value.time.tstates);

    case Event:
        return bp.value.event->type + ":" + bp.value.event->detail;
    }
    return QVariant();
}


BreakpointsModel::DebuggerBreakpoint::DebuggerBreakpoint(debugger_breakpoint *bp)
{
    id = bp->id;
    type = BreakpointType(bp->type);
    switch (type) {
    case Execute:
    case Read:
    case Write:
        value.address = bp->value.address;
        break;

    case PortRead:
    case PortWrite:
        value.port = bp->value.port;
        break;

    case Time:
        value.time = bp->value.time;
        break;

    case Event:
        value.event = new DebuggerEvent(bp->value.event);
    }
    ignore = bp->ignore;
    life = BreakpointLife(bp->life);
    if (bp->condition) {
        condition.reserve(100);
        debugger_expression_deparse(condition.data(), condition.size(), bp->condition);
    }

    if (bp->commands)
        commands = bp->commands;
}

BreakpointsModel::DebuggerBreakpoint::~DebuggerBreakpoint()
{
    if (type == Event)
        delete value.event;
}
