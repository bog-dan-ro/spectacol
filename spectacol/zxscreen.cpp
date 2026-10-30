/* fusecreen.cpp: QML Item representing the fuse screen

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

#include "zxscreen.h"

#include "breakpointsmodel.h"
#include "disassamblemodel.h"
#include "zximage.h"
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


static double fitFactor(double i, double s)
{
    if (i < s)
        return std::floor(s / i);

    return 1. / std::ceil(i / s);
}

ZxScreen::ZxScreen(QQuickItem *parent)
    : QQuickRhiItem(parent)
{
    setFlags(ItemHasContents | ItemIsFocusScope);
    setFocus(true);
    // ZxRhiRenderer scales the image, the item's texture is drawn 1:1, don't blur it
    setSmooth(false);
    m_fillMode = FuseEmulator::instance().settings()->fillMode();
    QSettings s;
    s.beginGroup(QLatin1String("Screen"));
    m_smoothScaling = s.value("smoothScaling", false).toBool();
    connect(ZxImage::instance(), &ZxImage::needsUpdate, this, &ZxScreen::update, Qt::QueuedConnection);
    connect(ZxImage::instance(), &ZxImage::geometryChanged, this, [this](QSize size) {
        m_imageSize = size;
        updateImplicitSize();
        update();
    }, Qt::QueuedConnection);

    // FUSE might have set up the display before the above connection was made
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
    // FUSE doesn't use it, it only saves it in its config
    pokeEvent([fullScreen]{
        settings_current.full_screen = fullScreen;
    });
}

bool ZxScreen::smoothScaling() const
{
    return m_smoothScaling;
}

void ZxScreen::setSmoothScaling(bool smoothScaling)
{
    if (m_smoothScaling == smoothScaling)
        return;

    m_smoothScaling = smoothScaling;
    QSettings s;
    s.beginGroup(QLatin1String("Screen"));
    s.setValue("smoothScaling", smoothScaling);
    emit smoothScalingChanged(smoothScaling);
    update();
}

FuseSettings::FillMode ZxScreen::fillMode() const
{
    return m_fillMode;
}

QRect ZxScreen::imageRect(QSize imageSize, QSize outputSize, FuseSettings::FillMode fillMode)
{
    if (imageSize.isEmpty() || outputSize.isEmpty())
        return {};

    QSize size = outputSize;
    switch (fillMode) {
    case FuseSettings::PreserveAspect: {
        auto wf = fitFactor(imageSize.width(), outputSize.width());
        auto hf = fitFactor(imageSize.height(), outputSize.height());
        size = (QSizeF(imageSize) * std::min(wf, hf)).toSize();
    }
    break;

    case FuseSettings::PreserveAspectFit:
        size = imageSize.scaled(outputSize, Qt::KeepAspectRatio);
        break;

    case FuseSettings::Stretch:
        break;
    }

    const int x = (outputSize.width() - size.width()) / 2;
#ifdef Q_OS_ANDROID
    // keep it at the top, in portrait mode the on screen controls go below it
    const int y = 0;
#else
    const int y = (outputSize.height() - size.height()) / 2;
#endif
    return {QPoint{x, y}, size};
}

void ZxScreen::updateFillMode()
{
    m_fillMode = FuseEmulator::instance().settings()->fillMode();
    updateImplicitSize();
    update();
}

void ZxScreen::updateImplicitSize()
{
    // The implicit size is the size of the image on the screen, Main.qml uses it to place the on screen controls.
    // Compute it the same way as QQuickRhiItem computes its texture size.
    const qreal dpr = window() ? window()->effectiveDevicePixelRatio() : 1.0;
    const QSize outputSize = (size() * dpr).toSize();
    const QSizeF imageSize = QSizeF(imageRect(m_imageSize, outputSize, m_fillMode).size()) / dpr;
    setImplicitSize(imageSize.width(), imageSize.height());
}

QQuickRhiItemRenderer *ZxScreen::createRenderer()
{
    return new ZxRhiRenderer();
}

void ZxScreen::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickRhiItem::geometryChange(newGeometry, oldGeometry);
    if (newGeometry.size() != oldGeometry.size())
        updateImplicitSize();
}

void ZxScreen::itemChange(ItemChange change, const ItemChangeData &value)
{
    QQuickRhiItem::itemChange(change, value);
    if (change == ItemSceneChange || change == ItemDevicePixelRatioHasChanged) {
        updateImplicitSize();
        update();
    }
}

void ZxScreen::mousePressEvent(QMouseEvent *event)
{
    FuseEmulator::instance().mousePress(event);
}

void ZxScreen::mouseMoveEvent(QMouseEvent *event)
{
    FuseEmulator::instance().mouseMove(event);
}

void ZxScreen::mouseReleaseEvent(QMouseEvent *event)
{
    FuseEmulator::instance().mouseRelease(event);
}
