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

#include <QAbstractListModel>
#include <QHash>
#include <QNetworkAccessManager>
#include <QUrl>

#include <memory>
#include <unordered_set>
#include <vector>

class ZXRequest;
class ZXGamesModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum ZXRole {
        Title = Qt::DisplayRole,
        Path = Qt::UserRole,
        ScreenFile
    };
    Q_ENUM(ZXRole)

    struct ZXData {
        ZXData(const QString &title, const QString &identifier)
            : title(title)
            , identifier(identifier)
        {}
        QString title;
        QString identifier;
        QString path;
    };

public:
    explicit ZXGamesModel(QObject *parent = 0);
    ~ZXGamesModel();

    const QString &searchString() const { return m_searchString; }
    void setSearchData(const std::vector<ZXData> &data, size_t total);
    void setFetchedData(const std::vector<ZXData> &data);
    void updateData(size_t row, const QString &path);
    QNetworkAccessManager &networkManager() { return m_netManager; }
    void requestFinished(ZXRequest* req);
    void updateFinished(const QString &identifier);

    // QAbstractItemModel interface
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    bool canFetchMore(const QModelIndex &parent) const override;
    void fetchMore(const QModelIndex &parent) override;
    QHash<int, QByteArray> roleNames() const override;

public slots:
    void search(const QString &title = QString(), const QString &firstTitle = QString());
    QString getPath(int index);

signals:
    void noResults();

private:
    void abortAll();

private:
    size_t m_totalFound = 0;
    size_t m_fetchStart = 0;
    QString m_searchString;

    std::unordered_set<ZXRequest*> m_pendingRequests;
    mutable QHash<QString, ZXRequest*> m_pendingUpdates;

    std::vector<ZXData> m_data;

    QNetworkAccessManager m_netManager;
};
