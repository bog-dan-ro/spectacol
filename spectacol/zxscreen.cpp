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

ZxScreen::ZxScreen()
{
    setFlags(ItemHasContents | ItemIsFocusScope);
    setFocus(true);
    m_fillMode = FuseEmulator::instance().settings()->fillMode();
    QSettings s;
    s.beginGroup(QLatin1String("Screen"));
    m_smoothScaling = s.value("smoothScaling", false).toBool();
    setWidth(384);
    setHeight(288);
    connect(ZxImage::instance(), &ZxImage::needsUpdate, this, &ZxScreen::update, Qt::QueuedConnection);
    connect(ZxImage::instance(), &ZxImage::geometryChanged, this, [this](QSize size) {
        setWidth(size.width());
        setHeight(size.height());
        updateFillMode();
    }, Qt::QueuedConnection);

    updateFillMode();
}

bool ZxScreen::fullScreen() const
{
    return settings_current.full_screen;
}

void ZxScreen::setFullScreen(bool fullScreen)
{
    pokeEvent([ fullScreen]{
        if (settings_current.full_screen == fullScreen)
            return;

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
    setSmoothScaling(m_smoothScaling);
    emit smoothScalingChanged(smoothScaling);
    update();
}

void ZxScreen::updateFillMode()
{
    m_fillMode = FuseEmulator::instance().settings()->fillMode();
    geometryChanged(boundingRect(), boundingRect());
    update();
}

QQuickRhiItemRenderer *ZxScreen::createRenderer()
{
    return new ZxRhiRenderer();
}

static double fitFactor(double i, double s)
{
    if (i < s)
        return std::floor(s / i);

    return 1. / std::ceil(i / s);
}

void ZxScreen::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickItem::geometryChange(newGeometry, oldGeometry);
    if (!newGeometry.width() || !newGeometry.height())
        return;

    QRectF itemRect = newGeometry;
    switch (m_fillMode) {
    case FuseSettings::PreserveAspect: {
        auto wf = fitFactor(ZxImage::instance()->imageSize().width(), newGeometry.width());
        auto hf = fitFactor(ZxImage::instance()->imageSize().height(), newGeometry.height());
        auto factor = (wf < 0 || hf < 0) ? std::max(wf, hf) : std::min(wf, hf);
        itemRect.setWidth(ZxImage::instance()->imageSize().width() * factor);
        itemRect.setHeight(ZxImage::instance()->imageSize().height() * factor);
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
