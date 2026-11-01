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

#include "nextsdcard.h"

#include <QDir>
#include <QFileInfo>
#include <QNetworkReply>
#include <QSettings>
#include <QStandardPaths>

#include <zxkit/core/Exception.h>
#include <zxkit/media/Compression.h>

using namespace Qt::StringLiterals;

namespace {
constexpr auto kDistroUrl = "https://www.specnext.com/distro/24.11/sn-emulator-24.11.zip";
constexpr auto kImageName = "cspect-next-1gb.img";
constexpr QStringView kSettingsKey = u"Next/sdCardImage";
} // namespace

NextSdCard *NextSdCard::create(QQmlEngine *, QJSEngine *)
{
    QQmlEngine::setObjectOwnership(&instance(), QQmlEngine::CppOwnership);
    return &instance();
}

NextSdCard &NextSdCard::instance()
{
    static NextSdCard res;
    return res;
}

NextSdCard::NextSdCard(QObject *parent)
    : QObject(parent)
{
    if (available())
        m_state = Ready;
}

NextSdCard::~NextSdCard()
{
    cancel();
    stopExtraction();
}

QString NextSdCard::defaultImagePath() const
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + u"/sdcard/"_s + QString::fromUtf8(kImageName);
}

QString NextSdCard::zipPath() const
{
    return QFileInfo(defaultImagePath()).absolutePath() + u"/"_s + QFileInfo(QString::fromLatin1(kDistroUrl)).fileName();
}

QString NextSdCard::imagePath() const
{
    const QString custom = QSettings{}.value(kSettingsKey).toString();
    return custom.isEmpty() ? defaultImagePath() : custom;
}

void NextSdCard::setImagePath(const QString &path)
{
    if ((path.isEmpty() ? defaultImagePath() : path) == imagePath())
        return;
    QSettings{}.setValue(kSettingsKey, path == defaultImagePath() ? QString{} : path);
    emit imagePathChanged();
}

bool NextSdCard::isCustomImage() const
{
    return !QSettings{}.value(kSettingsKey).toString().isEmpty();
}

bool NextSdCard::available() const
{
    return QFileInfo(imagePath()).size() > 0;
}

QString NextSdCard::downloadUrl() const
{
    return QString::fromLatin1(kDistroUrl);
}

void NextSdCard::requestDownload()
{
    emit downloadRequested();
}

void NextSdCard::setState(State state, const QString &error)
{
    m_errorString = error;
    if (m_state == state && error.isEmpty())
        return;
    m_state = state;
    emit stateChanged();
}

void NextSdCard::setProgress(double progress)
{
    if (qFuzzyCompare(m_progress, progress))
        return;
    m_progress = progress;
    emit progressChanged();
}

