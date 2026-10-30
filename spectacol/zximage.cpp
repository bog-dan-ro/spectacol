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
#include "fuseemulator.h"

#include <QtEndian>
#include <rhi/qrhi.h>

#include <cstring>
#include <utility>

extern "C"  {
# include <display.h>
# include <machine.h>
# include <ui/scaler/scaler.h>
# include <ui/uidisplay.h>
}

// A pixel of a QImage::Format_RGBA8888 image, which stores the bytes in R, G, B, A order
inline constexpr uint32_t zxColor(uint32_t r, uint32_t g, uint32_t b)
{
    return qFromLittleEndian((0xffu << 24) | ((b & 0xffu) << 16) | ((g & 0xffu) << 8) | (r & 0xffu));
}

static const uint32_t palette[16] = {
    zxColor(  0,   0,   0),
    zxColor(  0,   0, 192),
    zxColor(192,   0,   0),
    zxColor(192,   0, 192),
    zxColor(  0, 192,   0),
    zxColor(  0, 192, 192),
    zxColor(192, 192,   0),
    zxColor(192, 192, 192),
    zxColor(  0,   0,   0),
    zxColor(  0,   0, 255),
    zxColor(255,   0,   0),
    zxColor(255,   0, 255),
    zxColor(  0, 255,   0),
    zxColor(  0, 255, 255),
    zxColor(255, 255,   0),
    zxColor(255, 255, 255)
};

extern "C" int uidisplay_init( int width, int height )
{
    ZxImage::instance()->resize(width, height);

    // FUSE calls this on every machine change, selecting the same scaler again
    // would only rescale the image one more time
    if (current_scaler != SCALER_NORMAL) {
        scaler_register_clear();
        scaler_register(SCALER_NORMAL);
        scaler_select_scaler(SCALER_NORMAL);
    }
    display_ui_initialised = 1;
    return 0;
}

extern "C" int uidisplay_hotswap_gfx_mode()
{
    // runs on the FUSE thread, so there is no need to pause the emulation
    ZxImage::instance()->rescale();
    return 0;
}

extern "C" int uidisplay_end()
{
    return 0;
}

extern "C" void uidisplay_putpixel( int x, int y, int colour )
{
    ZxImage::instance()->putPixel(x, y, colour);
}

/* Print the 8 pixels in `data' using ink colour `ink' and paper
   colour `paper' to the screen at ( (8*x) , y ) */
extern "C" void uidisplay_plot8( int x, int y, libspectrum_byte data,
                 libspectrum_byte ink, libspectrum_byte paper )
{
    ZxImage::instance()->plot8(x, y, data, ink, paper);
}

/* Print the 16 pixels in `data' using ink colour `ink' and paper
   colour `paper' to the screen at ( (16*x) , y ) */
extern "C" void uidisplay_plot16( int x, int y, libspectrum_word data,
                 libspectrum_byte ink, libspectrum_byte paper )
{
    ZxImage::instance()->plot16(x, y, data, ink, paper);
}

extern "C" void uidisplay_area( int x, int y, int w, int h )
{
    ZxImage::instance()->update(x, y, w, h);
}

extern "C" void uidisplay_frame_end( void )
{
    ZxImage::instance()->frameEnd();
}

extern "C" void uidisplay_frame_save( void )
{
    ZxImage::instance()->saveScreen();
}

extern "C" void uidisplay_frame_restore( void )
{
    ZxImage::instance()->restoreScreen();
}

ZxImage::ZxImage()
{
    FuseEmulator::instance().startFuseThread();
}

ZxImage::~ZxImage() = default;

ZxImage *ZxImage::instance()
{
    static ZxImage self;
    return &self;
}

