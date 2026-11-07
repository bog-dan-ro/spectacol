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

#include <QJsonObject>
#include <QList>
#include <QString>
#include <QUrl>

namespace zxonline {

struct Endpoints
{
    Endpoints();
    QUrl archive;
    QUrl zxinfo;
    QUrl zxinfoMedia;
    QUrl spectrumComputing;
};

struct Game
{
    QString id;
    QString title;
    QString subtitle;
    QUrl screenshot;
};

struct Media
{
    QUrl url;
    QString name;
    QString label;
    QByteArray md5;
    qint64 size = 0;
};

struct Details
{
    QString title;
    QString subtitle;
    QString description;
    QUrl website;
    QUrl screenshot;
    QList<Media> files;
};

struct Page
{
    QList<Game> games;
    int total = 0;
};

// Source 0 is Internet Archive, source 1 is ZXInfo.
QUrl searchUrl(const Endpoints &endpoints, int source, const QString &text, int page);
QUrl detailsUrl(const Endpoints &endpoints, int source, const QString &id);
Page parsePage(const QJsonObject &object, int source, const Endpoints &endpoints = {});
Details parseDetails(const QJsonObject &object, const Endpoints &endpoints, int source,
                     const QString &id);
QString downloadName(int source, const QString &id, const Media &file);

} // namespace zxonline
