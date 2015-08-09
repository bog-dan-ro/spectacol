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

#include <fuse.h>
#include <input.h>
#include <keyboard.h>
#include <utils.h>

#include <ui/ui.h>

#include <QDebug>
#include <QSGSimpleTextureNode>
#include <QtMath>

#include <mutex>

static libspectrum_mutex_t create_mutex()
{
    return new std::mutex;
}

static void lock_mutex(libspectrum_mutex_t mutex)
{
    reinterpret_cast<std::mutex*>(mutex)->lock();
}

static void unlock_mutex(libspectrum_mutex_t mutex)
{
    reinterpret_cast<std::mutex*>(mutex)->unlock();
}

static void destroy_mutex(libspectrum_mutex_t mutex)
{
    delete reinterpret_cast<std::mutex*>(mutex);
}

FuseScreen::FuseScreen()
{
    libspectrum_mutex_vtable_t t;
    t.create = create_mutex;
    t.lock = lock_mutex;
    t.unlock = unlock_mutex;
    t.destroy = destroy_mutex;
    libspectrum_mutex_set_vtable( &t );

    setFlags(ItemHasContents | ItemIsFocusScope);
    setFocus(true);
}

bool FuseScreen::paused() const
{
    return bool(fuse_emulation_paused);
}

void FuseScreen::setPaused(bool paused)
{
    pokeEvent([this, paused]() {
        if (paused == bool(fuse_emulation_paused))
            return;
        if (paused)
            fuse_emulation_pause();
        else
            fuse_emulation_unpause();
        emit pausedChanged();
    });
}

void FuseScreen::load(QString path)
{
    pokeEvent([path]() {
        fuse_emulation_pause();
        utils_open_file( path.toUtf8().constData(), 1 , NULL );
        display_refresh_all();
        fuse_emulation_unpause();
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