void ZxImage::resize(uint32_t w, uint32_t h)
{
    m_rgbImage = QImage{int(w), int(h), QImage::Format_RGBA8888};
    m_rgbImage.fill(Qt::black);
    m_rgbPixelsWidth = m_rgbImage.bytesPerLine() / 4;
    m_rgbPixels = reinterpret_cast<uint32_t *>(m_rgbImage.bits());
    {
        QMutexLocker lock(&m_syncVars);
        m_frameImage = m_rgbImage.copy();
        m_frameDirty = {};
    }
    rescale();
}

void ZxImage::rescale()
{
    update(0, 0, m_rgbImage.width(), m_rgbImage.height());
    // don't wait for the frame end, the emulation might be paused
    publishFrame();
    emit geometryChanged(m_rgbImage.size());
}

QSize ZxImage::imageSize() const
{
    QMutexLocker lock(&m_syncVars);
    return m_frameImage.size();
}

ZxImage::Frame ZxImage::takeFrame()
{
    QMutexLocker lock(&m_syncVars);
    return {m_frameImage, std::exchange(m_frameDirty, {})};
}

void ZxImage::update(int x, int y, int w, int h)
{
    m_dirty |= QRect{x, y, w, h};
}

void ZxImage::frameEnd()
{
    publishFrame();
}

void ZxImage::publishFrame()
{
    const QRect dirty = m_dirty & m_rgbImage.rect();
    m_dirty = {};
    if (dirty.isEmpty())
        return;

    {
        QMutexLocker lock(&m_syncVars);
        // bits() detaches m_frameImage if the renderer still holds a copy of it
        auto *framePixels = reinterpret_cast<uint32_t *>(m_frameImage.bits());
        const auto framePixelsWidth = m_frameImage.bytesPerLine() / 4;
        for (int y = dirty.top(); y <= dirty.bottom(); ++y) {
            std::memcpy(framePixels + framePixelsWidth * y + dirty.left(),
                        m_rgbPixels + m_rgbPixelsWidth * y + dirty.left(),
                        dirty.width() * sizeof(uint32_t));
        }
        m_frameDirty |= dirty;
    }
    emit needsUpdate();
}

void ZxImage::saveScreen()
{
    m_savedImage = m_rgbImage.copy();
}

void ZxImage::restoreScreen()
{
    if (m_savedImage.size() != m_rgbImage.size())
        return;
    // copy the pixels, m_rgbPixels must keep pointing to m_rgbImage's buffer
    std::memcpy(m_rgbPixels, m_savedImage.constBits(), m_rgbImage.sizeInBytes());
    update(0, 0, m_rgbImage.width(), m_rgbImage.height());
}

void ZxImage::putPixel(int x, int y, int colour)
{
    Q_ASSERT(colour < 16);

    const auto rgbColor = palette[colour];

    if (machine_current->timex) {
        // the image is twice as big, every pixel is a 2x2 block
        x <<= 1;
        y <<= 1;
        Q_ASSERT(x + 1 < m_rgbImage.width() && y + 1 < m_rgbImage.height());
        m_rgbPixels[x + m_rgbPixelsWidth * y] = rgbColor;
        m_rgbPixels[x + 1 + m_rgbPixelsWidth * y] = rgbColor;
        m_rgbPixels[x + m_rgbPixelsWidth * (y + 1)] = rgbColor;
        m_rgbPixels[x + 1 + m_rgbPixelsWidth * (y + 1)] = rgbColor;
    } else {
        Q_ASSERT(x < m_rgbImage.width() && y < m_rgbImage.height());
        m_rgbPixels[x + m_rgbPixelsWidth * y] = rgbColor;
    }
}

