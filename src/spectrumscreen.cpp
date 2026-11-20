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

#include "qmlui.h"
#include "spectrumscreen.h"

#include <zxkit/media/MediaPreview.h>

#include <QFile>
#include <QFileInfo>
#include <QFuture>
#include <QImageReader>
#include <QPromise>
#include <QtConcurrent/QtConcurrentRun>
#ifdef Q_OS_ANDROID
#include "androidintegration.h"
#include <QUrl>
#endif

#include <cstring>

using namespace Qt::StringLiterals;

namespace {

QImage videoFrameToQImage(const zxkit::VideoFrame &vf)
{
    QImage img(int(vf.width()), int(vf.height()), QImage::Format_RGBA8888);
    for (uint32_t y = 0; y < vf.height(); ++y) {
        const auto row = vf.scanline(y);
        std::memcpy(img.scanLine(int(y)), row.data(), row.size() * sizeof(uint32_t));
    }
    return img;
}

// Only placeholders are rasterized at the requested size. Spectrum previews
// retain their native pixels for the browser's nearest-neighbour rendering.
QImage iconImage(const QString &name, QSize requestedSize = {})
{
    if (requestedSize.width() <= 0)
        requestedSize.setWidth(requestedSize.height() > 0 ? requestedSize.height() : 256);
    if (requestedSize.height() <= 0)
        requestedSize.setHeight(requestedSize.width());
    QImageReader reader(u":/images/"_s + name + u".svg"_s);
    reader.setScaledSize(QSize(256, 256).scaled(requestedSize, Qt::KeepAspectRatio));
    return reader.read();
}

QImage mediaImage(std::span<const uint8_t> span, const QString &fileName,
                  const QSize &requestedSize = {})
{
    BlockErrors silenter;
    const std::string fname = fileName.toStdString();
    const auto mediaClass = zxkit::classifyMediaFile(fname, span);

    if (mediaClass == zxkit::MediaClass::Disk) {
        return iconImage(u"floppy"_s, requestedSize);
    }

    try {
        return videoFrameToQImage(zxkit::extractMediaPreview(span, fname));
    } catch (...) {
        return QImage();
    }
}

} // namespace

QImage buff2Image(const unsigned char *buffer, size_t bufferSize, const QString &fileName)
{
    return mediaImage(std::span<const uint8_t>(buffer, bufferSize), fileName);
}

QImage buff2Image(const QByteArray &buffer, const QString &fileName)
{
    return buff2Image(reinterpret_cast<const unsigned char *>(buffer.constData()),
                      size_t(buffer.size()), fileName);
}

QImage scr2Image(const QByteArray &buffer)
{
    return scr2Image(buffer.constData(), size_t(buffer.size()));
}

QImage scr2Image(const char *buffer, size_t bufferSize)
{
    if (bufferSize < 6912)
        return QImage();
    try {
        const std::span<const uint8_t> span(reinterpret_cast<const uint8_t *>(buffer), bufferSize);
        return videoFrameToQImage(zxkit::renderScrToVideoFrame(span));
    } catch (...) {
        return QImage();
    }
}

