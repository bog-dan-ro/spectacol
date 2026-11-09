/* zxscreen.cpp: QML Item representing the ZX Spectrum screen

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

#include "zxscreen.h"

#include "zximage.h"
#include "zxemulator.h"
#include "screenfilters.h"
#include "screenfilterinput.h"

#include <zxkit/core/Exception.h>
#include <rhi/qrhi.h>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QSGSimpleTextureNode>

#include <cmath>
#include <algorithm>
#include <new>
#include <stdexcept>
#include <utility>

static double fitFactor(double i, double s)
{
    if (i < s)
        return std::floor(s / i);

    return 1. / std::ceil(i / s);
}

// Where the image goes inside an output of outputSize pixels
static QRect imageRect(QSize imageSize, QSize outputSize, ZxSettings::FillMode fillMode)
{
    if (imageSize.isEmpty() || outputSize.isEmpty())
        return {};

    QSize size = outputSize;
    switch (fillMode) {
    case ZxSettings::PreserveAspect: {
        auto wf = fitFactor(imageSize.width(), outputSize.width());
        auto hf = fitFactor(imageSize.height(), outputSize.height());
        size = (QSizeF(imageSize) * (std::min)(wf, hf)).toSize();
    } break;

    case ZxSettings::PreserveAspectFit:
        size = imageSize.scaled(outputSize, Qt::KeepAspectRatio);
        break;

    case ZxSettings::Stretch:
        break;
    }

#ifdef Q_OS_ANDROID
    const int x = outputSize.width() - size.width();
    // Keep it at the top right; portrait controls go below the image.
    const int y = 0;
#else
    const int x = (outputSize.width() - size.width()) / 2;
    const int y = (outputSize.height() - size.height()) / 2;
#endif
    return { QPoint{ x, y }, size };
}

ZxScreen::ZxScreen(QQuickItem *parent) : QQuickItem(parent)
{
    setFlags(ItemHasContents | ItemIsFocusScope);
    setFocus(true);
    setAcceptedMouseButtons(Qt::LeftButton | Qt::MiddleButton | Qt::RightButton);

    auto settings = ZxEmulator::instance().settings();
    m_filterIndex = ZxEmulator::instance().selectedFilterIndex();
    connect(&ZxEmulator::instance(), &ZxEmulator::selectedFilterIndexChanged, this, [this] {
        m_filterIndex = ZxEmulator::instance().selectedFilterIndex();
        m_filterChanged = true;
        update(); // Also re-filter a paused frame, without waiting for the Engine.
    });
    m_fillMode = settings->fillMode();
    m_smoothScaling = settings->smoothScaling();
    connect(settings, &ZxSettings::fillModeChanged, this, [this](ZxSettings::FillMode fillMode) {
        m_fillMode = fillMode;
        updateImplicitSize();
        update();
    });
    connect(settings, &ZxSettings::smoothScalingChanged, this, [this](bool smoothScaling) {
        m_smoothScaling = smoothScaling;
        update();
    });
    connect(&ZxEmulator::instance(), &ZxEmulator::mouseGrabbedChanged, this,
            &ZxScreen::updateMouseGrab);

    connect(ZxImage::instance(), &ZxImage::needsUpdate, this, &ZxScreen::update,
            Qt::QueuedConnection);
    connect(
            ZxImage::instance(), &ZxImage::geometryChanged, this,
            [this](QSize size) {
                m_imageSize = size;
                updateImplicitSize();
                update();
            },
            Qt::QueuedConnection);

    // The emulator might have published a frame before the above connection was made
    m_imageSize = ZxImage::instance()->imageSize();
    updateImplicitSize();
}

bool ZxScreen::fullScreen() const
{
    return m_fullScreen;
}

void ZxScreen::setFullScreen(bool fullScreen)
{
    if (m_fullScreen == fullScreen)
        return;

    m_fullScreen = fullScreen;
    emit fullScreenChanged(fullScreen);
}

bool ZxScreen::mouseGrabSuspended() const
{
    return m_mouseGrabSuspended;
}

void ZxScreen::setMouseGrabSuspended(bool suspended)
{
    if (m_mouseGrabSuspended == suspended)
        return;

    m_mouseGrabSuspended = suspended;
    ZxEmulator::instance().setMouseGrabSuspended(suspended);
    emit mouseGrabSuspendedChanged(suspended);
}

QRectF ZxScreen::imageRect(QSize imageSize) const
{
    // compute it in device pixels, so the image is scaled by whole pixels
    const qreal dpr = window() ? window()->effectiveDevicePixelRatio() : 1.0;
    const QRect rect = ::imageRect(imageSize, (size() * dpr).toSize(), m_fillMode);
    return { QPointF(rect.topLeft()) / dpr, QSizeF(rect.size()) / dpr };
}

void ZxScreen::updateImplicitSize()
{
    // The implicit size is the size of the image on the screen, Main.qml uses it to place the on screen controls.
    const QSizeF imageSize = imageRect(m_imageSize).size();
    setImplicitSize(imageSize.width(), imageSize.height());
}

QSGNode *ZxScreen::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    auto node = static_cast<QSGSimpleTextureNode *>(oldNode);
    auto frame = ZxImage::instance()->takeFrame();
    const QRectF rect = imageRect(frame.image.size());
    if (rect.isEmpty()) {
        delete node;
        return nullptr;
    }

    // Layout and integer scaling use the native screen dimensions. Enlarging a
    // texture must not change the screen's geometry or the mouse coordinates.
    applySoftwareFilter(frame.image, frame.dirty, frame.pixelScale);

    if (window()->rendererInterface()->graphicsApi() == QSGRendererInterface::Software) {
        // The software renderer (no GPU, the offscreen platform) can't use ZxTexture's
        // RHI uploads: give it a new texture when the image changed
        if (!node)
            node = new QSGSimpleTextureNode;
        if (!node->texture() || !frame.dirty.isEmpty()) {
            QSGTexture *old = node->texture();
            node->setOwnsTexture(false);
            node->setTexture(window()->createTextureFromImage(frame.image));
            node->setOwnsTexture(true);
            delete old;
        }
    } else {
        if (!node) {
            node = new QSGSimpleTextureNode;
            node->setOwnsTexture(true);
            node->setTexture(new ZxTexture);
        }
        static_cast<ZxTexture *>(node->texture())->setFrame(std::move(frame));
    }
    node->setFiltering(m_smoothScaling ? QSGTexture::Linear : QSGTexture::Nearest);
    node->setRect(rect);
    // the texture object is the same, but its content changed
    node->markDirty(QSGNode::DirtyMaterial);
    return node;
}

void ZxScreen::applySoftwareFilter(QImage &image, QRect &dirty, QSize pixelScale)
{
    const bool changed = std::exchange(m_filterChanged, false) || m_filterPixelScale != pixelScale;
    m_filterPixelScale = pixelScale;
    try {
        if (changed) {
            m_filterPipeline.setStages(screenFilterPresets()[std::size_t(m_filterIndex)].stages);
            m_filterPipeline.releaseBuffers();
            m_filterSource = zxkit::VideoFrame(0, 0);
            m_filteredImage = {};
        }
        if (m_filterPipeline.stages().empty()) {
            if (changed)
                dirty = image.rect();
            return;
        }
        if (changed || !dirty.isEmpty() || m_filteredImage.isNull()) {
            // Pixel-art scalers must see the logical pixel grid, before Next's
            // presentation duplication. Keep genuine high-resolution samples.
            const bool pixelArt = std::ranges::any_of(m_filterPipeline.stages(), [](auto stage) {
                using F = zxkit::VideoFilter;
                return stage.filter == F::Hq || stage.filter == F::Xbrz || stage.filter == F::Scale
                        || stage.filter == F::Sai || stage.filter == F::SuperSai
                        || stage.filter == F::SuperEagle || stage.filter == F::Bilinear;
            });
            const auto scale = pixelArt ? filterInputPixelScale(image, pixelScale) : QSize(1, 1);
            const QSize inputSize(image.width() / scale.width(), image.height() / scale.height());
            const auto size = m_filterPipeline.outputSize(
                    { uint32_t(inputSize.width()), uint32_t(inputSize.height()) });
            const auto rhi = static_cast<QRhi *>(window()->rendererInterface()->getResource(
                    window(), QSGRendererInterface::RhiResource));
            if (rhi) {
                const auto maxSize = uint32_t(rhi->resourceLimit(QRhi::TextureSizeMax));
                if (size.width > maxSize || size.height > maxSize)
                    throw std::runtime_error(
                            "The selected filter exceeds the maximum screen texture size.");
            }
            if (m_filterSource.width() != uint32_t(inputSize.width())
                || m_filterSource.height() != uint32_t(inputSize.height()))
                m_filterSource = zxkit::VideoFrame(uint32_t(inputSize.width()),
                                                   uint32_t(inputSize.height()));
            for (int y = 0; y < inputSize.height(); ++y) {
                const auto source = std::span(
                        reinterpret_cast<const uint32_t *>(image.constScanLine(y * scale.height())),
                        std::size_t(image.width()));
                auto target = m_filterSource.scanline(uint32_t(y));
                for (int x = 0; x < inputSize.width(); ++x)
                    target[std::size_t(x)] = source[std::size_t(x * scale.width())];
            }
            const auto &output = m_filterPipeline.apply(m_filterSource);
            // Own the QImage pixels: queued texture uploads may retain them past
            // the next pipeline.apply(), which reuses its buffers.
            m_filteredImage =
                    QImage(reinterpret_cast<const uchar *>(output.pixels().data()),
                           int(output.width()), int(output.height()),
                           qsizetype(output.width()) * sizeof(uint32_t), QImage::Format_RGBA8888)
                            .copy();
            if (m_filteredImage.isNull())
                throw std::bad_alloc();
            dirty = m_filteredImage.rect(); // Neighborhood filters also change adjacent rows.
        }
        image = m_filteredImage;
    } catch (const std::exception &error) {
        const auto failedIndex = m_filterIndex;
        m_filterIndex = 0;
        m_filterPipeline.setStages({});
        m_filterPipeline.releaseBuffers();
        m_filterSource = zxkit::VideoFrame(0, 0);
        m_filteredImage = {};
        dirty = image.rect();
        QString reason;
        if (const auto *filterError = dynamic_cast<const zxkit::Exception *>(&error)) {
            const auto text = filterError->rawMessage();
            reason = QString::fromUtf8(text.data(), qsizetype(text.size()));
        } else if (dynamic_cast<const std::bad_alloc *>(&error)) {
            reason = tr("There is not enough memory for this screen filter.");
        } else {
            reason = QString::fromUtf8(error.what());
        }
        const auto message = tr("Unable to apply the screen filter: %1").arg(reason);
        QMetaObject::invokeMethod(
                &ZxEmulator::instance(),
                [failedIndex, message] {
                    auto &emulator = ZxEmulator::instance();
                    if (emulator.selectedFilterIndex() == failedIndex) {
                        emulator.setSelectedFilterIndex(0);
                        emulator.showMessage(message, ZxEmulator::Warning);
                    }
                },
                Qt::QueuedConnection);
    }
}

void ZxScreen::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickItem::geometryChange(newGeometry, oldGeometry);
    if (newGeometry.size() != oldGeometry.size()) {
        updateImplicitSize();
        update();
    }
}

void ZxScreen::itemChange(ItemChange change, const ItemChangeData &value)
{
    QQuickItem::itemChange(change, value);
    if (change == ItemSceneChange || change == ItemDevicePixelRatioHasChanged) {
        updateImplicitSize();
        update();
    }
}

void ZxScreen::updateMouseGrab(bool grabbed)
{
    // While the emulated Kempston mouse is grabbed, hide the cursor and send the movements
    // even when no button is pressed
    m_lastMousePosition.reset();
    m_mouseMotionRemainder = {};
    setAcceptHoverEvents(grabbed);
    if (grabbed)
        setCursor(Qt::BlankCursor);
    else
        unsetCursor();
}

void ZxScreen::sendMouseMotion(QPointF position)
{
    if (!ZxEmulator::instance().mouseGrabbed())
        return;

    if (m_lastMousePosition) {
        // move the Spectrum pointer as much as the host cursor moves over the image
        const QSizeF imageSize = imageRect(m_imageSize).size();
        if (!imageSize.isEmpty()) {
            const QPointF delta = position - *m_lastMousePosition;
            m_mouseMotionRemainder += QPointF(delta.x() * 320.0 / imageSize.width(),
                                              delta.y() * 240.0 / imageSize.height());
            const int dx = int(m_mouseMotionRemainder.x());
            const int dy = int(m_mouseMotionRemainder.y());
            if (dx || dy) {
                m_mouseMotionRemainder -= QPointF(dx, dy);
                ZxEmulator::instance().mouseMotion(dx, dy);
            }
        }
    }
    m_lastMousePosition = position;
}

void ZxScreen::mousePressEvent(QMouseEvent *event)
{
    ZxEmulator::instance().mouseButton(event->button(), true);
}

void ZxScreen::mouseMoveEvent(QMouseEvent *event)
{
    sendMouseMotion(event->position());
}

void ZxScreen::mouseReleaseEvent(QMouseEvent *event)
{
    ZxEmulator::instance().mouseButton(event->button(), false);
}

void ZxScreen::hoverEnterEvent(QHoverEvent *event)
{
    // don't turn the jump from where the cursor left the item into a movement
    m_lastMousePosition = event->position();
}

void ZxScreen::hoverMoveEvent(QHoverEvent *event)
{
    sendMouseMotion(event->position());
}
