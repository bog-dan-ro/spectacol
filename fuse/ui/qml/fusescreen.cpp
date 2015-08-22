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
#include "qmlui.h"

#include <fuse.h>
#include <input.h>
#include <keyboard.h>
#include <machine.h>
#include <settings.h>
#include <snapshot.h>
#include <utils.h>
#include <z80/z80.h>

#include <ui/ui.h>
#include <ui/scaler/scaler.h>

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

#define setRegisterValue(reg) \
    bool ok; \
    int val = value.toInt(&ok); \
    if (!ok) \
        val = value.toInt(&ok, 16); \
    if (!ok) { \
        emit error(Warning, tr("Value \"%1\" is not a number").arg(value)); \
        return; \
    } \
    if (fuse_emulation_paused || debugger_mode == DEBUGGER_MODE_HALTED) { \
        reg = val; \
        emit registersChanged(); \
    } else { \
        pokeEvent([this, val]{ \
            reg = val; \
            emit registersChanged(); \
        }); \
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
    m_disassambleModel = new DisassambleModel(this);
    m_breakpointsModel = new BreakpointsModel(this);
}

FuseScreen::~FuseScreen()
{
    delete m_disassambleModel;
    delete m_breakpointsModel;
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

QStringList FuseScreen::filtersModel() const
{
    updateScalers();
    QStringList ret;
    for (int scaller : m_supportedScalers)
        ret.push_back(QLatin1String(scaler_name(scaler_type(scaller))));
    return ret;
}

int FuseScreen::selectedFilterIndex() const
{
    auto it = std::find(m_supportedScalers.begin(), m_supportedScalers.end(), current_scaler);
    if (it != m_supportedScalers.end())
        return it - m_supportedScalers.begin();
    return -1;
}

void FuseScreen::setSelectedFilterIndex(int selectedFilterIndex)
{
    const scaler_type scaler = scaler_type(m_supportedScalers[selectedFilterIndex]);
    pokeEvent([scaler, this]{
        scaler_select_scaler(scaler);
        emit selectedFilterIndexChanged();
    });
}

QString FuseScreen::PC() const
{
    return formatNumber(z80.pc.w);
}

void FuseScreen::setPC(const QString &value)
{
    setRegisterValue(z80.pc.w);
}

QString FuseScreen::SP() const
{
    return formatNumber(z80.sp.w);
}

void FuseScreen::setSP(const QString &value)
{
    setRegisterValue(z80.sp.w);
}

QString FuseScreen::IX() const
{
    return formatNumber(z80.ix.w);
}

void FuseScreen::setIX(const QString &value)
{
    setRegisterValue(z80.ix.w);
}

QString FuseScreen::IY() const
{
    return formatNumber(z80.iy.w);
}

void FuseScreen::setIY(const QString &value)
{
    setRegisterValue(z80.iy.w);
}

QString FuseScreen::R() const
{
    return formatNumber(libspectrum_byte(z80.r));
}

void FuseScreen::setR(const QString &value)
{
    setRegisterValue(z80.r);
}

QString FuseScreen::I() const
{
    return formatNumber(z80.i);
}

void FuseScreen::setI(const QString &value)
{
    setRegisterValue(z80.i);
}

QString FuseScreen::R7() const
{
    return formatNumber(z80.r7);
}

void FuseScreen::setR7(const QString &value)
{
    setRegisterValue(z80.r7);
}

QString FuseScreen::IFF1() const
{
    return formatNumber(z80.iff1);
}

void FuseScreen::setIFF1(const QString &value)
{
    setRegisterValue(z80.iff1);
}

QString FuseScreen::IFF2() const
{
    return formatNumber(z80.iff2);
}

void FuseScreen::setIFF2(const QString &value)
{
    setRegisterValue(z80.iff2);
}

QString FuseScreen::IM() const
{
    return formatNumber(z80.im);
}

void FuseScreen::setIM(const QString &value)
{
    setRegisterValue(z80.im);
}

QString FuseScreen::AF() const
{
    return formatNumber(z80.af.w);
}

void FuseScreen::setAF(const QString &value)
{
    setRegisterValue(z80.af.w);
}

QString FuseScreen::BC() const
{
    return formatNumber(z80.bc.w);
}

void FuseScreen::setBC(const QString &value)
{
    setRegisterValue(z80.bc.w);
}

QString FuseScreen::DE() const
{
    return formatNumber(z80.de.w);
}

void FuseScreen::setDE(const QString &value)
{
    setRegisterValue(z80.de.w);
}

QString FuseScreen::HL() const
{
    return formatNumber(z80.hl.w);
}

void FuseScreen::setHL(const QString &value)
{
    setRegisterValue(z80.hl.w);
}

QString FuseScreen::AF_() const
{
    return formatNumber(z80.af_.w);
}

void FuseScreen::setAF_(const QString &value)
{
    setRegisterValue(z80.af_.w);
}

QString FuseScreen::BC_() const
{
    return formatNumber(z80.bc_.w);
}

void FuseScreen::setBC_(const QString &value)
{
    setRegisterValue(z80.bc_.w);
}

QString FuseScreen::DE_() const
{
    return formatNumber(z80.de_.w);
}

void FuseScreen::setDE_(const QString &value)
{
    setRegisterValue(z80.de_.w);
}

QString FuseScreen::HL_() const
{
    return formatNumber(z80.hl_.w);
}

void FuseScreen::setHL_(const QString &value)
{
    setRegisterValue(z80.hl_.w);
}

QAbstractItemModel *FuseScreen::disassambleModel() const
{
    return m_disassambleModel;
}

QAbstractItemModel *FuseScreen::breakpointsModel() const
{
    return m_breakpointsModel;
}

//QStringList FuseScreen::disassemble() const
//{
//    QStringList ret;
//    libspectrum_word address = z80.pc.w;
//    char buff[100];
//    size_t len;
//    for (int i = 0; i< 0xffff; i++) {
//        debugger_disassemble(buff, sizeof(buff), &len, address);
//        ret.push_back(formatNumber(address) + QLatin1Literal(" ") + buff);
//        qDebug() << len << ret.last();
//        address += len;
//    }
//    return ret;
//}

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

void FuseScreen::reset()
{
    pokeEvent([]() {
        machine_reset(0);
    });
}

void FuseScreen::hardReset()
{
    pokeEvent([]() {
        machine_reset(1);
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

void FuseScreen::disassamble()
{
    disassamble(z80.pc.w);
}

void FuseScreen::disassamble(uint16_t address, uint16_t delta, uint16_t length)
{
    pokeEvent([address, delta, length, this]{
        fuse_emulation_pause();
        m_disassambleModel->disassamble(address, delta, length);
        fuse_emulation_unpause();
    });
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

void FuseScreen::updateScalers() const
{
    if (!m_supportedScalers.empty())
        return;

    for (int i = SCALER_HALF; i < SCALER_NUM; ++i)
        if (scaler_is_supported(scaler_type(i)))
            m_supportedScalers.push_back(i);
}
