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

#include <QImage>
#include <QMutex>
#include <QSGTexture>

#include <memory>

extern "C"  {
# include <libspectrum.h>
}

class QRhiTexture;

class ZxImage : public QObject
{
    Q_OBJECT
private:
    ZxImage();
    ~ZxImage();
public:
    struct Frame
    {
        QImage image;
        QRect dirty;
    };

    static ZxImage *instance();

    // Any thread
    QSize imageSize() const;
    // Returns the last finished frame and the area changed since the previous call
    Frame takeFrame();

    // FUSE thread only
    void resize(uint32_t w, uint32_t h);
    void rescale();
    void putPixel(int x, int y, int colour);
    void plot8(int x, int y, libspectrum_byte data, libspectrum_byte ink, libspectrum_byte paper);
    void plot16(int x, int y, libspectrum_word data, libspectrum_byte ink, libspectrum_byte paper);

    void update(int x, int y, int w, int h);
    void frameEnd();
    void saveScreen();
    void restoreScreen();

signals:
    void needsUpdate();
    void geometryChanged(QSize size);

private:
    void publishFrame();

private:
    // FUSE thread only. FUSE draws into m_rgbImage through m_rgbPixels, which bypasses
    // QImage's copy on write, so m_rgbImage must never be shallow copied.
    QImage m_rgbImage, m_savedImage;
    uint32_t *m_rgbPixels = nullptr;
    uint32_t m_rgbPixelsWidth = 0;
    QRect m_dirty;

    // Guarded by m_syncVars. publishFrame() copies the dirty part of m_rgbImage to
    // m_frameImage; the renderer gets shallow copies of it, which the next publishFrame()
    // detaches from, so they never change while QRhi uploads them.
    mutable QMutex m_syncVars;
    QImage m_frameImage;
    QRect m_frameDirty;
};

// The ZX screen texture, it uploads only the parts of the frame that changed
class ZxTexture : public QSGTexture
{
public:
    ZxTexture();
    ~ZxTexture() override;

    // Render thread, while the GUI thread is blocked
    void setFrame(ZxImage::Frame frame);

    // QSGTexture interface
    qint64 comparisonKey() const final;
    QRhiTexture *rhiTexture() const final;
    QSize textureSize() const final;
    bool hasAlphaChannel() const final;
    bool hasMipmaps() const final;
    void commitTextureOperations(QRhi *rhi, QRhiResourceUpdateBatch *resourceUpdates) final;

private:
    QImage m_image;
    QRect m_dirty;
    QSize m_size;
    QSize m_failedSize;
    std::unique_ptr<QRhiTexture> m_texture;
};
