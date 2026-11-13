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

#include "zxgamesmodel.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QNetworkRequest>
#include <QSaveFile>

#include <algorithm>
#include <stdexcept>
#include <utility>

using namespace Qt::StringLiterals;

namespace {
QJsonObject readObject(QNetworkReply &reply)
{
    if (reply.error() != QNetworkReply::NoError)
        throw std::runtime_error(reply.errorString().toStdString());
    QJsonParseError error;
    const auto document = QJsonDocument::fromJson(reply.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !document.isObject())
        throw std::runtime_error("The service returned invalid JSON");
    return document.object();
}

QByteArray readMedia(QNetworkReply &reply, const zxonline::Media &file)
{
    if (reply.error() != QNetworkReply::NoError)
        throw std::runtime_error(reply.errorString().toStdString());
    const auto bytes = reply.readAll();
    const auto contentType = reply.header(QNetworkRequest::ContentTypeHeader).toString();
    if (bytes.isEmpty() || contentType.startsWith(u"text/html"_s, Qt::CaseInsensitive))
        throw std::runtime_error("The server did not return a media file");
    if (file.size > 0 && bytes.size() != file.size)
        throw std::runtime_error("The downloaded size does not match the catalogue");
    if (!file.md5.isEmpty()
        && QCryptographicHash::hash(bytes, QCryptographicHash::Md5).toHex() != file.md5)
        throw std::runtime_error("The downloaded checksum does not match the catalogue");
    return bytes;
}
} // namespace

ZXGamesModel::ZXGamesModel(QObject *parent) : ZXGamesModel(zxonline::Endpoints{}, parent) { }

ZXGamesModel::ZXGamesModel(const zxonline::Endpoints &endpoints, QObject *parent)
    : QAbstractListModel(parent), m_endpoints(endpoints)
{
}

ZXGamesModel::~ZXGamesModel()
{
    for (auto &reply : m_screenshotReplies)
        abort(reply);
    abort(m_searchReply);
    abort(m_detailReply);
    abort(m_downloadReply);
}

QStringList ZXGamesModel::sources() const
{
    return { u"Internet Archive"_s, u"ZXInfo"_s };
}

void ZXGamesModel::setSource(int source)
{
    if (source < 0 || source >= sources().size() || source == m_source)
        return;
    cancel();
    m_source = source;
    clearResults();
    emit sourceChanged();
}

int ZXGamesModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_games.size());
}

QVariant ZXGamesModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_games.size())
        return {};
    const auto &game = m_games[index.row()];
    switch (role) {
    case Title:
        return game.title;
    case Subtitle:
        return game.subtitle;
    case Identifier:
        return game.id;
    case Screenshot:
        return game.screenshot;
    case ScreenshotLoading:
        return m_source == 0 && m_screenshotReplies.contains(game.id);
    default:
        return {};
    }
}

QHash<int, QByteArray> ZXGamesModel::roleNames() const
{
    return { { Title, "title" },
             { Subtitle, "subtitle" },
             { Identifier, "identifier" },
             { Screenshot, "screenshot" },
             { ScreenshotLoading, "screenshotLoading" } };
}

bool ZXGamesModel::canFetchMore(const QModelIndex &parent) const
{
    return !parent.isValid() && m_hasMore && !searching();
}

void ZXGamesModel::fetchMore(const QModelIndex &parent)
{
    if (canFetchMore(parent))
        fetchPage();
}

void ZXGamesModel::loadMore()
{
    fetchMore({});
}

void ZXGamesModel::updateScreenshot(const QString &id, const QUrl &url)
{
    for (qsizetype row = 0; row < m_games.size(); ++row) {
        if (m_games[row].id == id) {
            m_games[row].screenshot = url;
            emit dataChanged(index(int(row)), index(int(row)), { Screenshot, ScreenshotLoading });
            return;
        }
    }
}

