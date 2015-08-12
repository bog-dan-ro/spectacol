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
#include <settings.h>
#include <snapshot.h>
#include <utils.h>

#include <ui/ui.h>

#include <QDebug>
#include <QtMath>
#include <QSGSimpleTextureNode>
#include <QStandardPaths>
#include <QSettings>
#include <QDir>
#include <QDateTime>

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

FuseScreen *g_fuseEmulator = nullptr;
FuseScreen::FuseScreen()
{
    g_fuseEmulator = this; // remove me when FUSE will be context aware

    qRegisterMetaType<ErrorLevel>("ErrorLevel");

    libspectrum_mutex_vtable_t t;
    t.create = create_mutex;
    t.lock = lock_mutex;
    t.unlock = unlock_mutex;
    t.destroy = destroy_mutex;
    libspectrum_mutex_set_vtable( &t );

    setFlags(ItemHasContents | ItemIsFocusScope);
    setFocus(true);
    setDataPath(dataPath());
}

bool FuseScreen::paused() const
{
    return bool(fuse_emulation_paused);
}

void FuseScreen::setPaused(bool paused)
{
    pokeEvent([this, paused]() {
        if (paused)
            fuse_emulation_pause();
        else
            fuse_emulation_unpause();
        if (paused != bool(fuse_emulation_paused))
            emit pausedChanged();
    });
}

QUrl FuseScreen::dataPath() const
{
    QSettings s;
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
# define DATA_LOCATION QStandardPaths::GenericDataLocation
#else
# define DATA_LOCATION QStandardPaths::HomeLocation
#endif
    return QUrl::fromLocalFile(s.value("dataPath", QStandardPaths::writableLocation(DATA_LOCATION) + "/Spectrum/").toString());
}

void FuseScreen::setDataPath(const QUrl &dataPath)
{
    {
        QSettings s;
        s.setValue("dataPath", dataPath.toLocalFile());
    }
    QDir dir(dataPath.toLocalFile());
    dir.mkpath("Snapshots");
    emit dataPathChanged();
}

bool FuseScreen::saveSnapshotEnabled() const
{
    return !m_loadedFileName.isEmpty();
}

QUrl FuseScreen::snapshotsPath() const
{
    return QUrl::fromLocalFile(dataPath().toLocalFile() + QLatin1Literal("/Snapshots/"));
}

bool FuseScreen::fullScreen() const
{
    return settings_current.full_screen;
}

void FuseScreen::setFullScreen(bool fullScreen)
{
    if (settings_current.full_screen == fullScreen)
        return;

    settings_current.full_screen = fullScreen;
}

void FuseScreen::load(const QUrl &filePath)
{
    m_loadedFileName = QFileInfo(filePath.toLocalFile()).baseName();
    pokeEvent([this, filePath]() {
        fuse_emulation_pause();
        if (utils_open_file(filePath.path().toUtf8().constData(), 1 , nullptr))
            m_loadedFileName = "";
        emit saveSnapshotEnabledChanged();
        display_refresh_all();
        fuse_emulation_unpause();
    });
}

void FuseScreen::save(const QUrl &filePath)
{
    pokeEvent([filePath]() {
        fuse_emulation_pause();
        snapshot_write(filePath.path().toUtf8().constData());
        fuse_emulation_unpause();
    });
}

void FuseScreen::quickSaveSnapshot()
{
    save(snapshotsPath().toLocalFile() + snapshotFileName(false) + QLatin1Char(' ') +
         QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") +
         QLatin1Literal(".szx"));
}

void FuseScreen::quickLoadSnapshot()
{
    QDir dir(snapshotsPath().toLocalFile());
    const auto &list = dir.entryInfoList(QDir::Files, QDir::Time);
    if (list.size())
        load(QUrl::fromLocalFile(list.first().filePath()));
}

QString FuseScreen::snapshotFileName(bool addExtension) const
{
    return m_loadedFileName + (addExtension ? QLatin1Literal(".szx") : QLatin1Literal(""));
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
        connect(texture, &FuseTexture::screenGeometryChanged, this, &FuseScreen::screenChanged);
        connect(texture, &FuseTexture::needsUpdate, this, &FuseScreen::update, Qt::QueuedConnection);
        connect(texture, &FuseTexture::sizeChanged, this, [this](const QSizeF &size) {
            m_aspectRatio = size.width()/size.height();
            geometryChanged(boundingRect(), boundingRect());
        });
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
