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

#include "fusetexture.h"
#include "qmlui.h"

#include <input.h>
#include <keyboard.h>
#include <ui/ui.h>

#include <QDebug>
#include <QSGSimpleTextureNode>
#include <QtMath>

FuseScreen::FuseScreen()
{
    setFlags(ItemHasContents | ItemIsFocusScope);
    setFocus(true);
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
        QSizeF spectrumSize(FuseTexture::instance()->imageSize());
        m_aspectRatio = spectrumSize.width()/spectrumSize.height();
        connect(texture, &FuseTexture::needsUpdate, this, &FuseScreen::update, Qt::QueuedConnection);
        connect(texture, &FuseTexture::sizeChanged, this, [this](const QSizeF &size) {
            m_aspectRatio = size.width()/size.height();
            geometryChanged(boundingRect(), boundingRect());
        } );
    }
    node->setSourceRect(QRect(QPoint(0, 0), texture->imageSize()));

    node->setRect(QRectF(qCeil((width() - implicitWidth()) / 2.),
                         qCeil((height() - implicitHeight()) / 2.),
                         implicitWidth(), implicitHeight()));
    return node;
}

void FuseScreen::keyPressEvent(QKeyEvent *event)
{
    event->accept();
    if (ui_widget_level ==-1 && event->isAutoRepeat())
        return;
    input_key key = keysyms_remap(event->key() + event->modifiers());
    if (key == INPUT_KEY_NONE)
        key = keysyms_remap(event->key());
    if (key == INPUT_KEY_NONE)
        return;

    pokeEvent([key]{
        input_event_t event;
        event.type = INPUT_EVENT_KEYPRESS;
        event.types.key.spectrum_key = key;
        event.types.key.native_key = key;
        input_event(&event);
    });
}

void FuseScreen::keyReleaseEvent(QKeyEvent *event)
{
    event->accept();
    if (ui_widget_level ==-1 && event->isAutoRepeat())
        return;
    input_key key = keysyms_remap(event->key() + event->modifiers());
    if (key == INPUT_KEY_NONE)
        key = keysyms_remap(event->key());
    if (key == INPUT_KEY_NONE)
        return;

    pokeEvent([key]{
        input_event_t event;
        event.type = INPUT_EVENT_KEYRELEASE;
        event.types.key.spectrum_key = key;
        event.types.key.native_key = key;
        input_event(&event);
    });
}

void FuseScreen::mousePressEvent(QMouseEvent *event)
{
    int button = event->button();
    pokeEvent([button]{
        ui_mouse_button( button, 1 );
    });
}

void FuseScreen::mouseMoveEvent(QMouseEvent *event)
{
    Q_UNUSED(event)
}

void FuseScreen::mouseReleaseEvent(QMouseEvent *event)
{
    int button = event->button();
    pokeEvent([button]{
        ui_mouse_button( button, 0 );
    });
}