void ZXGamesModel::loadScreenshot(int row)
{
    if (m_source != 0 || row < 0 || row >= m_games.size())
        return;
    const auto id = m_games[row].id;
    if (m_screenshotReplies.contains(id))
        return;
    if (m_screenshotCache.contains(id)) {
        updateScreenshot(id, m_screenshotCache.value(id));
        return;
    }
    auto *reply = get(zxonline::detailsUrl(m_endpoints, 0, id));
    m_screenshotReplies.insert(id, reply);
    emit dataChanged(index(row), index(row), { ScreenshotLoading });
    connect(reply, &QNetworkReply::finished, this, [this, reply, id] {
        try {
            const auto file = zxonline::archivePreviewMedia(readObject(*reply), m_endpoints, id);
            if (file.url.isEmpty() || !m_previewDirectory.isValid()) {
                m_screenshotReplies.remove(id);
                m_screenshotCache.insert(id, {});
                updateScreenshot(id, {});
            } else {
                auto *mediaReply = get(file.url);
                m_screenshotReplies.insert(id, mediaReply);
                connect(mediaReply, &QNetworkReply::finished, this, [this, mediaReply, id, file] {
                    QUrl screenshot;
                    try {
                        const auto bytes = readMedia(*mediaReply, file);
                        const auto name =
                                QString::fromLatin1(QCryptographicHash::hash(
                                                            id.toUtf8(), QCryptographicHash::Sha256)
                                                            .toHex())
                                + u'.' + QFileInfo(file.name).completeSuffix();
                        const auto path = m_previewDirectory.filePath(name);
                        QSaveFile output(path);
                        if (output.open(QIODevice::WriteOnly) && output.write(bytes) == bytes.size()
                            && output.commit()) {
                            screenshot = QUrl(u"image://spectrum/"_s + path);
                            m_screenshotCache.insert(id, screenshot);
                        }
                    } catch (const std::exception &) {
                        // Preview failures leave details and downloads available.
                    }
                    m_screenshotReplies.remove(id);
                    updateScreenshot(id, screenshot);
                    mediaReply->deleteLater();
                });
            }
        } catch (const std::exception &) {
            m_screenshotReplies.remove(id);
            updateScreenshot(id, {});
        }
        reply->deleteLater();
    });
}

QVariantMap ZXGamesModel::details() const
{
    QVariantList files;
    for (const auto &file : m_details.files)
        files.append(QVariantMap{ { u"label"_s, file.label }, { u"url"_s, file.url } });
    return { { u"title"_s, m_details.title },
             { u"subtitle"_s, m_details.subtitle },
             { u"description"_s, m_details.description },
             { u"website"_s, m_details.website },
             { u"screenshot"_s, m_details.screenshot },
             { u"files"_s, files } };
}

void ZXGamesModel::abort(QPointer<QNetworkReply> &reply)
{
    const auto old = std::exchange(reply, {});
    if (old) {
        // Disconnect first: abort() can synchronously emit finished().
        old->disconnect(this);
        old->abort();
        old->deleteLater();
    }
}

void ZXGamesModel::cancel()
{
    for (auto &reply : m_screenshotReplies)
        abort(reply);
    m_screenshotReplies.clear();
    abort(m_searchReply);
    abort(m_detailReply);
    abort(m_downloadReply);
    emit stateChanged();
}

void ZXGamesModel::cancelSelection()
{
    abort(m_detailReply);
    abort(m_downloadReply);
    m_selectedId.clear();
    m_details = {};
    m_error.clear();
    emit detailsChanged();
    emit stateChanged();
}

void ZXGamesModel::clearResults()
{
    beginResetModel();
    m_games.clear();
    m_total = m_nextPage = 0;
    m_hasMore = m_searched = false;
    endResetModel();
    m_selectedId.clear();
    m_details = {};
    m_error.clear();
    emit detailsChanged();
    emit stateChanged();
}

void ZXGamesModel::setError(const QString &error)
{
    m_error = error;
    emit stateChanged();
}