void NextSdCard::download()
{
    if (m_state == Downloading || m_state == Extracting)
        return;

    // The download goes next to the default image location, even when the
    // user picked another image: downloading means "use the official one"
    setImagePath(defaultImagePath());
    QDir().mkpath(QFileInfo(zipPath()).absolutePath());
    m_cancel = false;
    setProgress(0);

    if (QFileInfo::exists(zipPath())) {
        m_freshZip = false;
        startExtraction();
        return;
    }

    // Resume an interrupted download
    m_zipPart.setFileName(zipPath() + u".part"_s);
    if (!m_zipPart.open(QIODevice::WriteOnly | QIODevice::Append)) {
        setState(Failed, tr("Can't write %1").arg(m_zipPart.fileName()));
        return;
    }
    QNetworkRequest request{QUrl{downloadUrl()}};
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    const qint64 offset = m_zipPart.size();
    if (offset > 0)
        request.setRawHeader("Range", "bytes=" + QByteArray::number(offset) + '-');

    setState(Downloading);
    m_expectedSize = -1;
    m_reply = m_network.get(request);
    connect(m_reply, &QNetworkReply::metaDataChanged, this, [this, offset] {
        const qint64 length = m_reply->header(QNetworkRequest::ContentLengthHeader).toLongLong();
        if (m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 206) {
            // "Content-Range: bytes <first>-<last>/<total>"
            const QByteArray range = m_reply->rawHeader("Content-Range");
            m_expectedSize = range.mid(range.lastIndexOf('/') + 1).toLongLong();
        } else {
            // A server ignoring the range request sends the whole file again
            if (offset > 0)
                m_zipPart.resize(0);
            m_expectedSize = length;
        }
    });
    connect(m_reply, &QNetworkReply::readyRead, this, [this] {
        m_zipPart.write(m_reply->readAll());
    });
    connect(m_reply, &QNetworkReply::downloadProgress, this, [this, offset](qint64 received, qint64 total) {
        if (total > 0)
            setProgress(double(offset + received) / double(offset + total));
    });
    connect(m_reply, &QNetworkReply::finished, this, [this] {
        m_reply->deleteLater();
        m_zipPart.write(m_reply->readAll());
        m_zipPart.close();
        if (m_reply->error() != QNetworkReply::NoError) {
            // Keep the partial file, the next attempt continues from there
            setState(m_cancel ? Idle : Failed, m_cancel ? QString{} : m_reply->errorString());
            return;
        }
        if (m_expectedSize > 0 && m_zipPart.size() != m_expectedSize) {
            if (m_zipPart.size() > m_expectedSize)
                m_zipPart.remove();
            setState(Failed, tr("The download is incomplete (%1 of %2 bytes)").arg(m_zipPart.size()).arg(m_expectedSize));
            return;
        }
        QFile::remove(zipPath());
        if (!QFile::rename(m_zipPart.fileName(), zipPath())) {
            setState(Failed, tr("Can't rename %1").arg(m_zipPart.fileName()));
            return;
        }
        m_freshZip = true;
        startExtraction();
    });
}

void NextSdCard::startExtraction()
{
    stopExtraction();
    setState(Extracting);
    setProgress(0);
    const std::filesystem::path zip = zipPath().toStdString();
    const std::filesystem::path image = defaultImagePath().toStdString();
    m_extractThread = std::jthread([this, zip, image] {
        QString error;
        bool corruptZip = false;
        try {
            auto part = image;
            part += ".part";
            zxkit::extractZipEntryToFile(zip, kImageName, part, [this](uint64_t done, uint64_t total) {
                QMetaObject::invokeMethod(this, [this, p = double(done) / double(total)] { setProgress(p); }, Qt::QueuedConnection);
                return !m_cancel.load();
            });
            std::filesystem::rename(part, image);
        } catch (const zxkit::FormatException &e) {
            error = QString::fromUtf8(e.rawMessage());
            corruptZip = true;
        } catch (const zxkit::Exception &e) {
            error = QString::fromUtf8(e.rawMessage());
        } catch (const std::exception &e) {
            error = QString::fromUtf8(e.what());
        }
        QMetaObject::invokeMethod(this, [this, zip, error, corruptZip] {
            if (!error.isEmpty()) {
                if (corruptZip) {
                    // Don't keep a broken zip around (e.g. a truncated download):
                    // fetch it again, once, if it wasn't just downloaded
                    std::error_code ec;
                    std::filesystem::remove(zip, ec);
                    if (!m_freshZip && !m_cancel) {
                        setState(Idle);
                        download();
                        return;
                    }
                }
                setState(m_cancel ? Idle : Failed, m_cancel ? QString{} : error);
                return;
            }
            std::error_code ec;
            std::filesystem::remove(zip, ec); // the image is all we need
            emit imagePathChanged();
            setState(Ready);
            emit ready();
        }, Qt::QueuedConnection);
    });
}

void NextSdCard::stopExtraction()
{
    if (m_extractThread.joinable()) {
        m_cancel = true;
        m_extractThread.join();
        m_cancel = false;
    }
}

void NextSdCard::cancel()
{
    m_cancel = true;
    if (m_reply)
        m_reply->abort();
}
