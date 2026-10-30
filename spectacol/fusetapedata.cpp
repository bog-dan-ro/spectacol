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

#include "fusetapedata.h"

#include "fuseemulator.h"
#include "qmlui.h"
#include "spectrumscreen.h"

#include <QPointer>

#include <memory>

extern "C"  {
# include <tape.h>
}

FuseTapeData::FuseTapeData()
{
    // The page that owns this model can be closed before the FUSE thread answers.
    QPointer<FuseTapeData> self = this;
    pokeEvent([self] {
        auto blocks = std::make_shared<TapeBlocks>();
        tape_foreach(&addBlockInfo, blocks.get());
        const int current = tape_get_current_block();
        FuseEmulator::instance().callFunction([self, blocks, current] {
            if (!self)
                return;
            self->beginResetModel();
            self->m_tapeBlocks = std::move(*blocks);
            self->endResetModel();
            self->setCurrentBlock(current);
        });
    });
}

int FuseTapeData::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return m_tapeBlocks.size();
}

QVariant FuseTapeData::data(const QModelIndex &index, int role) const
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

QHash<int, QByteArray> FuseTapeData::roleNames() const
{
    static QHash<int, QByteArray> res = {
        {Label, "label"},
        {Detail, "detail"},
        {Current, "isCurrent"}
    };
    return res;
}

void FuseTapeData::selectBlock(int block)
{
    QPointer<FuseTapeData> self = this;
    pokeEvent([self, block] {
        tape_select_block(block);
        const int current = tape_get_current_block();
        FuseEmulator::instance().callFunction([self, current] {
            if (self)
                self->setCurrentBlock(current);
        });
    });
}

void FuseTapeData::setCurrentBlock(int block)
{
    if (m_currentBlock == block)
        return;
    m_currentBlock = block;
    emit currentBlockChanged();
    if (!m_tapeBlocks.empty())
        emit dataChanged(index(0), index(m_tapeBlocks.size() - 1), {Current});
}

QByteArray FuseTapeData::base64CompressedBlockData(int index) const
{
    if (size_t(index) >= m_tapeBlocks.size())
        return QByteArray{};
    return m_tapeBlocks[index].data;
}

int FuseTapeData::getBufferLength(int index)
{
    if (size_t(index) >= m_tapeBlocks.size())
        return 0;
    return m_tapeBlocks[index].length;
}

void FuseTapeData::addBlockInfo(libspectrum_tape_block *block, void *user_data)
{
    auto blocks = reinterpret_cast<TapeBlocks*>(user_data);

    char temp[256];
    libspectrum_tape_block_description(temp + 1, 255, block);
    temp[0] = ' ';
    TapeData tapeData;
    tapeData.label = QByteArray::number(uint(blocks->size() + 1)) + QByteArray(temp);
    tapeData.length = libspectrum_tape_block_data_length(block);
    tapeData.data = QByteArray((const char *)libspectrum_tape_block_data(block), tapeData.length);
    tapeData.data = qCompress(tapeData.data, 9).toBase64(QByteArray::Base64UrlEncoding);
    char *details = nullptr;
    if (!tape_block_details(&details, block)) {
        tapeData.detail = details;
        libspectrum_free(details);
    }
    blocks->push_back(tapeData);
}
