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

#include <QObject>
#include <QPointer>

class ZXGamesModel;
class QNetworkReply;
class ZXRequest : public QObject
{
    Q_OBJECT
public:
    explicit ZXRequest(ZXGamesModel *parent);
    virtual ~ZXRequest();
    void abort();

protected:
    QPointer<QNetworkReply> m_reply;
    QByteArray m_data;
    ZXGamesModel *m_model;
    bool m_aborted = false;
};

class ZXSearchRequest : public ZXRequest
{
    Q_OBJECT
public:
    explicit ZXSearchRequest(ZXGamesModel *parent);
};

class ZXFetchRequest : public ZXRequest
{
public:
    explicit ZXFetchRequest(ZXGamesModel *parent, size_t start, size_t count);

private:
    size_t m_start;
    size_t m_count;
};

class ZXUpdateRequest : public ZXRequest
{
public:
    explicit ZXUpdateRequest(ZXGamesModel *parent, const QString &identifier, size_t row);

private:
    QString m_identifier;
    size_t m_row;
    QByteArray m_fileData;
};