namespace {

// Larger files are never read for a cover: images and cards, not Spectrum programs
constexpr qint64 kMaxCoverFileBytes = 16 * 1024 * 1024;
// Thousands of compressed screens
constexpr qint64 kCacheBytes = 64 * 1024 * 1024;
constexpr int kThreads = 4;

bool openFile(const QString &path, QFile &file)
{
#ifdef Q_OS_ANDROID
    if (path.startsWith(u"content:"_s))
        return AndroidIntegration::openDocument(QUrl(path), file);
#endif
    file.setFileName(path);
    return file.open(QIODevice::ReadOnly);
}

QByteArray fileScreen(const QByteArray &bytes, const QString &name)
{
    BlockErrors silenter;
    try {
        const auto screen = zxkit::extractMediaScreen(
                { reinterpret_cast<const uint8_t *>(bytes.constData()), size_t(bytes.size()) },
                name.toStdString());
        return { reinterpret_cast<const char *>(screen.data()), qsizetype(screen.size()) };
    } catch (...) {
        return {};
    }
}

// A file's screen: from the cache, else read once and kept there
QImage coverImage(const ScreenCover &cover, const QSize &requestedSize, const ScreenCache &cache,
                  const QPromise<QImage> &promise)
{
    if (zxkit::classifyMediaFile(cover.name.toStdString()) == zxkit::MediaClass::Disk)
        return iconImage(u"floppy"_s, requestedSize);
    auto screen = cache.find(cover.path, cover.size, cover.modified);
    if (!screen) {
        if (cover.size > kMaxCoverFileBytes)
            return iconImage(u"cassette-unknown"_s, requestedSize);
        QFile file;
        if (promise.isCanceled())
            return {};
        if (!openFile(cover.path, file))
            return iconImage(u"cassette-broken"_s, requestedSize);
        const auto bytes = file.read(kMaxCoverFileBytes);
        if (promise.isCanceled())
            return {};
        screen = fileScreen(bytes, cover.name);
        cache.store(cover.path, cover.size, cover.modified, *screen);
    }
    if (screen->isEmpty())
        return iconImage(u"cassette-unknown"_s, requestedSize);
    return scr2Image(*screen);
}

QImage compressedImage(const QString &id, const QSize &requestedSize)
{
    const auto pos = id.indexOf(u':', 13);
    if (pos != -1) {
        bool ok;
        const int delta = id.mid(13, pos - 13).toInt(&ok);
        if (ok && delta >= 0) {
            const auto buff = qUncompress(QByteArray::fromBase64(id.mid(pos + 1).toLatin1(),
                                                                 QByteArray::Base64UrlEncoding));
            if (buff.size() - delta >= 0) {
                auto img = scr2Image(buff.constData() + delta, buff.size() - delta);
                if (!img.isNull())
                    return img;
            }
        }
    }
    return iconImage(u"binary"_s, requestedSize);
}

QImage screenImage(const QString &id, const QSize &requestedSize, const ScreenCache &cache,
                   const QPromise<QImage> &promise)
{
    if (id.startsWith(u"compressed64:"_s))
        return compressedImage(id, requestedSize);
    if (const auto cover = ScreenCover::fromId(id))
        return coverImage(*cover, requestedSize, cache, promise);

    const QFileInfo info(id);
    if (info.isDir())
        return iconImage(u"folder"_s, requestedSize);
    if (!info.isFile())
        return iconImage(u"cassette-wait"_s, requestedSize);
    return coverImage({ id, info.fileName(), info.size(), info.lastModified().toMSecsSinceEpoch() },
                      requestedSize, cache, promise);
}

class ScreenResponse : public QQuickImageResponse
{
public:
    explicit ScreenResponse(QFuture<QImage> future) : m_future(std::move(future))
    {
        // A canceled response still reports that it has finished, for the engine to drop it.
        // Queued: a screen found at once must not finish before the engine connects to it.
        m_future.then(this,
                      [this](const QImage &image) {
                          m_image = image;
                          finishLater();
                      })
                .onCanceled(this, [this] { finishLater(); });
    }

    QQuickTextureFactory *textureFactory() const override
    {
        return QQuickTextureFactory::textureFactoryForImage(m_image);
    }

    void cancel() override
    {
        m_future.cancel();
    }

private:
    void finishLater()
    {
        QMetaObject::invokeMethod(this, &QQuickImageResponse::finished, Qt::QueuedConnection);
    }

    QFuture<QImage> m_future;
    QImage m_image;
};

} // namespace

SpectrumScreenImageProvider::SpectrumScreenImageProvider(QString cacheFolder)
    : m_cache(std::move(cacheFolder))
{
    m_pool.setMaxThreadCount(kThreads);
    m_pool.setObjectName(u"Screens"_s);
    // Keep the cache in bounds; the first screens wait for it
    m_pool.start([cache = m_cache] { cache.prune(kCacheBytes); });
}

SpectrumScreenImageProvider::~SpectrumScreenImageProvider()
{
    m_pool.clear();
    m_pool.waitForDone();
}

QQuickImageResponse *SpectrumScreenImageProvider::requestImageResponse(const QString &id,
                                                                       const QSize &requestedSize)
{
    return new ScreenResponse(QtConcurrent::run(
            &m_pool, [id, requestedSize, cache = m_cache](QPromise<QImage> &promise) {
                auto image = screenImage(id, requestedSize, cache, promise);
                if (!promise.isCanceled())
                    promise.addResult(std::move(image));
            }));
}
