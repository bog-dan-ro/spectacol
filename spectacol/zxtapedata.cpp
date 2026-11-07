/*
    Copyright (c) 2016, BogDan Vatra <bogdan@kde.org>

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

#include "zxtapedata.h"

#include "zxemulator.h"

#include <string>
#include <utility>
#include <vector>

ZxTapeData::ZxTapeData()
{
    // Copy the blocks out under the lock, compress them after
    std::vector<std::pair<std::string, std::vector<uint8_t>>> blocks;
    int current = 0;
    {
        const auto emulator = ZxEmulator::instance().emulator().readLock();
        const auto &deck = emulator->tapeDeck();
        for (const auto &blk : deck.blocks())
            blocks.emplace_back(blk.description, blk.data);
        current = int(deck.currentBlockIndex());
    }
    for (const auto &[description, data] : blocks) {
        TapeData td;
        td.label = QByteArray::number(uint(m_tapeBlocks.size() + 1)) + " "
                + QByteArray::fromStdString(description);
        td.detail = QByteArray::fromStdString(description);
        td.length = int(data.size());
        td.data = qCompress(data.data(), qsizetype(data.size()), 9)
                          .toBase64(QByteArray::Base64UrlEncoding);
        m_tapeBlocks.push_back(std::move(td));
    }
    m_currentBlock = current;
    connect(&ZxEmulator::instance(), &ZxEmulator::tapeBlockChanged, this,
            &ZxTapeData::setCurrentBlock);
}

int ZxTapeData::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return int(m_tapeBlocks.size());
}

QVariant ZxTapeData::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || size_t(index.row()) >= m_tapeBlocks.size())
        return {};
    switch (role) {
    case Label:
        return m_tapeBlocks[index.row()].label;

    case Detail:
        return m_tapeBlocks[index.row()].detail;

    case Current:
        return index.row() == m_currentBlock;

    default:
        return {};
    }
}

QHash<int, QByteArray> ZxTapeData::roleNames() const
{
    static QHash<int, QByteArray> res = { { Label, "label" },
                                          { Detail, "detail" },
                                          { Current, "isCurrent" } };
    return res;
}

void ZxTapeData::selectBlock(int block)
{
    int current = 0;
    {
        auto emulator = ZxEmulator::instance().emulator().writeLock();
        auto &deck = emulator->tapeDeck();
        if (block >= 0)
            deck.seekBlock(size_t(block));
        current = int(deck.currentBlockIndex());
    }
    setCurrentBlock(current);
}

bool ZxTapeData::resetAndLoadBlock(int block)
{
    if (block < 0 || std::size_t(block) >= m_tapeBlocks.size())
        return false;
    bool supported = false;
    auto &host = ZxEmulator::instance();
    if (!host.withEmulator([&](zxkit::Emulator &emulator) {
            supported = emulator.autoLoadTape(std::size_t(block));
        }))
        return false;
    if (!supported)
        host.showMessage(tr("Automatic tape loading is unavailable for this machine."),
                         ZxEmulator::Warning);
    if (supported)
        setCurrentBlock(block);
    return supported;
}

void ZxTapeData::setCurrentBlock(int block)
{
    if (m_currentBlock == block)
        return;
    m_currentBlock = block;
    emit currentBlockChanged();
    if (!m_tapeBlocks.empty())
        emit dataChanged(index(0), index(int(m_tapeBlocks.size()) - 1), { Current });
}

QByteArray ZxTapeData::base64CompressedBlockData(int index) const
{
    if (size_t(index) >= m_tapeBlocks.size())
        return QByteArray{};
    return m_tapeBlocks[index].data;
}

int ZxTapeData::getBufferLength(int index)
{
    if (size_t(index) >= m_tapeBlocks.size())
        return 0;
    return m_tapeBlocks[index].length;
}
