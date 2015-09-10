#include "pokefindermodel.h"

#include "qmlui.h"

#include <memory.h>
#include <pokefinder/pokefinder.h>

#define MAX_POSSIBLE 20

PokeFinderModel::PokeFinderModel(QObject *parent)
    : FuseListModel(parent)
{
}

void PokeFinderModel::update()
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_dataTmp.clear();
        if (pokefinder_count && pokefinder_count <= MAX_POSSIBLE) {
            for (size_t page = 0; page < MEMORY_PAGES_IN_16K * SPECTRUM_RAM_PAGES; page++ ) {
                memory_page *mapping = &memory_map_ram[page];
                const uint16_t bank = mapping->page_num;

                for (size_t offset = 0; offset < MEMORY_PAGE_SIZE; offset++) {
                    if( ! (pokefinder_impossible[page][offset/8] & 1 << (offset & 7)) ) {
                        const uint16_t bank_offset = mapping->offset + offset;
                        const uint8_t value = mapping->page[offset];
                        m_dataTmp.emplace_back(PokeFinderData(bank, bank_offset, value));
                    }
                }
            }
        }
    }
    callFunction([this]{
        beginResetModel();
        std::lock_guard<std::mutex> lock(m_mutex);
        m_data = std::move(m_dataTmp);
        m_dataTmp.clear();
        endResetModel();
    });
}

int PokeFinderModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_data.size();
}

QVariant PokeFinderModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || size_t(index.row()) >= m_data.size())
        return QVariant();

    switch (role) {
    case Bank:
        return m_data[index.row()].bank;
    case Offset:
        return m_data[index.row()].offset;
    case OffsetText:
        return formatNumber(m_data[index.row()].offset);
    case Value:
        return formatNumber(m_data[index.row()].value);
    }
    return QVariant();
}

QHash<int, QByteArray> PokeFinderModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[Bank] = "bank";
    roles[Offset] = "offset";
    roles[OffsetText] = "offsetText";
    roles[Value] = "value";
    return roles;
}
