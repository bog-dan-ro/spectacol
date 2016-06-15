#ifndef FUSETAPEDATA_H
#define FUSETAPEDATA_H

#include <libspectrum.h>

#include "fuselistmodel.h"

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
