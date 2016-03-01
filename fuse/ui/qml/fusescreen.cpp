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

#include <input.h>
#include <keyboard.h>
#include <settings.h>

#include <ui/ui.h>

#include <QtMath>
#include <QSGSimpleTextureNode>

FuseScreen::FuseScreen()
{
    setFlags(ItemHasContents | ItemIsFocusScope);
    setFocus(true);
}

bool FuseScreen::fullScreen() const
{
    return settings_current.full_screen;
}

void FuseScreen::setFullScreen(bool fullScreen)
{
    pokeEvent([this, fullScreen]{
        if (settings_current.full_screen == fullScreen)
            return;

        settings_current.full_screen = fullScreen;
    });
}

void FuseScreen::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickItem::geometryChanged(newGeometry, oldGeometry);
    QRectF itemRect = newGeometry;
    if (itemRect.width() > itemRect.height())
        itemRect.setWidth(itemRect.height() * m_aspectRatio);
    else
        itemRect.setHeight(itemRect.width() * m_aspectRatio);
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
        QSizeF spectrumSize(texture->imageSize());
        m_aspectRatio = spectrumSize.width()/spectrumSize.height();
        connect(texture, &FuseTexture::screenGeometryChanged, this, &FuseScreen::screenChanged, Qt::QueuedConnection);
        connect(texture, &FuseTexture::needsUpdate, this, &FuseScreen::update, Qt::QueuedConnection);
        connect(texture, &FuseTexture::sizeChanged, this, [this](const QSizeF &size) {
            m_aspectRatio = size.width()/size.height();
            geometryChanged(boundingRect(), boundingRect());
        }, Qt::QueuedConnection);
        geometryChanged(boundingRect(), boundingRect());
    }

    node->setSourceRect(QRect(QPoint(0, 0), texture->imageSize()));
    node->setRect(QRectF(qCeil((width() - implicitWidth()) / 2.),
                         qCeil((height() - implicitHeight()) / 2.),
                         implicitWidth(), implicitHeight()));

    node->markDirty(QSGNode::DirtyMaterial);
    return node;
}

void FuseScreen::keyPressEvent(QKeyEvent *event)
{
    g_fuseEmulator->keyPress(event);
}

void FuseScreen::keyReleaseEvent(QKeyEvent *event)
{
    g_fuseEmulator->keyRelease(event);
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
