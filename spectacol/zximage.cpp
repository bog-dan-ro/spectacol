/* fusetexture.cpp: fuse screen texture

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
#include "zxscreen.h"

#include <QFile>

#include <vector>

extern "C"  {
# include <fuse.h>
# include <settings.h>
# include <ui/uidisplay.h>
# include <ui/widget/widget.h>
}


inline constexpr uint32_t zxColor(uint32_t b, uint32_t g, uint32_t r)
{
    return (0xffu << 24) | ((r & 0xffu) << 16) | ((g & 0xffu) << 8) | (b & 0xffu);
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

    scaler_register_clear();
    scaler_register(SCALER_NORMAL);
    scaler_select_scaler(SCALER_NORMAL);
    display_ui_initialised = 1;
    return 0;
}

extern "C" int uidisplay_hotswap_gfx_mode()
{
  fuse_emulation_pause();

  ZxImage::instance()->rescale();

  fuse_emulation_unpause();

  return 0;
}

extern "C" int uidisplay_end()
{
    return 0;
}

extern "C" void uidisplay_putpixel( int x, int y, int colour )
{
    ZxImage::instance()->putpixel(x, y, colour);
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
    m_updates.reserve(100);
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
    {
        QMutexLocker lock(&m_syncVars);
        m_rgbImage = QImage{int(w), int(h), QImage::Format_RGBA8888};
        m_rgbImage.fill(Qt::black);
        m_rgbPixelsWidth = m_rgbImage.bytesPerLine() / 4;
        m_rgbPixels = reinterpret_cast<uint32_t *>(m_rgbImage.bits());
    }
    rescale();
}

void ZxImage::rescale()
{
    {
        QMutexLocker lock(&m_syncVars);
        m_recreate = true;
    }
    update(0, 0, m_rgbImage.width(), m_rgbImage.height());
    emit needsUpdate();
    emit geometryChanged(m_rgbImage.size());
}

QSize ZxImage::imageSize() const
{
    return m_rgbImage.size();
}

void ZxImage::update(int x, int y, int w, int h)
{
    QMutexLocker lock(&m_syncVars);

    QRhiTextureSubresourceUploadDescription desc{m_rgbImage};
    const QPoint topLeft{x, y};
    desc.setSourceTopLeft(topLeft);
    desc.setDestinationTopLeft(topLeft);
    desc.setSourceSize({w, h});
    m_updates.emplace_back(0, 0, desc);
}

void ZxImage::frameEnd()
{
    QMutexLocker lock(&m_syncVars);
    if (!m_updates.empty() || m_recreate)
        emit needsUpdate();
}

void ZxImage::saveScreen()
{
    m_savedImage = m_rgbImage;
}

void ZxImage::restoreScreen()
{
    m_rgbImage = m_savedImage;
    update(0, 0, m_rgbImage.width(), m_rgbImage.height());
}

void ZxImage::putpixel(int x, int y, int colour)
{
    QMutexLocker lock(&m_syncVars);
    Q_ASSERT(colour < 16);
    Q_ASSERT(uint32_t(x) < m_rgbImage.width() && uint32_t(y) < m_rgbImage.height());

    const auto rgbColor = palette[colour];

    if (machine_current->timex) {
        m_rgbPixels[x + m_rgbPixelsWidth * y] = rgbColor;
        m_rgbPixels[x + 1 + m_rgbPixelsWidth * y] = rgbColor;
        m_rgbPixels[x + m_rgbPixelsWidth * (y + 1)] = rgbColor;
        m_rgbPixels[x + 1 + m_rgbPixelsWidth * (y + 1)] = rgbColor;
    } else {
        m_rgbPixels[x + m_rgbPixelsWidth * y] = rgbColor;
    }
}

void ZxImage::plot8(int x, int y, libspectrum_byte data, libspectrum_byte ink, libspectrum_byte paper)
{
    QMutexLocker lock(&m_syncVars);
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
    QMutexLocker lock(&m_syncVars);
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

static QShader getShader(const QString &name)
{
    QFile f(name);
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open shader file:" << name;
        return QShader();
    }
    return QShader::fromSerialized(f.readAll());
}

void ZxRhiRenderer::initialize(QRhiCommandBuffer * /*cb*/)
{
    if (m_rhi != rhi()) {
        m_rhi = rhi();
        m_sampleCount = renderTarget()->sampleCount();
    }
    if (m_sampleCount != renderTarget()->sampleCount()) {
        m_sampleCount = renderTarget()->sampleCount();
        m_pipeline.reset();
    }
    QRhiTexture *finalTex = m_sampleCount > 1 ? resolveTexture() : colorTexture();
    if (m_textureFormat != finalTex->format()) {
        m_textureFormat = finalTex->format();
        m_pipeline.reset();
    }

    if (!m_pipeline) {
    }
}

void ZxRhiRenderer::synchronize(QQuickRhiItem *item)
{
    auto zxScreen = qobject_cast<ZxScreen *>(item);
    Q_ASSERT(zxScreen);
    if (!zxScreen)
        return;
    m_itemSize = zxScreen->size();
}

