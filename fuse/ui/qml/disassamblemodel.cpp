#include "breakpointsmodel.h"

#include "disassamblemodel.h"
#include "fuseemulator.h"
#include "machine.h"
#include "qmlui.h"

#include <QTimer>

#include <debugger/debugger.h>

DisassambleModel::DisassambleModel(QObject *parent)
    : QAbstractListModel(parent)
{
    qRegisterMetaType<Origin>("Origin");
}

void DisassambleModel::disassamble(uint16_t address, int delta, uint16_t length)
{
    if (address && delta)
        address = debugger_search_instruction(address, delta);
    else
        delta = 0;

    BreakpointsModel* breakpointsModel = g_fuseEmulator->breakpointsModel();
    std::lock_guard<std::mutex> lock(m_mutex);
    std::lock_guard<std::mutex> lockBreakpoints(breakpointsModel->breakpointsMutex());
    m_address = address;
    m_delta = delta;
    m_length = length;
    m_disassambleDataTemp.clear();

    char buff[100];
    size_t len = 0;
    const auto &addresses = breakpointsModel->addresses();
    while (length > len) {
        debugger_disassemble(buff, sizeof(buff), &len, address);
        QString bytes;
        for (size_t i = 0; i < len; ++i)
            bytes += formatNumber(readbyte(address + i)) + QLatin1Char(' ');
        bytes = bytes.trimmed();

        auto it = addresses.find(address);
        if (it != addresses.end()) { // we have a breakpoint
            if (it->second.second.source == memory_source_ram) { // we need to check if is the current page
                if (it->second.second.page == 5 ||
                        it->second.second.page == 2 ||
                        it->second.second.page == machine_current->ram.current_page) {
                    m_disassambleDataTemp.emplace_back(DisassambleData(address, bytes, QLatin1String(buff), DisassambleDataType(it->second.first)));
                } else {
                    m_disassambleDataTemp.emplace_back(DisassambleData(address, bytes, QLatin1String(buff), NormalLine));
                }
            }
            m_disassambleDataTemp.emplace_back(DisassambleData(address, bytes, QLatin1String(buff), DisassambleDataType(it->second.first)));
        } else {
            m_disassambleDataTemp.emplace_back(DisassambleData(address, bytes, QLatin1String(buff), NormalLine));
        }

        address += len;
        length -= len;
    }

    QTimer::singleShot(0, this, [this](){
        beginResetModel();
        std::lock_guard<std::mutex> lock(m_mutex);
        m_disassambleData = std::move(m_disassambleDataTemp);
        m_disassambleDataTemp.clear();
        endResetModel();
        emit deltaChanged();
    });
}

void DisassambleModel::refresh()
{
    disassamble(m_address, m_delta, m_length);
}

void DisassambleModel::disassambleMore(DisassambleModel::Origin origin, int size)
{

}

int DisassambleModel::rowCount(const QModelIndex &parent) const
{
    Q_ASSERT(!parent.isValid());
    return m_disassambleData.size();
}

QVariant DisassambleModel::data(const QModelIndex &index, int role) const
{
    if (size_t(index.row()) >= m_disassambleData.size())
        return QVariant();
    const DisassambleData &dd = m_disassambleData[index.row()];
    switch (role) {
    case Background:
    case SelectedForeground:
        return dd.background;

    case Foreground:
    case SelectedBackground:
        return dd.foreground;

    case AddressText:
        return formatNumber(dd.address);

    case Address:
        return (int)dd.address;

    case Bytes:
        return dd.bytes;

    case Disassable:
        return dd.disassamble;
    default:
        break;
    }
    return QVariant();
}

bool DisassambleModel::canFetchMore(const QModelIndex &parent) const
{
    Q_ASSERT(!parent.isValid());
    return m_address + m_length < 0xffff;
}

void DisassambleModel::fetchMore(const QModelIndex &parent)
{
    Q_ASSERT(!parent.isValid());
    pokeEvent([this]{
        disassambleMore(End, 50);
    });
}

QHash<int, QByteArray> DisassambleModel::roleNames() const
{
    QHash<int, QByteArray> ret;
    ret[Background] = "background";
    ret[Foreground] = "foreground";
    ret[SelectedBackground] = "selectedBackground";
    ret[SelectedForeground] = "selectedForeground";
    ret[Address] = "address";
    ret[AddressText] = "addressText";
    ret[Bytes] = "bytes";
    ret[Disassable] = "disassable";
    return ret;
}

QColor DisassambleModel::color(DisassambleModel::ColorType colorType, DisassambleModel::DisassambleDataType type)
{
    switch (colorType) {
    case Ink:
        switch (type) {
        case NormalLine:
        case BreakExec:
        case BreakRead:
        case BreakWrite:
            return Qt::white;
            break;
        }
        break;

    case Paper:
        switch (type) {
        case NormalLine:
            return QColor(0, 0, 0, 200);
        case BreakExec:
            return Qt::red;
        case BreakRead:
        case BreakWrite:
            return Qt::darkRed;
            break;
        }
        break;
    }
    Q_ASSERT(false);
    return QColor();
}

void DisassambleModel::disassambleTemp(uint16_t address, int delta, uint16_t length)
{

}

DisassambleModel::DisassambleData::DisassambleData(uint16_t address, const QString &bytes, const QString &disassamble, DisassambleModel::DisassambleDataType type)
    : background(color(Paper, type))
    , foreground(color(Ink, type))
    , address(address)
    , bytes(bytes)
    , disassamble(disassamble)
{
}
