/* zximage.cpp: the ZX Spectrum screen image and texture

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

#include "zximage.h"

#include <rhi/qrhi.h>

#include <algorithm>
#include <span>
#include <utility>

ZxImage::ZxImage()
    : m_frameImage(int(zxkit::VideoFrame::kDefaultWidth), int(zxkit::VideoFrame::kDefaultHeight),
                   QImage::Format_RGBA8888)
{
    m_frameImage.fill(Qt::black);
}

ZxImage::~ZxImage() = default;

ZxImage *ZxImage::instance()
{
    static ZxImage self;
    return &self;
}

void ZxImage::presentFrame(const zxkit::VideoFramePtr &frame)
{
    if (!frame)
        return;

    const auto pixels = frame.readLock();
    bool resized = false;
    bool sourceChanged = false;
    bool changed = false;
    {
        QMutexLocker lock(&m_syncVars);
        if (m_sourceFrame) {
            const auto previous = m_sourceFrame.readLock();
            sourceChanged = previous->paperRect() != pixels->paperRect()
                    || previous->width() != pixels->width()
                    || previous->height() != pixels->height();
        } else {
            sourceChanged = true;
        }
        m_sourceFrame = frame;
        resized = updateImage(*pixels, changed);
    }
    if (sourceChanged)
        emit sourceGeometryChanged();
    if (resized)
        emit geometryChanged(imageSize());
    if (changed)
        emit needsUpdate();
}

bool ZxImage::updateImage(const zxkit::VideoFrame &pixels, bool &changed)
{
    const auto paper = pixels.paperRect();
    const uint32_t sx = std::max(1u, paper.width / 256);
    const uint32_t sy = std::max(1u, paper.height / 192);
    const QSize pixelScale{ int(sx), int(sy) };
    const bool scaleChanged = m_pixelScale != pixelScale;
    m_pixelScale = pixelScale;
    const uint32_t left = std::min<uint32_t>(m_borderMargins.left * sx, paper.x);
    const uint32_t topMargin = std::min<uint32_t>(m_borderMargins.top * sy, paper.y);
    const uint32_t right =
            std::min<uint32_t>(m_borderMargins.right * sx, pixels.width() - paper.x - paper.width);
    const uint32_t bottomMargin = std::min<uint32_t>(m_borderMargins.bottom * sy,
                                                     pixels.height() - paper.y - paper.height);
    const uint32_t sourceX = paper.x - left;
    const uint32_t sourceY = paper.y - topMargin;
    const QSize size(int(left + paper.width + right), int(topMargin + paper.height + bottomMargin));
    const bool resized = m_frameImage.size() != size;
    if (resized) {
        m_frameImage = QImage(size, QImage::Format_RGBA8888);
        m_frameDirty = m_frameImage.rect();
    }
    int top = size.height();
    int bottom = -1;
    for (int y = 0; y < size.height(); ++y) {
        const auto row =
                pixels.scanline(sourceY + uint32_t(y)).subspan(sourceX, uint32_t(size.width()));
        const auto current = std::span(
                reinterpret_cast<const uint32_t *>(m_frameImage.constScanLine(y)), row.size());
        if (!resized && std::ranges::equal(row, current))
            continue;
        std::ranges::copy(row, reinterpret_cast<uint32_t *>(m_frameImage.scanLine(y)));
        top = std::min(top, y);
        bottom = y;
    }
    if (scaleChanged) {
        top = 0;
        bottom = size.height() - 1;
    }
    changed = bottom >= 0;
    if (changed)
        m_frameDirty |= QRect(0, top, size.width(), bottom - top + 1);
    return resized;
}

void ZxImage::setBorderMargins(zxkit::BorderMargins margins)
{
    bool resized = false;
    bool changed = false;
    {
        QMutexLocker lock(&m_syncVars);
        if (m_borderMargins == margins)
            return;
        m_borderMargins = margins;
        if (m_sourceFrame)
            resized = updateImage(*m_sourceFrame.readLock(), changed);
    }
    if (resized)
        emit geometryChanged(imageSize());
    if (changed)
        emit needsUpdate();
}

QMargins ZxImage::borderLimits() const
{
    QMutexLocker lock(&m_syncVars);
    if (!m_sourceFrame)
        return { 32, 32, 32, 32 };
    const auto pixels = m_sourceFrame.readLock();
    const auto paper = pixels->paperRect();
    const auto sx = std::max(1u, paper.width / 256);
    const auto sy = std::max(1u, paper.height / 192);
    return { int(paper.x / sx), int(paper.y / sy),
             int((pixels->width() - paper.x - paper.width) / sx),
             int((pixels->height() - paper.y - paper.height) / sy) };
}

QImage ZxImage::image() const
{
    QMutexLocker lock(&m_syncVars);
    return m_frameImage;
}

QSize ZxImage::imageSize() const
{
    QMutexLocker lock(&m_syncVars);
    return m_frameImage.size();
}

ZxImage::Frame ZxImage::takeFrame()
{
    QMutexLocker lock(&m_syncVars);
    return { m_frameImage, std::exchange(m_frameDirty, {}), m_pixelScale };
}

ZxTexture::ZxTexture() = default;

ZxTexture::~ZxTexture() = default;

void ZxTexture::setFrame(ZxImage::Frame frame)
{
    m_image = std::move(frame.image);
    m_dirty |= frame.dirty;
    m_size = m_image.size();
}

qint64 ZxTexture::comparisonKey() const
{
    return qint64(quintptr(this));
}

QRhiTexture *ZxTexture::rhiTexture() const
{
    return m_texture.get();
}

QSize ZxTexture::textureSize() const
{
    return m_size;
}

bool ZxTexture::hasAlphaChannel() const
{
    return false;
}

bool ZxTexture::hasMipmaps() const
{
    return false;
}

void ZxTexture::commitTextureOperations(QRhi *rhi, QRhiResourceUpdateBatch *resourceUpdates)
{
    if (m_image.isNull())
        return;

    if (!m_texture || m_texture->pixelSize() != m_image.size()) {
        if (m_failedSize == m_image.size())
            return;
        m_texture.reset(rhi->newTexture(QRhiTexture::RGBA8, m_image.size()));
        if (!m_texture->create()) {
            // don't retry every frame, only when the size changes
            qWarning() << "ZxTexture: failed to create a texture of size" << m_image.size();
            m_failedSize = m_image.size();
            m_texture.reset();
            return;
        }
        m_failedSize = {};
        // the new texture has no content, upload the whole image
        resourceUpdates->uploadTexture(m_texture.get(), m_image);
    } else if (!m_dirty.isEmpty()) {
        QRhiTextureSubresourceUploadDescription desc(m_image);
        desc.setSourceTopLeft(m_dirty.topLeft());
        desc.setSourceSize(m_dirty.size());
        desc.setDestinationTopLeft(m_dirty.topLeft());
        resourceUpdates->uploadTexture(m_texture.get(), QRhiTextureUploadEntry(0, 0, desc));
    }
    m_dirty = {};
    // the upload holds its own reference, release ours so the next
    // ZxImage::publishFrame() doesn't need to detach
    m_image = {};
}