void ZxRhiRenderer::render(QRhiCommandBuffer *cb)
{
    QSize imageSize;
    auto resourceUpdates = m_rhi->nextResourceUpdateBatch();
    {
        auto zxImage = ZxImage::instance();
        QMutexLocker lock(&zxImage->mutex());
        auto &updates = zxImage->updates();
        imageSize = zxImage->rgbImage().size();
        if (!m_pipeline || m_texture->pixelSize() != imageSize) {
            recreatePipeline(cb);

            if (!m_pipeline || m_texture->pixelSize() != imageSize)
                return;

            if (!imageSize.isNull()) {
                updates.clear();
                QRhiTextureSubresourceUploadDescription desc{zxImage->rgbImage()};
                const QPoint topLeft{0, 0};
                desc.setSourceTopLeft(topLeft);
                desc.setDestinationTopLeft(topLeft);
                desc.setSourceSize(imageSize);
                updates.emplace_back(0, 0, desc);
            }
        } else if (!updates.empty()) {
            QRhiTextureUploadDescription desc;
            desc.setEntries(updates.cbegin(), updates.cend());
            resourceUpdates->uploadTexture(m_texture.get(), desc);
            updates.clear();
        }
    }

    QMatrix4x4 modelViewProjection = m_rhi->clipSpaceCorrMatrix();
    // modelViewProjection.scale(float(imageSize.width()) / m_itemSize.width(),
    //                           float(imageSize.height()) / m_itemSize.height());
    // qDebug() << float(imageSize.width()) / m_itemSize.width()
    //          << float(imageSize.height()) / m_itemSize.height();
    resourceUpdates->updateDynamicBuffer(m_ubuf.get(), 0, 64, modelViewProjection.constData());

    const QColor clearColor = QColor::fromRgbF(1, 1, 1, 1);
    cb->beginPass(renderTarget(), clearColor, { 1.0f, 0 }, resourceUpdates);

    const QSize outputSize = renderTarget()->pixelSize();
    cb->setViewport(QRhiViewport(0, 0, outputSize.width(), outputSize.height()));

    cb->setGraphicsPipeline(m_pipeline.get());
    cb->setShaderResources();
    const QRhiCommandBuffer::VertexInput vbufBinding(m_vbuf.get(), 0);
    cb->setVertexInput(0, 1, &vbufBinding);
    cb->draw(4);

    cb->endPass();
}

void ZxRhiRenderer::recreatePipeline(QRhiCommandBuffer *cb)
{
    m_pipeline.reset();
    auto zxImage = ZxImage::instance();
    auto sz = zxImage->rgbImage().size();
    if (sz.isNull())
        return;
    m_texture.reset(m_rhi->newTexture(QRhiTexture::RGBA8, sz));
    m_texture->create();

    static const float vertices[] = {
        // Triangle strip that makes 2 CCW triangles
        // position (vec4)           // texcoord
        -1.0f,  1.0f, 0.0f, 1.0f,     0.0f, 0.0f, // Top-left
        -1.0f, -1.0f, 0.0f, 1.0f,     0.0f, 1.0f, // Bottom-left
        1.0f,  1.0f, 0.0f, 1.0f,     1.0f, 0.0f, // Top-right
        1.0f, -1.0f, 0.0f, 1.0f,     1.0f, 1.0f  // Bottom-right
    };

    m_vbuf.reset(m_rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(vertices)));
    m_vbuf->create();
    auto resourceUpdates = m_rhi->nextResourceUpdateBatch();
    resourceUpdates->uploadStaticBuffer(m_vbuf.get(), vertices);

    m_ubuf.reset(m_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 64));
    m_ubuf->create();

    m_sampler.reset(m_rhi->newSampler(QRhiSampler::Nearest, QRhiSampler::Nearest, QRhiSampler::None,
                                      QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge));
    m_sampler->create();

    m_srb.reset(m_rhi->newShaderResourceBindings());
    m_srb->setBindings({
        QRhiShaderResourceBinding::sampledTexture(0, QRhiShaderResourceBinding::FragmentStage, m_texture.get(), m_sampler.get()),
        QRhiShaderResourceBinding::uniformBuffer(1, QRhiShaderResourceBinding::VertexStage, m_ubuf.get())
    });
    m_srb->create();

    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({ QRhiVertexInputBinding(6 * sizeof(float)) });
    inputLayout.setAttributes({
        QRhiVertexInputAttribute(0, 0, QRhiVertexInputAttribute::Float2, 0),
        QRhiVertexInputAttribute(0, 1, QRhiVertexInputAttribute::Float2, 4 * sizeof(float))
    });

    m_pipeline.reset(m_rhi->newGraphicsPipeline());
    m_pipeline->setShaderStages({
        { QRhiShaderStage::Vertex, getShader(QLatin1String(":/shaders/simple.vert.qsb")) },
        { QRhiShaderStage::Fragment, getShader(QLatin1String(":/shaders/simple.frag.qsb")) }
    });
    m_pipeline->setSampleCount(m_sampleCount);
    m_pipeline->setVertexInputLayout(inputLayout);
    m_pipeline->setShaderResourceBindings(m_srb.get());
    m_pipeline->setRenderPassDescriptor(renderTarget()->renderPassDescriptor());
    m_pipeline->setCullMode(QRhiGraphicsPipeline::None);
    m_pipeline->setFrontFace(QRhiGraphicsPipeline::CCW);
    m_pipeline->setTopology(QRhiGraphicsPipeline::TriangleStrip);

    m_pipeline->create();

    cb->resourceUpdate(resourceUpdates);
}