void ZxImage::plot8(int x, int y, libspectrum_byte data, libspectrum_byte ink, libspectrum_byte paper)
{
    Q_ASSERT(ink < 16 && paper < 16);
    const auto inkColor = palette[ink];
    const auto paperColor = palette[paper];

    if (machine_current->timex) {
        x <<= 4;
        y <<= 1;
        for (int i = 0; i < 2; ++i) {
            auto *dataPtr = m_rgbPixels + x + m_rgbPixelsWidth * y++;
            *(dataPtr++) = ( data & 0x80 ) ? inkColor : paperColor;
            *(dataPtr++) = ( data & 0x80 ) ? inkColor : paperColor;
            *(dataPtr++) = ( data & 0x40 ) ? inkColor : paperColor;
            *(dataPtr++) = ( data & 0x40 ) ? inkColor : paperColor;
            *(dataPtr++) = ( data & 0x20 ) ? inkColor : paperColor;
            *(dataPtr++) = ( data & 0x20 ) ? inkColor : paperColor;
            *(dataPtr++) = ( data & 0x10 ) ? inkColor : paperColor;
            *(dataPtr++) = ( data & 0x10 ) ? inkColor : paperColor;
            *(dataPtr++) = ( data & 0x08 ) ? inkColor : paperColor;
            *(dataPtr++) = ( data & 0x08 ) ? inkColor : paperColor;
            *(dataPtr++) = ( data & 0x04 ) ? inkColor : paperColor;
            *(dataPtr++) = ( data & 0x04 ) ? inkColor : paperColor;
            *(dataPtr++) = ( data & 0x02 ) ? inkColor : paperColor;
            *(dataPtr++) = ( data & 0x02 ) ? inkColor : paperColor;
            *(dataPtr++) = ( data & 0x01 ) ? inkColor : paperColor;
            *dataPtr     = ( data & 0x01 ) ? inkColor : paperColor;
        }
    } else {
        x <<= 3;
        auto *dataPtr = m_rgbPixels + x + m_rgbPixelsWidth * y;
        *dataPtr++ = (data & 0x80) ? inkColor : paperColor;
        *dataPtr++ = (data & 0x40) ? inkColor : paperColor;
        *dataPtr++ = (data & 0x20) ? inkColor : paperColor;
        *dataPtr++ = (data & 0x10) ? inkColor : paperColor;
        *dataPtr++ = (data & 0x08) ? inkColor : paperColor;
        *dataPtr++ = (data & 0x04) ? inkColor : paperColor;
        *dataPtr++ = (data & 0x02) ? inkColor : paperColor;
        *dataPtr   = (data & 0x01) ? inkColor : paperColor;
    }
}

void ZxImage::plot16(int x, int y, libspectrum_word data, libspectrum_byte ink, libspectrum_byte paper)
{
    Q_ASSERT(ink < 16 && paper < 16);

    const auto inkColor = palette[ink];
    const auto paperColor = palette[paper];

    x <<= 4;
    y <<= 1;

    for (int i = 0; i < 2; ++i) {
        auto *dataPtr = m_rgbPixels + x + m_rgbPixelsWidth * y++;
        *dataPtr++ = (data & 0x8000) ? inkColor : paperColor;
        *dataPtr++ = (data & 0x4000) ? inkColor : paperColor;
        *dataPtr++ = (data & 0x2000) ? inkColor : paperColor;
        *dataPtr++ = (data & 0x1000) ? inkColor : paperColor;
        *dataPtr++ = (data & 0x0800) ? inkColor : paperColor;
        *dataPtr++ = (data & 0x0400) ? inkColor : paperColor;
        *dataPtr++ = (data & 0x0200) ? inkColor : paperColor;
        *dataPtr++ = (data & 0x0100) ? inkColor : paperColor;
        *dataPtr++ = (data & 0x0080) ? inkColor : paperColor;
        *dataPtr++ = (data & 0x0040) ? inkColor : paperColor;
        *dataPtr++ = (data & 0x0020) ? inkColor : paperColor;
        *dataPtr++ = (data & 0x0010) ? inkColor : paperColor;
        *dataPtr++ = (data & 0x0008) ? inkColor : paperColor;
        *dataPtr++ = (data & 0x0004) ? inkColor : paperColor;
        *dataPtr++ = (data & 0x0002) ? inkColor : paperColor;
        *dataPtr   = (data & 0x0001) ? inkColor : paperColor;
    }
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
