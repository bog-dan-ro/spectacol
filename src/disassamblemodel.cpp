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
#include "disassamblemodel.h"
#include "zxemulator.h"
#include "qmlui.h"

#include <iterator>

#include <zxkit/debug/Disassembler.h>

using namespace Qt::StringLiterals;

static QString bytesText(const QByteArray &bytes)
{
    QString ret;
    for (uint8_t byte : bytes)
        ret += formatNumber(byte) + u' ';
    return ret.trimmed();
}

DisassambleModel::DisassambleModel(QObject *parent) : QAbstractListModel(parent)
{
    qRegisterMetaType<Origin>("Origin");
}

void DisassambleModel::disassamble(uint16_t address, int delta, uint16_t instructions)
{
    auto data = disassambleTemp(address, delta, instructions);
    m_canFetchMore = false;
    beginResetModel();
    m_disassambleData = std::move(data);
    endResetModel();
    m_canFetchMore = m_address + m_length < 0xffff;
    emit deltaChanged();
}

void DisassambleModel::disassambleMore(DisassambleModel::Origin origin, int size)
{
    uint16_t addr = 0;
    if (!m_disassambleData.empty()) {
        if (origin == Start) {
            addr = m_disassambleData.front().address;
            if (!addr)
                return;
        } else {
            const auto &last = m_disassambleData.back();
            const int next = last.address + last.bytes.size();
            if (next > 0xffff)
                return;
            addr = uint16_t(next);
        }
    }
    auto data = disassambleTemp(addr, origin == Start ? -size : 0, uint16_t(size));
    if (data.empty())
        return;
    const int first = (origin == Start) ? 0 : int(m_disassambleData.size());
    beginInsertRows(QModelIndex(), first, first + int(data.size()) - 1);
    m_disassambleData.insert(origin == Start ? m_disassambleData.begin() : m_disassambleData.end(),
                             std::make_move_iterator(data.begin()),
                             std::make_move_iterator(data.end()));
    endInsertRows();
    if (origin == End) {
        const auto &last = m_disassambleData.back();
        m_canFetchMore = last.address + last.bytes.size() <= 0xffff;
    }
}

void DisassambleModel::update()
{
    if (m_disassambleData.empty())
        return;

    auto data =
            disassambleTemp(m_disassambleData[0].address, 0, uint16_t(m_disassambleData.size()));
    const bool reset = data.size() != m_disassambleData.size();
    if (reset)
        beginResetModel();
    m_disassambleData = std::move(data);
    if (reset)
        endResetModel();
    m_canFetchMore = m_address + m_length < 0xffff;
    if (!reset && !m_disassambleData.empty())
        emit dataChanged(index(0), index(int(m_disassambleData.size()) - 1));
}

int DisassambleModel::address(int index)
{
    if (size_t(index) >= m_disassambleData.size())
        return -1;
    return m_disassambleData[index].address;
}

int DisassambleModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_disassambleData.size());
}

QVariant DisassambleModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.column() != 0 || index.row() < 0
        || size_t(index.row()) >= m_disassambleData.size())
        return {};
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
        return int(dd.address);

    case Bytes:
        return dd.bytes;

    case BytesText:
        return bytesText(dd.bytes);

    case Disassable:
        return dd.disassamble;
    case Annotation:
        return dd.annotation;
    default:
        break;
    }
    return {};
}

bool DisassambleModel::canFetchMore(const QModelIndex &parent) const
{
    return !parent.isValid() && m_canFetchMore;
}

void DisassambleModel::fetchMore(const QModelIndex &parent)
{
    if (parent.isValid())
        return;
    m_canFetchMore = false;
    disassambleMore(End, 50);
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
    ret[Annotation] = "annotation";
    return ret;
}

QColor DisassambleModel::color(DisassambleModel::ColorType colorType,
                               const std::unordered_set<int> &types, uint16_t address,
                               uint16_t currentPc)
{
    switch (colorType) {
    case Ink:
        return QColor(0xec, 0xee, 0xf2);

    // Dark tints, so the text stays readable: the PC in the accent's cyan, breakpoints in red
    case Paper:
        if (types.find(BreakpointsModel::BreakOnExecute) != types.end() && address == currentPc)
            return QColor(0x6b, 0x2a, 0x7a);

        if (types.find(BreakpointsModel::BreakOnExecute) != types.end())
            return QColor(0x8a, 0x1c, 0x1c);

        if (address == currentPc)
            return QColor(0x00, 0x5f, 0x62);

        return Qt::transparent;
    }
    Q_ASSERT(false);
    return QColor();
}

DisassambleModel::DisassambleDataVector
DisassambleModel::disassambleTemp(uint16_t address, int delta, uint16_t instructions)
{
    DisassambleDataVector disassambledData;
    disassambledData.reserve(instructions);
    uint16_t pc = address;
    if (address && delta < 0) {
        const int backBytes = -delta * 2;
        address = (address > backBytes) ? uint16_t(address - backBytes) : 0;
    }
    delta = 0;

    m_address = address;
    m_delta = 0;
    m_length = 0;

    const auto &addresses = ZxEmulator::instance().breakpointsModel()->addresses();
    const auto emulator = ZxEmulator::instance().emulator().readLock();
    const uint16_t currentPc = emulator->registers().PC;
    const auto &mmu = emulator->mmu();
    while (instructions--) {
        if (pc == address)
            m_delta = -delta;

        const auto inst =
                zxkit::Disassembler::disassembleOne([&mmu](uint16_t a) { return mmu.peek(a); },
                                                    address, emulator->cpu().isZ80nEnabled());

        const QByteArray bytes(QByteArrayView(inst.bytes).first(inst.length));
        const QString mnemonic = QString::fromStdString(inst.mnemonic);
        if (auto it = addresses.find(address); it != addresses.end())
            disassambledData.emplace_back(address, currentPc, bytes, mnemonic, it->second);
        else
            disassambledData.emplace_back(address, currentPc, bytes, mnemonic);

        disassambledData.back().annotation =
                ZxEmulator::instance().symbols()->annotation(*emulator, address);
        address += inst.length;
        ++delta;
        m_length += inst.length;
    }
    return disassambledData;
}

DisassambleModel::DisassambleData::DisassambleData(uint16_t address, uint16_t currentPc,
                                                   const QByteArray &bytes,
                                                   const QString &disassamble,
                                                   const std::unordered_set<int> &types)
    : background(color(Paper, types, address, currentPc)),
      foreground(color(Ink, types, address, currentPc)),
      address(address),
      bytes(bytes),
      disassamble(disassamble)
{
}
