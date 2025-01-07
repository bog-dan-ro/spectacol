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

#include <QMutex>
#include <QQuickRhiItemRenderer>
#include <rhi/qrhi.h>

#include <memory>

extern "C"  {
# include <libspectrum.h>
# include <display.h>
}

class ZxImage : public QObject
{
    Q_OBJECT
private:
    ZxImage();
    ~ZxImage();
public:
    static ZxImage *instance();

    QSize imageSize() const;
    void resize(uint32_t w, uint32_t h);
    void rescale();
    void putpixel( int x, int y, int colour );
    void plot8(int x, int y, libspectrum_byte data, libspectrum_byte ink, libspectrum_byte paper);
    void plot16(int x, int y, libspectrum_word data, libspectrum_byte ink, libspectrum_byte paper);

    void update(int x, int y, int w, int h);
    void frameEnd();
    void saveScreen();
    void restoreScreen();

    inline QMutex &mutex() { return m_syncVars; }
    inline const QImage &rgbImage() const { return m_rgbImage; }
    inline std::vector<QRhiTextureUploadEntry> &updates() {return m_updates;}

signals:
    void needsUpdate();
    void geometryChanged(QSize size);

private:
    std::vector<QRhiTextureUploadEntry> m_updates;
    QImage m_rgbImage, m_savedImage;
    uint32_t *m_rgbPixels = nullptr;
    uint32_t m_rgbPixelsWidth = 0;

    bool m_recreate = false;
    QMutex m_syncVars;
};

class ZxRhiRenderer : public QQuickRhiItemRenderer
{
protected:
    // QQuickRhiItemRenderer interface
    void initialize(QRhiCommandBuffer *cb) override;
    void synchronize(QQuickRhiItem *item) override;
    void render(QRhiCommandBuffer *cb) override;
private:
    void recreatePipeline(QRhiCommandBuffer *cb);

private:
    QRhi *m_rhi = nullptr;
    int m_sampleCount = 1;
    QRhiTexture::Format m_textureFormat = QRhiTexture::RGBA8;
    std::unique_ptr<QRhiGraphicsPipeline> m_pipeline;
    std::unique_ptr<QRhiBuffer> m_vbuf;
    std::unique_ptr<QRhiBuffer> m_ubuf;
    std::unique_ptr<QRhiTexture> m_texture;
    std::unique_ptr<QRhiSampler> m_sampler;
    std::unique_ptr<QRhiShaderResourceBindings> m_srb;
    QSizeF m_itemSize;
};

