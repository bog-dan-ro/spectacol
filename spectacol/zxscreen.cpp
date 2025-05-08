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
    m_fillMode = FuseEmulator::instance().settings()->fillMode();
    QSettings s;
    s.beginGroup(QLatin1String("Screen"));
    m_smoothScaling = s.value("smoothScaling", false).toBool();
    setWidth(320);
    setHeight(240);
    connect(this, &QQuickItem::parentChanged, this, &ZxScreen::updateFillMode);
    connect(this, &QQuickItem::windowChanged, this, &ZxScreen::updateFillMode);
    connect(ZxImage::instance(), &ZxImage::needsUpdate, this, [this] {
        if (window() && m_devicePixelRatio != window()->effectiveDevicePixelRatio()) {
            m_devicePixelRatio = window()->effectiveDevicePixelRatio();
            updateFillMode();
        }
        update();
    }, Qt::QueuedConnection);
    connect(ZxImage::instance(), &ZxImage::geometryChanged, this, [this](QSize size) {
        m_textureSize = size;
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

    if (!window() || !m_textureSize.isValid())
        return;

    if (m_parentItem != parentItem()) {
        if (m_parentItem) {
            disconnect(m_parentItem, &QQuickItem::widthChanged, this, &ZxScreen::updateFillMode);
            disconnect(m_parentItem, &QQuickItem::heightChanged, this, &ZxScreen::updateFillMode);
        }
        m_parentItem = parentItem();
        if (m_parentItem) {
            connect(m_parentItem, &QQuickItem::widthChanged, this, &ZxScreen::updateFillMode);
            connect(m_parentItem, &QQuickItem::heightChanged, this, &ZxScreen::updateFillMode);
        }
    }
    if (!m_parentItem)
        return;

    m_devicePixelRatio = window()->effectiveDevicePixelRatio();

    QSizeF parentSize = m_parentItem->size();

    switch (m_fillMode) {
    case FuseSettings::PreserveAspect: {
        auto wf = fitFactor(m_textureSize.width(), parentSize.width() * m_devicePixelRatio);
        auto hf = fitFactor(m_textureSize.height(), parentSize.height() * m_devicePixelRatio);
        auto factor = std::min(wf, hf);
        factor /= m_devicePixelRatio;
        setWidth(m_textureSize.width() * factor);
        setHeight(m_textureSize.height() * factor);
    }
    break;

    case FuseSettings::PreserveAspectFit: {
        auto wf = parentSize.width() / m_textureSize.width();
        auto hf = parentSize.height() / m_textureSize.height();
        auto factor = std::min(wf, hf);
        setWidth(m_textureSize.width() * factor);
        setHeight(m_textureSize.height() * factor);
    }
    break;

    case FuseSettings::Stretch:
        setWidth(parentSize.width());
        setHeight(parentSize.height());
        break;
    }
    update();
}

QQuickRhiItemRenderer *ZxScreen::createRenderer()
{
    return new ZxRhiRenderer();
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