QNetworkReply *ZXGamesModel::get(const QUrl &url)
{
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      u"Spectacol/1.6 (+https://github.com/bog-dan-ro/spectacol)"_s);
    request.setTransferTimeout(30000);
    return m_network.get(request);
}

void ZXGamesModel::search(const QString &title)
{
    cancel();
    clearResults();
    m_query = title.trimmed();
    if (m_query.size() > 200) {
        setError(tr("Use at most 200 characters for a search."));
        return;
    }
    fetchPage();
}

void ZXGamesModel::fetchPage()
{
    m_error.clear();
    m_searchReply = get(zxonline::searchUrl(m_endpoints, m_source, m_query, m_nextPage));
    const auto reply = m_searchReply;
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        const bool firstPage = m_nextPage == 0;
        bool succeeded = false;
        try {
            const auto page = zxonline::parsePage(readObject(*reply), m_source, m_endpoints);
            QList<zxonline::Game> added;
            for (const auto &game : page.games) {
                const auto hasId = [&game](const auto &existing) { return game.id == existing.id; };
                if (!std::ranges::any_of(m_games, hasId) && !std::ranges::any_of(added, hasId))
                    added.append(game);
            }
            if (!added.isEmpty()) {
                const int start = int(m_games.size());
                beginInsertRows({}, start, start + int(added.size()) - 1);
                m_games.append(added);
                endInsertRows();
            }
            m_total = std::max(page.total, int(m_games.size()));
            m_hasMore = !added.isEmpty() && m_games.size() < m_total;
            m_searched = true;
            ++m_nextPage;
            succeeded = true;
        } catch (const std::exception &error) {
            setError(tr("%1 search failed: %2")
                             .arg(sources()[m_source], QString::fromUtf8(error.what())));
        }
        m_searchReply.clear();
        reply->deleteLater();
        if (succeeded)
            emit pageLoaded(firstPage);
        emit stateChanged();
    });
    emit stateChanged();
}

void ZXGamesModel::select(int row)
{
    if (row < 0 || row >= m_games.size())
        return;
    if (loadingDetails() && m_selectedId == m_games[row].id)
        return;
    abort(m_detailReply);
    abort(m_downloadReply);
    m_selectedId = m_games[row].id;
    m_details = {};
    m_error.clear();
    emit detailsChanged();
    m_detailReply = get(zxonline::detailsUrl(m_endpoints, m_source, m_selectedId));
    const auto reply = m_detailReply;
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        try {
            m_details =
                    zxonline::parseDetails(readObject(*reply), m_endpoints, m_source, m_selectedId);
            emit detailsChanged();
        } catch (const std::exception &error) {
            setError(tr("Could not load title details: %1").arg(QString::fromUtf8(error.what())));
        }
        m_detailReply.clear();
        reply->deleteLater();
        emit stateChanged();
    });
    emit stateChanged();
}

void ZXGamesModel::download(int fileIndex, const QString &folder)
{
    if (downloading() || fileIndex < 0 || fileIndex >= m_details.files.size())
        return;
    m_error.clear();
    const auto file = m_details.files[fileIndex];
    const auto path = QDir(folder).filePath(zxonline::downloadName(m_source, m_selectedId, file));
    m_downloadReply = get(file.url);
    const auto reply = m_downloadReply;
    connect(reply, &QNetworkReply::finished, this, [this, reply, file, path] {
        bool saved = false;
        try {
            const auto bytes = readMedia(*reply, file);
            if (!QFileInfo(path).absoluteDir().mkpath(u"."_s))
                throw std::runtime_error("Cannot create the download folder");
            QSaveFile output(path);
            if (!output.open(QIODevice::WriteOnly) || output.write(bytes) != bytes.size()
                || !output.commit())
                throw std::runtime_error(output.errorString().toStdString());
            saved = true;
        } catch (const std::exception &error) {
            setError(tr("Download failed: %1").arg(QString::fromUtf8(error.what())));
        }
        m_downloadReply.clear();
        reply->deleteLater();
        emit stateChanged();
        if (saved)
            emit downloaded(path);
    });
    emit stateChanged();
}
