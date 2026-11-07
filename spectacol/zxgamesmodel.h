/*
    Copyright (c) 2015-2026, BogDan Vatra <bogdan@kde.org>

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

#include "zxrequests.h"

#include <QAbstractListModel>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPointer>
#include <QVariantMap>

class ZXGamesModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(QStringList sources READ sources CONSTANT)
    Q_PROPERTY(bool searching READ searching NOTIFY stateChanged)
    Q_PROPERTY(bool loadingDetails READ loadingDetails NOTIFY stateChanged)
    Q_PROPERTY(bool downloading READ downloading NOTIFY stateChanged)
    Q_PROPERTY(bool hasMore READ hasMore NOTIFY stateChanged)
    Q_PROPERTY(bool searched READ searched NOTIFY stateChanged)
    Q_PROPERTY(int total READ total NOTIFY stateChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY stateChanged)
    Q_PROPERTY(QVariantMap details READ details NOTIFY detailsChanged)

public:
    enum ZXRole { Title = Qt::DisplayRole, Subtitle = Qt::UserRole, Identifier, Screenshot };
    Q_ENUM(ZXRole)

    explicit ZXGamesModel(QObject *parent = nullptr);
    explicit ZXGamesModel(const zxonline::Endpoints &endpoints, QObject *parent = nullptr);
    ~ZXGamesModel() override;

    int source() const
    {
        return m_source;
    }
    void setSource(int source);
    QStringList sources() const;
    bool searching() const
    {
        return !m_searchReply.isNull();
    }
    bool loadingDetails() const
    {
        return !m_detailReply.isNull();
    }
    bool downloading() const
    {
        return !m_downloadReply.isNull();
    }
    bool hasMore() const
    {
        return m_hasMore;
    }
    bool searched() const
    {
        return m_searched;
    }
    int total() const
    {
        return m_total;
    }
    QString errorString() const
    {
        return m_error;
    }
    QVariantMap details() const;

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    bool canFetchMore(const QModelIndex &parent) const override;
    void fetchMore(const QModelIndex &parent) override;
    QHash<int, QByteArray> roleNames() const override;

public slots:
    void search(const QString &title = {});
    void loadMore();
    void select(int row);
    void download(int fileIndex, const QString &folder);
    void cancel();
    void cancelSelection();

signals:
    void sourceChanged();
    void stateChanged();
    void detailsChanged();
    void pageLoaded(bool firstPage);
    void downloaded(const QString &path);

private:
    void fetchPage();
    void clearResults();
    QNetworkReply *get(const QUrl &url);
    void abort(QPointer<QNetworkReply> &reply);
    void setError(const QString &error);

    zxonline::Endpoints m_endpoints;
    QNetworkAccessManager m_network;
    QPointer<QNetworkReply> m_searchReply;
    QPointer<QNetworkReply> m_detailReply;
    QPointer<QNetworkReply> m_downloadReply;
    QList<zxonline::Game> m_games;
    zxonline::Details m_details;
    QString m_selectedId;
    QString m_query;
    QString m_error;
    int m_source = 0;
    int m_nextPage = 0;
    int m_total = 0;
    bool m_hasMore = false;
    bool m_searched = false;
};
