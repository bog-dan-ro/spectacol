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
        // the pending updates point to the old image
        m_updates.clear();
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
    QMutexLocker lock(&m_syncVars);
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
    if (!m_updates.empty() || m_recreate) {
        m_recreate = false;
        emit needsUpdate();
    }
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
        m_pipeline.reset();
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
}

void ZxRhiRenderer::synchronize(QQuickRhiItem *item)
{
    auto zxScreen = qobject_cast<ZxScreen *>(item);
    Q_ASSERT(zxScreen);
    if (!zxScreen)
        return;
    m_fillMode = zxScreen->fillMode();
    if (m_smoothScaling != zxScreen->smoothScaling()) {
        m_smoothScaling = zxScreen->smoothScaling();
        m_pipeline.reset();
    }
}

void ZxRhiRenderer::render(QRhiCommandBuffer *cb)
{
    auto resourceUpdates = m_rhi->nextResourceUpdateBatch();
    QSize imageSize;
    {
        auto zxImage = ZxImage::instance();
        QMutexLocker lock(&zxImage->mutex());
        auto &updates = zxImage->updates();
        imageSize = zxImage->rgbImage().size();
        if (!m_pipeline || m_texture->pixelSize() != imageSize) {
            recreatePipeline(resourceUpdates, imageSize);
            // the new texture has no content, upload the whole image now
            if (m_pipeline)
                resourceUpdates->uploadTexture(m_texture.get(), zxImage->rgbImage());
            updates.clear();
        } else if (!updates.empty()) {
            QRhiTextureUploadDescription desc;
            desc.setEntries(updates.cbegin(), updates.cend());
            resourceUpdates->uploadTexture(m_texture.get(), desc);
            updates.clear();
        }
    }

    if (m_pipeline) {
        const QMatrix4x4 modelViewProjection = m_rhi->clipSpaceCorrMatrix();
        resourceUpdates->updateDynamicBuffer(m_uniformBuffer.get(), 0, 64, modelViewProjection.constData());
        resourceUpdates->updateDynamicBuffer(m_pushConstantsBuffer.get(), 0, 64, modelViewProjection.constData());
    }

    // always submit the batch and clear the item, even if there is nothing to draw yet
    cb->beginPass(renderTarget(), Qt::black, { 1.0f, 0 }, resourceUpdates);
    if (m_pipeline) {
        const QSize outputSize = renderTarget()->pixelSize();
        const QRect rect = ZxScreen::imageRect(imageSize, outputSize, m_fillMode);
        // QRhiViewport's origin is bottom-left
        cb->setViewport(QRhiViewport(rect.x(), outputSize.height() - rect.y() - rect.height(),
                                     rect.width(), rect.height()));
        cb->setGraphicsPipeline(m_pipeline.get());
        cb->setShaderResources();
        const QRhiCommandBuffer::VertexInput vbufBinding(m_vertexBuffer.get(), 0);
        cb->setVertexInput(0, 1, &vbufBinding);
        cb->draw(4);
    }
    cb->endPass();
}

void ZxRhiRenderer::recreatePipeline(QRhiResourceUpdateBatch *resourceUpdates, QSize imageSize)
{
    m_pipeline.reset();
    if (imageSize.isEmpty())
        return;
    m_texture.reset(m_rhi->newTexture(QRhiTexture::RGBA8, imageSize));
    m_texture->create();

    static const float vertices[] = {
        // Triangle strip that makes 2 CCW triangles
        // position (vec4)           // texcoord
        -1.0f,  1.0f, 0.0f, 1.0f,     0.0f, 0.0f, // Top-left
        -1.0f, -1.0f, 0.0f, 1.0f,     0.0f, 1.0f, // Bottom-left
        1.0f,  1.0f, 0.0f, 1.0f,     1.0f, 0.0f, // Top-right
        1.0f, -1.0f, 0.0f, 1.0f,     1.0f, 1.0f  // Bottom-right
    };

    m_vertexBuffer.reset(m_rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(vertices)));
    m_vertexBuffer->create();
    resourceUpdates->uploadStaticBuffer(m_vertexBuffer.get(), vertices);

    m_uniformBuffer.reset(m_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 64));
    m_uniformBuffer->create();

    m_pushConstantsBuffer.reset(m_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 64));
    m_pushConstantsBuffer->create();

    const auto filter = m_smoothScaling ? QRhiSampler::Linear : QRhiSampler::Nearest;
    m_sampler.reset(m_rhi->newSampler(filter, filter, QRhiSampler::None,
                                      QRhiSampler::Mirror, QRhiSampler::Mirror));
    m_sampler->create();

    m_srb.reset(m_rhi->newShaderResourceBindings());
    m_srb->setBindings({
        QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, m_uniformBuffer.get()),
        QRhiShaderResourceBinding::uniformBuffer(1, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, m_pushConstantsBuffer.get()),
        QRhiShaderResourceBinding::sampledTexture(2, QRhiShaderResourceBinding::FragmentStage, m_texture.get(), m_sampler.get()),
    });
    m_srb->create();

    m_pipeline.reset(m_rhi->newGraphicsPipeline());
    m_pipeline->setShaderStages({
        { QRhiShaderStage::Vertex, getShader(QLatin1String(":/shaders/simple.vert.qsb")) },
        { QRhiShaderStage::Fragment, getShader(QLatin1String(":/shaders/simple.frag.qsb")) }
    });

    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({ QRhiVertexInputBinding(6 * sizeof(float)) });
    inputLayout.setAttributes({
        QRhiVertexInputAttribute(0, 0, QRhiVertexInputAttribute::Float4, 0),
        QRhiVertexInputAttribute(0, 1, QRhiVertexInputAttribute::Float2, 4 * sizeof(float))
    });
    m_pipeline->setVertexInputLayout(inputLayout);
    m_pipeline->setSampleCount(m_sampleCount);

    m_pipeline->setShaderResourceBindings(m_srb.get());
    m_pipeline->setRenderPassDescriptor(renderTarget()->renderPassDescriptor());
    m_pipeline->setCullMode(QRhiGraphicsPipeline::None);
    m_pipeline->setFrontFace(QRhiGraphicsPipeline::CCW);
    m_pipeline->setTopology(QRhiGraphicsPipeline::TriangleStrip);

    m_pipeline->create();
}
