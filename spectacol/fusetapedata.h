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

#ifndef FUSETAPEDATA_H
#define FUSETAPEDATA_H

#include "fuselistmodel.h"

extern "C"  {
# include <libspectrum.h>
}

class FuseTapeData : public FuseListModel
{
    Q_OBJECT
    enum {
        Label = Qt::DisplayRole,
        Detail = Qt::UserRole + 1,
        Image,
        Data,
        Current
    };
public:
    FuseTapeData();

    // QAbstractItemModel interface
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

public slots:
    int currentIndex() const;
    void setCurrentIndex(int block);
    QByteArray base64CompressedBlockData(int index) const;
    int getBufferLength(int index);

private:
    static void addBlockInfo(libspectrum_tape_block *block, void *user_data);

private:
    struct TapeData {
        QByteArray label;
        QByteArray detail;
        QByteArray data;
        int length = 0;
    };
    std::vector<TapeData> m_tapeBlocks;
};

#endif // FUSETAPEDATA_H
