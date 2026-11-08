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

#include "zxrequests.h"

#include <QCryptographicHash>
#include <QFileInfo>
#include <QJsonArray>
#include <QUrlQuery>

#include <stdexcept>

using namespace Qt::StringLiterals;

namespace zxonline {
namespace {
QString segment(const QString &text)
{
    return QString::fromLatin1(QUrl::toPercentEncoding(text));
}

QUrl relativePath(const QString &path)
{
    QUrl url;
    url.setPath(path);
    return url;
}

bool supportedMedia(QString name)
{
    name = name.toLower();
    if (name.endsWith(u".zip"_s))
        name.chop(4);
    static const QStringList formats{ u"tap"_s, u"tzx"_s, u"sna"_s, u"szx"_s, u"z80"_s,
                                      u"nex"_s, u"rzx"_s, u"dsk"_s, u"trd"_s, u"scl"_s,
                                      u"mgt"_s, u"img"_s, u"opd"_s, u"td0"_s };
    return formats.contains(QFileInfo(name).suffix());
}

QString textValue(const QJsonValue &value)
{
    if (value.isArray()) {
        QStringList texts;
        for (const auto &item : value.toArray())
            if (item.isString())
                texts.append(item.toString());
        return texts.join(u", "_s);
    }
    return value.toVariant().toString();
}

QString summary(const QJsonObject &entry)
{
    QStringList parts;
    const auto year = textValue(entry.value(u"originalYearOfRelease"_s));
    if (!year.isEmpty())
        parts.append(year);
    QStringList publishers;
    for (const auto &publisher : entry.value(u"publishers"_s).toArray()) {
        const auto name = publisher.toObject().value(u"name"_s).toString();
        if (!name.isEmpty() && !publishers.contains(name))
            publishers.append(name);
    }
    if (!publishers.isEmpty())
        parts.append(publishers.join(u", "_s));
    const auto machine = entry.value(u"machineType"_s).toString();
    if (!machine.isEmpty())
        parts.append(machine);
    return parts.join(u" · "_s);
}

QUrl screenshotUrl(const QJsonObject &entry, const Endpoints &endpoints)
{
    for (const auto &screen : entry.value(u"screens"_s).toArray()) {
        const auto path = screen.toObject().value(u"url"_s).toString();
        const QUrl url(path);
        if (url.scheme() == u"https"_s || url.scheme() == u"http"_s)
            return url;
        if (path.startsWith(u'/') && !path.startsWith(u"//"_s))
            return endpoints.zxinfoMedia.resolved(relativePath(path.mid(1)));
    }
    return {};
}

QUrl zxinfoFileUrl(const Endpoints &endpoints, const QString &path)
{
    const QUrl url(path);
    if (url.scheme() == u"https"_s || url.scheme() == u"http"_s)
        return url;
    // Follow ZXInfo's own download-link mapping, rather than treating catalogue
    // paths as API-relative URLs. Unknown relative paths are not downloads.
    if (path.startsWith(u"/pub/sinclair/"_s)) {
        const QString mirror =
                u"download/World_of_Spectrum_June_2017_Mirror/World of Spectrum June 2017 Mirror.zip/World of Spectrum June 2017 Mirror/sinclair/"_s;
        return endpoints.archive.resolved(relativePath(mirror + path.mid(14)));
    }
    if (path.startsWith(u"/zxdb/"_s))
        return endpoints.spectrumComputing.resolved(relativePath(path.mid(1)));
    return {};
}

void addFile(Details &details, Media file)
{
    if (!supportedMedia(file.name)
        || (file.url.scheme() != u"https"_s && file.url.scheme() != u"http"_s))
        return;
    for (const auto &existing : details.files)
        if (existing.url == file.url)
            return;
    details.files.append(std::move(file));
}
} // namespace

Endpoints::Endpoints()
    : archive(u"https://archive.org/"_s),
      zxinfo(u"https://api.zxinfo.dk/v5/"_s),
      zxinfoMedia(u"https://zxinfo.dk/media/"_s),
      spectrumComputing(u"https://spectrumcomputing.co.uk/"_s)
{
}

QUrl searchUrl(const Endpoints &endpoints, int source, const QString &text, int page)
{
    QUrl url;
    QUrlQuery query;
    if (source == 1) {
        QString path = u"search"_s;
        if (text.size() == 1 && (text[0].isLetter() || text == u"#"_s))
            path = u"entries/byletter/"_s + segment(text);
        else if (!text.isEmpty())
            path = u"search/titles/"_s + segment(text);
        url = endpoints.zxinfo.resolved(QUrl(path));
        query.addQueryItem(u"size"_s, u"25"_s);
        query.addQueryItem(u"offset"_s, QString::number(page));
        query.addQueryItem(u"mode"_s, u"tiny"_s);
        query.addQueryItem(u"contenttype"_s, u"SOFTWARE"_s);
        query.addQueryItem(u"machinetype"_s, u"ZXSPECTRUM"_s);
    } else {
        url = endpoints.archive.resolved(QUrl(u"advancedsearch.php"_s));
        QString expression = u"collection:(zx_spectrum_library_games)"_s;
        QString escaped = text;
        escaped.replace(u"\\"_s, u"\\\\"_s).replace(u"\""_s, u"\\\""_s);
        if (text.size() == 1 && text[0].isLetterOrNumber())
            expression += u" AND firstTitle:"_s + text;
        else if (!text.isEmpty())
            expression += u" AND title:(\"%1\")"_s.arg(escaped);
        query.addQueryItem(u"q"_s, expression);
        query.addQueryItem(u"fl[]"_s, u"identifier"_s);
        query.addQueryItem(u"fl[]"_s, u"title"_s);
        query.addQueryItem(u"sort[]"_s, u"titleSorter asc"_s);
        query.addQueryItem(u"rows"_s, u"25"_s);
        query.addQueryItem(u"page"_s, QString::number(page + 1));
        query.addQueryItem(u"output"_s, u"json"_s);
    }
    url.setQuery(query);
    return url;
}

QUrl detailsUrl(const Endpoints &endpoints, int source, const QString &id)
{
    QUrl url =
            (source == 1 ? endpoints.zxinfo : endpoints.archive)
                    .resolved(QUrl((source == 1 ? u"entries/"_s : u"metadata/"_s) + segment(id)));
    if (source == 1)
        url.setQuery(u"mode=full"_s);
    return url;
}

Page parsePage(const QJsonObject &object, int source, const Endpoints &endpoints)
{
    Page page;
    if (source == 1) {
        if (!object.value(u"hits"_s).isObject())
            throw std::runtime_error("Unexpected ZXInfo search response");
        const auto hits = object.value(u"hits"_s).toObject();
        const auto total = hits.value(u"total"_s);
        page.total = total.isObject() ? total.toObject().value(u"value"_s).toInt() : total.toInt();
        if (!hits.value(u"hits"_s).isArray())
            throw std::runtime_error("Missing ZXInfo results");
        for (const auto &value : hits.value(u"hits"_s).toArray()) {
            const auto hit = value.toObject();
            const auto entry = hit.value(u"_source"_s).toObject();
            const auto id = hit.value(u"_id"_s).toString();
            const auto title = entry.value(u"title"_s).toString();
            if (!id.isEmpty() && !title.isEmpty())
                page.games.append({ id, title, summary(entry), screenshotUrl(entry, endpoints) });
        }
    } else {
        const auto response = object.value(u"response"_s).toObject();
        if (!response.value(u"docs"_s).isArray())
            throw std::runtime_error("Unexpected Internet Archive search response");
        page.total = response.value(u"numFound"_s).toInt();
        for (const auto &value : response.value(u"docs"_s).toArray()) {
            const auto entry = value.toObject();
            const auto id = entry.value(u"identifier"_s).toString();
            const auto title = textValue(entry.value(u"title"_s));
            if (!id.isEmpty() && !title.isEmpty())
                page.games.append(
                        { id,
                          title,
                          {},
                          endpoints.archive.resolved(QUrl(u"services/img/"_s + segment(id))) });
        }
    }
    return page;
}

Details parseDetails(const QJsonObject &object, const Endpoints &endpoints, int source,
                     const QString &id)
{
    Details details;
    if (source == 1) {
        const auto entry = object.value(u"_source"_s).toObject();
        details.title = entry.value(u"title"_s).toString();
        details.subtitle = summary(entry);
        QStringList description;
        for (const auto &key : { u"genre"_s, u"availability"_s, u"language"_s }) {
            const auto text = textValue(entry.value(key));
            if (!text.isEmpty())
                description.append(text);
        }
        details.description = description.join(u" · "_s);
        details.website = QUrl(u"https://zxinfo.dk/details/"_s + segment(id));
        details.screenshot = screenshotUrl(entry, endpoints);
        QJsonArray files;
        for (const auto &release : entry.value(u"releases"_s).toArray())
            for (const auto &file : release.toObject().value(u"files"_s).toArray())
                files.append(file);
        for (const auto &file : entry.value(u"additionalDownloads"_s).toArray())
            files.append(file);
        for (const auto &value : files) {
            const auto file = value.toObject();
            const auto path = file.value(u"path"_s).toString();
            const auto name =
                    QFileInfo(path.startsWith(u'/') ? path : QUrl(path).path()).fileName();
            QString label = file.value(u"format"_s).toString();
            if (!label.isEmpty())
                label += u" · "_s;
            label += name;
            addFile(details,
                    { zxinfoFileUrl(endpoints, path),
                      name,
                      label,
                      {},
                      file.value(u"size"_s).toInteger() });
        }
    } else {
        const auto metadata = object.value(u"metadata"_s).toObject();
        details.title = textValue(metadata.value(u"title"_s));
        details.subtitle = textValue(metadata.value(u"year"_s));
        const auto creator = textValue(metadata.value(u"creator"_s));
        if (!creator.isEmpty())
            details.subtitle += (details.subtitle.isEmpty() ? QString{} : u" · "_s) + creator;
        details.description = textValue(metadata.value(u"description"_s));
        details.website = endpoints.archive.resolved(QUrl(u"details/"_s + segment(id)));
        details.screenshot = endpoints.archive.resolved(QUrl(u"services/img/"_s + segment(id)));
        for (const auto &value : object.value(u"files"_s).toArray()) {
            const auto file = value.toObject();
            if (file.value(u"private"_s).toString() == u"true"_s
                || file.value(u"private"_s).toBool())
                continue;
            const auto name = file.value(u"name"_s).toString();
            const QUrl url = endpoints.archive.resolved(
                    QUrl(u"download/"_s + segment(id) + u'/' + segment(name)));
            addFile(details,
                    { url, QFileInfo(name).fileName(), name,
                      file.value(u"md5"_s).toString().toLatin1().toLower(),
                      textValue(file.value(u"size"_s)).toLongLong() });
        }
    }
    if (details.title.isEmpty())
        throw std::runtime_error("This title's details are unavailable");
    return details;
}

QString downloadName(int source, const QString &id, const Media &file)
{
    const auto hash = QString::fromLatin1(
            QCryptographicHash::hash(file.url.toEncoded(), QCryptographicHash::Sha256)
                    .toHex()
                    .left(10));
    QString name =
            (source == 1 ? u"zxinfo-"_s : u"archive-"_s) + id + u'-' + hash + u'-' + file.name;
    for (QChar &ch : name)
        if (ch.unicode() < 32 || ch.unicode() == 127 || u"<>:\"/\\|?*"_s.contains(ch))
            ch = u'_';
    // Keep the extension and the distinguishing source/hash if the catalogue's
    // filename is longer than a host filesystem component allows.
    if (name.size() > 200)
        name = name.first(120) + u'-' + name.last(60);
    while (name.endsWith(u'.') || name.endsWith(u' '))
        name.chop(1);
    return name;
}
} // namespace zxonline
