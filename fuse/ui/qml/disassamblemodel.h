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

#ifndef DISASSAMBLEMODEL_H
#define DISASSAMBLEMODEL_H

#include <fuselistmodel.h>

#include <debugger/breakpoint.h>

#include <mutex>
#include <vector>
#include <unordered_set>

#include <QColor>

class DisassambleModel : public FuseListModel
{
    Q_OBJECT
    Q_ENUMS(Origin)

    Q_PROPERTY(int delta READ delta NOTIFY deltaChanged)
    enum {
        Background = Qt::BackgroundColorRole + 1,
        Foreground = Qt::ForegroundRole + 1,
        SelectedBackground = Qt::UserRole + 1,
        SelectedForeground,
        Address,
        AddressText,
        Bytes,
        Disassable
    };

    enum ColorType {
        Paper,
        Ink
    };

    struct DisassambleData {
        DisassambleData(uint16_t address,
                        const QString &bytes,
                        const QString &disassamble,
                        const std::unordered_set<int> &types = std::unordered_set<int>());
        QColor background, foreground;
        uint16_t address = 0;
        QString bytes;
        QString disassamble;
    };

public:
    enum Origin {
        Start,
        End
    };

public:
    DisassambleModel(QObject *parent);

    void disassamble(uint16_t address, int delta = -10, uint16_t instructions = 50);
    void disassambleMore(Origin origin, int size);

    // QAbstractItemModel interface
    int rowCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    bool canFetchMore(const QModelIndex &parent) const;
    void fetchMore(const QModelIndex &parent);

    int delta() const { return -m_delta; }

signals:
    void deltaChanged();

protected:
    QHash<int, QByteArray> roleNames() const;

private:
    static QColor color(ColorType colorType, const std::unordered_set<int> &types, uint16_t address);
    void disassambleTemp(uint16_t address, int delta, uint16_t instructions);

private:
    uint16_t m_address = 0;
    int m_delta = 0;
    uint16_t m_length = 0;
    bool m_canFetchMore = false;
    mutable std::mutex m_mutex;
    std::vector<DisassambleData> m_disassambleData, m_disassambleDataTemp;
};

#endif // DISASSAMBLEMODEL_H
