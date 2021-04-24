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

#include "breakpointsmodel.h"

#include "disassamblemodel.h"
#include "fuseemulator.h"
#include "qmlui.h"

extern "C"  {
# include <debugger/debugger.h>
# include <machine.h>
# include <z80/z80.h>
}

static inline uint32_t absolute2PageAddress(uint16_t page, uint16_t address)
{
    uint32_t absAdd = 0;
    switch (page) {
    case 5:
        absAdd = 0x4000;
        break;
    case 2:
        absAdd = 0x8000;
        break;
    default:
        absAdd = 0xc000;
        break;
    }
    return address - absAdd;
}

static QString bytesText(const QByteArray &bytes)
{
    QString ret;
    for (uint8_t byte : bytes)
        ret += formatNumber(byte) + QLatin1Char(' ');
    return ret.trimmed();
}

DisassambleModel::DisassambleModel(QObject *parent)
    : FuseListModel(parent)
{
    qRegisterMetaType<Origin>("Origin");
}

void DisassambleModel::disassamble(uint16_t address, int delta, uint16_t instructions)
{
    auto data = disassambleTemp(address, delta, instructions);
    callFunction([this, data](){
        m_canFetchMore = false;
        beginResetModel();
        std::lock_guard<std::mutex> lock(m_mutex);
        m_disassambleData = *data;
        endResetModel();
        m_canFetchMore = m_address + m_length < 0xffff;
        emit deltaChanged();
    });
}

void DisassambleModel::disassambleMore(DisassambleModel::Origin origin, int size)
{
    uint16_t addr = 0;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_disassambleData.empty()) {
            if (origin == Start)
                addr = m_disassambleData.front().address;
            else
                addr = m_disassambleData.back().address;
            if (!addr)
                return;
        }
    }
    auto data = disassambleTemp(addr, origin == Start ? -size : 0, size);
    callFunction([this, size, origin, data]{
        std::lock_guard<std::mutex> lock(m_mutex);
        int first = (origin == Start) ? 0 : m_disassambleData.size();
        beginInsertRows(QModelIndex(), first, first + size -1);
        m_disassambleData.insert(origin == Start ? m_disassambleData.begin() : m_disassambleData.end(), data->begin(), data->end());
        endInsertRows();
    });
}

void DisassambleModel::update()
{
    pokeEvent([this]{
        if (m_disassambleData.empty())
            return;

        auto data = disassambleTemp(m_disassambleData[0].address, 0, m_disassambleData.size());
        callFunction([this, data] {
            m_canFetchMore = false;
            std::lock_guard<std::mutex> lock(m_mutex);
            m_disassambleData = *data;
            m_canFetchMore = m_address + m_length < 0xffff;
            emit dataChanged(index(0), index(m_disassambleData.size() - 1));
        });
    });
}

int DisassambleModel::address(int index)
{
    if (size_t(index) >= m_disassambleData.size())
        return -1;
    return m_disassambleData[index].address;
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

    case BytesText:
        return bytesText(dd.bytes);

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
    return m_canFetchMore;
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
    ret[BytesText] = "bytesText";
    ret[Disassable] = "disassable";
    return ret;
}

QColor DisassambleModel::color(DisassambleModel::ColorType colorType, const std::unordered_set<int> &types, uint16_t address)
{
    switch (colorType) {
    case Ink:
        return Qt::white;

    case Paper:
        if (types.find(DEBUGGER_BREAKPOINT_TYPE_EXECUTE) != types.end() && address == z80.pc.w)
            return Qt::darkMagenta;

        if (types.find(DEBUGGER_BREAKPOINT_TYPE_EXECUTE) != types.end())
            return Qt::red;

        if (address == z80.pc.w)
            return Qt::blue;

        return QColor(0, 0, 0, 200);
    }
    Q_ASSERT(false);
    return QColor();
}

std::shared_ptr<DisassambleModel::DisassambleDataVector> DisassambleModel::disassambleTemp(uint16_t address, int delta, uint16_t instructions)
{
    std::shared_ptr<DisassambleModel::DisassambleDataVector> disassambledData(new DisassambleModel::DisassambleDataVector);
    disassambledData->reserve(instructions);
    uint16_t pc = address;
    if (address && delta)
        address = debugger_search_instruction(address, delta);
    delta = 0;

    BreakpointsModel* breakpointsModel = g_fuseEmulator->breakpointsModel();
    std::lock_guard<std::mutex> lock(m_mutex);
    std::lock_guard<std::mutex> lockBreakpoints(breakpointsModel->breakpointsMutex());
    m_address = address;
    m_delta = 0;
    m_length = 0;

    char buff[100];
    const auto &addresses = breakpointsModel->addresses();
    while (instructions--) {
        if (pc == address)
            m_delta = -delta;
        size_t len;
        debugger_disassemble(buff, sizeof(buff), &len, address);
        QByteArray bytes;
        for (size_t i = 0; i < len; ++i)
            bytes.push_back(readbyte(address + i));

        debugger_breakpoint_address addr;
        addr.source = memory_source_any;
        addr.page = 0;
        addr.offset = address;
        auto it = addresses.find(addr);
        if (it == addresses.end()) {
            addr.source = memory_source_ram;
            addr.page = machine_current->ram.current_page;
            addr.offset = absolute2PageAddress(addr.page, address);
            it = addresses.find(addr);
        }

        if (it != addresses.end()) // we have a breakpoint
            disassambledData->emplace_back(DisassambleData(address, bytes, QLatin1String(buff), it->second));
        else
            disassambledData->emplace_back(DisassambleData(address, bytes, QLatin1String(buff)));

        address += len;
        ++delta;
        m_length += len;
    }
    return disassambledData;
}

DisassambleModel::DisassambleData::DisassambleData(uint16_t address, const QByteArray &bytes, const QString &disassamble, const std::unordered_set<int> &types)
    : background(color(Paper, types, address))
    , foreground(color(Ink, types, address))
    , address(address)
    , bytes(bytes)
    , disassamble(disassamble)
{
}
