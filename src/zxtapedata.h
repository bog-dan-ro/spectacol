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

class ZxTapeData : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int currentBlock READ currentBlock NOTIFY currentBlockChanged)
    enum { Label = Qt::DisplayRole, Detail = Qt::UserRole + 1, Image, Data, Current };

public:
    ZxTapeData();

    // QAbstractItemModel interface
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int currentBlock() const
    {
        return m_currentBlock;
    }

signals:
    void currentBlockChanged();

public slots:
    void selectBlock(int block);
    bool resetAndLoadBlock(int block);
    QByteArray base64CompressedBlockData(int index) const;
    int getBufferLength(int index);

private:
    struct TapeData
    {
        QByteArray label;
        QByteArray detail;
        QByteArray data;
        int length = 0;
    };
    using TapeBlocks = std::vector<TapeData>;

    void setCurrentBlock(int block);

private:
    TapeBlocks m_tapeBlocks;
    int m_currentBlock = -1;
};
