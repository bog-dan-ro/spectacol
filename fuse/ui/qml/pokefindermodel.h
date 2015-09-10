#ifndef POKEFINDERMODEL_H
#define POKEFINDERMODEL_H

#include "fuselistmodel.h"

#include <vector>
#include <mutex>

class PokeFinderModel : public FuseListModel
{
    enum {
        Bank = Qt::UserRole + 1,
        Offset,
        OffsetText,
        Value
    };

public:
    PokeFinderModel(QObject *parent);
    void update();

protected:
    // QAbstractItemModel interface
    int rowCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QHash<int, QByteArray> roleNames() const;

private:
    struct PokeFinderData {
        PokeFinderData(uint16_t bank, uint16_t offset, uint8_t value)
            : bank(bank), offset(offset), value(value) {}
        uint16_t bank;
        uint16_t offset;
        uint8_t value;
    };

    mutable std::mutex m_mutex;
    std::vector<PokeFinderData> m_data, m_dataTmp;
};

#endif // POKEFINDERMODEL_H
