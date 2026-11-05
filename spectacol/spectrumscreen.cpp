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
#include <QImageReader>

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

QImage mediaImage(std::span<const uint8_t> span, const QString &fileName, const QSize &requestedSize = {})
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
    return buff2Image(reinterpret_cast<const unsigned char *>(buffer.constData()), size_t(buffer.size()), fileName);
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

QImage previewImage(const QString &id, const QSize &requestedSize)
{
    if (id.startsWith("compressed64:")) {
        auto pos = id.indexOf(u':', 13);
        if (pos != -1) {
            bool ok;
            int delta = id.mid(13, pos - 13).toInt(&ok);
            if (ok && delta >=0) {
                auto buff = qUncompress(QByteArray::fromBase64(id.mid(pos + 1).toLatin1(), QByteArray::Base64UrlEncoding));
                if (buff.size() - delta >= 0) {
                    auto img = scr2Image(buff.constData() + delta, buff.size() - delta);
                    if (!img.isNull())
                        return img;
                }
            }
            return iconImage(u"binary"_s, requestedSize);
        }
    }

    QFileInfo inf(id);
    if (inf.isDir())
        return iconImage(u"folder"_s, requestedSize);
    if (!inf.isFile())
        return iconImage(u"cassette-wait"_s, requestedSize);

    QFile file(id);
    if (!file.open(QIODevice::ReadOnly))
        return iconImage(u"cassette-broken"_s, requestedSize);

    const auto buffer = file.readAll();
    QImage ret = mediaImage(std::span<const uint8_t>(reinterpret_cast<const uint8_t *>(buffer.constData()), size_t(buffer.size())),
                            inf.fileName(), requestedSize);
    if (ret.isNull())
        return iconImage(u"cassette-unknown"_s, requestedSize);
    return ret;
}

} // namespace

QImage SpectrumScreenImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    const auto image = previewImage(id, requestedSize);
    if (size)
        *size = image.size();
    return image;
}
