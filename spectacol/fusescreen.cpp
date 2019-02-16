/* fusecreen.cpp: QML Item representing the fuse screen

    Copyright (c) 2015, BogDan Vatra <bogdan@kde.org>

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

#include "fusescreen.h"

#include "breakpointsmodel.h"
#include "disassamblemodel.h"
#include "fusetexture.h"
#include "fuseemulator.h"
#include "qmlui.h"

#include <QtMath>
#include <QSettings>
#include <QSGSimpleTextureNode>

extern "C"  {
# include <input.h>
# include <keyboard.h>
# include <settings.h>
# include <ui/ui.h>
}

FuseScreen::FuseScreen()
{
    setFlags(ItemHasContents | ItemIsFocusScope);
    setFocus(true);
    m_fillMode = g_fuseEmulator->settings()->fillMode();
    QSettings s;
    s.beginGroup(QLatin1String("Screen"));
    m_smoothScaling = s.value("smoothScaling", false).toBool();
}

bool FuseScreen::fullScreen() const
{
    return settings_current.full_screen;
}

void FuseScreen::setFullScreen(bool fullScreen)
{
    pokeEvent([ fullScreen]{
        if (settings_current.full_screen == fullScreen)
            return;

        settings_current.full_screen = fullScreen;
    });
}

bool FuseScreen::smoothScaling() const
{
    return m_smoothScaling;
}

void FuseScreen::setSmoothScaling(bool smoothScaling)
{
    if (m_smoothScaling == smoothScaling)
        return;

    m_smoothScaling = smoothScaling;
    QSettings s;
    s.beginGroup(QLatin1String("Screen"));
    s.setValue("smoothScaling", smoothScaling);
    emit smoothScalingChanged(smoothScaling);
    FuseTexture::instance()->rescale();
}

void FuseScreen::updateFillMode()
{
    m_fillMode = g_fuseEmulator->settings()->fillMode();
    geometryChanged(boundingRect(), boundingRect());
    update();
}

static double fitFactor(double i, double s)
{
    if (i < s)
        return std::floor(s / i);

    return 1. / std::ceil(i / s);
}

void FuseScreen::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickItem::geometryChanged(newGeometry, oldGeometry);
    if (!newGeometry.width() || !newGeometry.height())
        return;

    QRectF itemRect = newGeometry;
    switch (m_fillMode) {
    case FuseSettings::PreserveAspect: {
        auto wf = fitFactor(m_imageSize.width(), newGeometry.width());
        auto hf = fitFactor(m_imageSize.height(), newGeometry.height());
        auto factor = (wf < 0 || hf < 0) ? std::max(wf, hf) : std::min(wf, hf);
        itemRect.setWidth(m_imageSize.width() * factor);
        itemRect.setHeight(m_imageSize.height() * factor);
    }
        break;

    case FuseSettings::PreserveAspectFit:
        if (itemRect.width() > itemRect.height())
            itemRect.setWidth(itemRect.height() * m_aspectRatio);
        else
            itemRect.setHeight(itemRect.width() / m_aspectRatio);
        break;

    case FuseSettings::Stretch:
        break;
    }
    setImplicitSize(itemRect.width(), itemRect.height());
}

QSGNode *FuseScreen::updatePaintNode(QSGNode *n, QQuickItem::UpdatePaintNodeData *)
{
    FuseTexture *texture =  FuseTexture::instance();
    QSGSimpleTextureNode *node = static_cast<QSGSimpleTextureNode *>(n);
    if (!node) {
        node = new QSGSimpleTextureNode;
        node->setTexture(texture);
        node->setOwnsTexture(false);
        QSizeF spectrumSize(m_imageSize = texture->imageSize());
        m_aspectRatio = spectrumSize.width()/spectrumSize.height();
        connect(texture, &FuseTexture::screenGeometryChanged, this, &FuseScreen::screenChanged, Qt::QueuedConnection);
        connect(texture, &FuseTexture::needsUpdate, this, &FuseScreen::update, Qt::QueuedConnection);
        connect(texture, &FuseTexture::sizeChanged, this, [this](const QSizeF &size) {
            m_imageSize = size.toSize();
            m_aspectRatio = size.width()/size.height();
            geometryChanged(boundingRect(), boundingRect());
        }, Qt::QueuedConnection);
        geometryChanged(boundingRect(), boundingRect());
    }
    texture->setFiltering(m_smoothScaling ? QSGTexture::Linear : QSGTexture::Nearest);
    if (node->filtering() != texture->filtering()) {
        node->setFiltering(texture->filtering());
        texture->rescale();
    }
    node->setSourceRect(QRect(QPoint(0, 0), m_imageSize));
    node->setRect(QRectF(std::ceil((width() - implicitWidth()) / 2.),
#ifdef Q_OS_ANDROID
                         0,
#else
                         std::ceil((height() - implicitHeight()) / 2.),
#endif
                         implicitWidth(),
                         implicitHeight()));

    node->markDirty(QSGNode::DirtyMaterial);
    return node;
}

void FuseScreen::mousePressEvent(QMouseEvent *event)
{
    g_fuseEmulator->mousePress(event);
}

void FuseScreen::mouseMoveEvent(QMouseEvent *event)
{
    g_fuseEmulator->mouseMove(event);
}

void FuseScreen::mouseReleaseEvent(QMouseEvent *event)
{
    g_fuseEmulator->mouseRelease(event);
}
