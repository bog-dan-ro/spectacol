/*
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

#include "fuseemulator.h"
#include "fusetexture.h"
#include "fusesettings.h"

#include "qmlui.h"

#include <debugger/breakpoint.h>
#include <fuse.h>
#include <libspectrum.h>
#include <machine.h>
#include <pokefinder/pokefinder.h>
#include <settings.h>
#include <snapshot.h>
#include <utils.h>
#include <z80/z80.h>

#include <QGuiApplication>
#include <QDateTime>
#include <QDir>
#include <QQmlContext>
#include <QSettings>
#include <QStandardPaths>
#include <QSemaphore>
#include <QAudioDeviceInfo>
#include <QAudioOutput>

#include <mutex>
#include <thread>

extern "C" int sound_lowlevel_init( const char *device, int *freqptr, int *stereoptr )
{
    return g_fuseEmulator->soundLowlevelInit(device, freqptr, stereoptr);
}

extern "C" void sound_lowlevel_end( void )
{
    g_fuseEmulator->soundLowlevelEnd();
}

extern "C" void sound_lowlevel_frame( libspectrum_signed_word *data, int len )
{
    g_fuseEmulator->soundLowlevelFrame(data, len);
}

FuseThread::FuseThread()
 : QThread()
{
    setPriority(QThread::HighestPriority);
}

int FuseThread::soundLowlevelInit(const char *, int *freqptr, int *stereoptr)
{
    QAudioFormat format;
    format.setCodec("audio/pcm");
    format.setChannelCount(*stereoptr ? 2 : 1);
    format.setSampleSize(16);
    format.setSampleType(QAudioFormat::SignedInt);
    format.setSampleRate(*freqptr);
    if (format == m_audioFormat && m_audioOutput)
        return 0;

    m_audioFormat = format;
    QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
    if (!info.isFormatSupported(format)) {
        m_audioOutputDevice.clear();
        return 0;
    }

    m_audioOutput.reset(new QAudioOutput(format));
    m_audioOutputDevice = m_audioOutput->start();
    m_uSleepTotal = 0;
    return 0;
}

void FuseThread::soundLowlevelFrame(libspectrum_signed_word *data, int len)
{
    if (!m_audioOutputDevice) {
        std::this_thread::sleep_for(std::chrono::microseconds(m_audioFormat.durationForBytes(len * 2)));
        return;
    }
    auto now = std::chrono::steady_clock::now();
    if (m_uSleepTotal) {
        auto sleepTime = m_uSleepTotal - std::chrono::duration_cast<std::chrono::microseconds>(now - m_startFrameTime).count();
        if (sleepTime > 0)
            std::this_thread::sleep_for(std::chrono::microseconds(sleepTime));
    }

    m_startFrameTime = now;
    m_uSleepTotal = m_audioFormat.durationForBytes(len * 2);
    while (len) {
        auto written = m_audioOutputDevice->write((const char *)data, len * 2);
        if (!written) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }
        written /= 2;
        data += written;
        len -= written;
    }
}

void FuseThread::run()
{
    int argc = 0;
    auto args = QCoreApplication::arguments();
    std::vector<QByteArray> argsVector(args.size());
    const char *argv[args.size()];
    foreach (const QString &arg, args) {
        argsVector.push_back(arg.toLocal8Bit());
        argv[argc++] = argsVector.back().constData();
    }
    fuse_main(argc, argv);
    m_audioOutput.reset();
    qApp->quit();
}

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
            callFunction([this]{ \
                emit registersChanged(); \
            }); \
        }); \
    }

FuseEmulator *g_fuseEmulator = nullptr;

static input_key toJoystickKey(QGamepadManager::GamepadButton button)
{
    switch (button) {

    case QGamepadManager::ButtonUp: return INPUT_JOYSTICK_UP;
    case QGamepadManager::ButtonDown: return INPUT_JOYSTICK_DOWN;
    case QGamepadManager::ButtonRight: return INPUT_JOYSTICK_RIGHT;
    case QGamepadManager::ButtonLeft: return INPUT_JOYSTICK_LEFT;

    case QGamepadManager::ButtonL1: return INPUT_JOYSTICK_FIRE_1;
    case QGamepadManager::ButtonR1: return INPUT_JOYSTICK_FIRE_2;

    case QGamepadManager::ButtonL2: return INPUT_JOYSTICK_FIRE_3;
    case QGamepadManager::ButtonR2: return INPUT_JOYSTICK_FIRE_4;

    case QGamepadManager::ButtonA: return INPUT_JOYSTICK_FIRE_5;
    case QGamepadManager::ButtonB: return INPUT_JOYSTICK_FIRE_6;
    case QGamepadManager::ButtonX: return INPUT_JOYSTICK_FIRE_7;
    case QGamepadManager::ButtonY: return INPUT_JOYSTICK_FIRE_8;

    case QGamepadManager::ButtonSelect: return INPUT_JOYSTICK_FIRE_9;
    case QGamepadManager::ButtonStart: return INPUT_JOYSTICK_FIRE_10;
    case QGamepadManager::ButtonL3: return INPUT_JOYSTICK_FIRE_11;
    case QGamepadManager::ButtonR3: return INPUT_JOYSTICK_FIRE_12;
    case QGamepadManager::ButtonCenter: return INPUT_JOYSTICK_FIRE_13;
    case QGamepadManager::ButtonGuide: return INPUT_JOYSTICK_FIRE_14;
    default:
        return INPUT_JOYSTICK_FIRE_15;
    }
}

FuseEmulator::FuseEmulator(QQmlContext *ctxt, QObject *parent)
    : FuseObject(parent)
    , m_breakpointsModel(this)
    , m_disassambleModel(this)
    , m_pokeFinderModel(this)
    , m_resetPokeFinder(true)
    , m_fuseSettings(new FuseSettings(this))
{
    connect(qGuiApp, &QGuiApplication::applicationStateChanged, this, [this](Qt::ApplicationState state){
        switch (state) {
        case Qt::ApplicationActive:
            if (m_applicationState != Qt::ApplicationActive) {
                m_applicationState = Qt::ApplicationActive;
                setPaused(false);
            }
            break;
        case Qt::ApplicationInactive:
            if (m_applicationState != Qt::ApplicationInactive) {
                m_applicationState = Qt::ApplicationInactive;
                setPaused(true);
            }
            break;
        default:
            break;
        }
    });
    QGamepadManager *gm = QGamepadManager::instance();
    g_fuseEmulator = this;
    {
        QSettings s;
        m_gamepadId = s.value("gamepadId", -1).toInt();
        emit gamepadIdChanged();
    }

    if (!gm->connectedGamepads().contains(m_gamepadId))
        m_gamepadId = -1;

    connect(gm, &QGamepadManager::gamepadAxisEvent, this, [this](int deviceId, QGamepadManager::GamepadAxis axis, double value){
        if (!m_processJoysticksEvents || deviceId != m_gamepadId ||
                axis == QGamepadManager::AxisInvalid)
            return;

        pokeEvent([axis, value]{
            if (fuse_emulation_paused && ui_widget_level == -1)
                return;

            input_event_t event1, event2;
            switch (axis) {
            case QGamepadManager::AxisLeftX:
            case QGamepadManager::AxisRightX:
                event1.types.joystick.button = INPUT_JOYSTICK_LEFT;
                event2.types.joystick.button = INPUT_JOYSTICK_RIGHT;
                break;

            case QGamepadManager::AxisLeftY:
            case QGamepadManager::AxisRightY:
                event1.types.joystick.button = INPUT_JOYSTICK_UP;
                event2.types.joystick.button = INPUT_JOYSTICK_DOWN;
                break;
            default:
                return;
            }
            event1.types.joystick.which = event2.types.joystick.which = 0;
            if (value <= -0.5) {
                event1.type = INPUT_EVENT_JOYSTICK_PRESS;
                event2.type = INPUT_EVENT_JOYSTICK_RELEASE;
            } else if (value >= 0.5) {
                event1.type = INPUT_EVENT_JOYSTICK_RELEASE;
                event2.type = INPUT_EVENT_JOYSTICK_PRESS;
            } else {
                event1.type = INPUT_EVENT_JOYSTICK_RELEASE;
                event2.type = INPUT_EVENT_JOYSTICK_RELEASE;
            }
            input_event(&event1);
            input_event(&event2);
        });
    });

    connect(gm, &QGamepadManager::gamepadButtonPressEvent, this, [this] (int deviceId, QGamepadManager::GamepadButton button, double value) {
        if (!m_processJoysticksEvents.load() || value != 1)
            return;

        if (fuse_emulation_paused && ui_widget_level == -1)
            return;

        switch (button) {
        case QGamepadManager::ButtonStart:
            emit showMenu();
            return;
        case QGamepadManager::ButtonL2:
            quickSaveSnapshot();
            return;
        case QGamepadManager::ButtonR2:
            quickLoadSnapshot();
            return;
        default:
            break;
        }

        if (deviceId != m_gamepadId || button == QGamepadManager::ButtonInvalid)
            return;

        pokeEvent([button] {
            input_event_t event;
            event.type = INPUT_EVENT_JOYSTICK_PRESS;
            event.types.joystick.which = 0;
            event.types.joystick.button = toJoystickKey(button);
            input_event(&event);
        });
    });

    connect(gm, &QGamepadManager::gamepadButtonReleaseEvent, this, [this] (int deviceId, QGamepadManager::GamepadButton button) {
        if (!m_processJoysticksEvents.load() || deviceId != m_gamepadId ||
                button == QGamepadManager::ButtonInvalid ||
                button == QGamepadManager::ButtonStart)
            return;

        if (fuse_emulation_paused && ui_widget_level == -1)
            return;

        pokeEvent([button] {
            input_event_t event;
            event.type = INPUT_EVENT_JOYSTICK_RELEASE;
            event.types.joystick.which = 0;
            event.types.joystick.button = toJoystickKey(button);
            input_event(&event);
        });
    });

    connect(gm, &QGamepadManager::gamepadConnected, this, [this](int deviceId){
        if (m_gamepadId == -1)
            setGamepadId(deviceId);
    });

    connect(gm, &QGamepadManager::gamepadDisconnected, this, [this](int deviceId){
        if (m_gamepadId == deviceId)
            setGamepadId(-1);
    });


    connect(&m_breakpointsModel, &BreakpointsModel::modelReset, &m_disassambleModel, &DisassambleModel::update);

    ctxt->setContextProperty("fuse", this);
    ctxt->setContextProperty("fuseSettings", m_fuseSettings.get());
    ctxt->setContextProperty("breakpointsModel", &m_breakpointsModel);
    ctxt->setContextProperty("disassambleModel", &m_disassambleModel);
    ctxt->setContextProperty("pokeFinderModel", &m_pokeFinderModel);
    ctxt->setContextProperty("onlineGamesModel", &m_onlineGamesModel);

    qRegisterMetaType<ErrorLevel>("ErrorLevel");

    libspectrum_mutex_vtable_t t;
    t.create = create_mutex;
    t.lock = lock_mutex;
    t.unlock = unlock_mutex;
    t.destroy = destroy_mutex;
    libspectrum_mutex_set_vtable( &t );
    setDataPath(dataPath());
    m_debuggerActivated.store(false);
    m_processJoysticksEvents.store(true);
    pokeEvent([]{
        settings_current.autosave_settings = 1;
    });
}

FuseEmulator::~FuseEmulator()
{
    fuse_exiting = 1;
    m_fuseThread.wait();
}

bool FuseEmulator::paused() const
{
    return bool(fuse_emulation_paused);
}

void FuseEmulator::setPaused(bool paused)
{
    pokeEvent([this, paused]{
        if (paused)
            fuse_emulation_pause();
        else
            fuse_emulation_unpause();

        callFunction([this, paused]{
            if (paused != bool(fuse_emulation_paused))
                emit pausedChanged();
        });
    });
}

QUrl FuseEmulator::dataPath() const
{
    QSettings s;
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
# define DATA_LOCATION QStandardPaths::GenericDataLocation
#else
# define DATA_LOCATION QStandardPaths::HomeLocation
#endif
    return QUrl::fromLocalFile(s.value("dataPath", QStandardPaths::writableLocation(DATA_LOCATION) + "/Spectrum/").toString());
}

void FuseEmulator::setDataPath(const QUrl &dataPath)
{
    {
        QSettings s;
        s.setValue("dataPath", dataPath.toLocalFile());
    }
    QDir dir(dataPath.toLocalFile());
    dir.mkpath("Snapshots");
    emit dataPathChanged();
}

bool FuseEmulator::saveSnapshotEnabled() const
{
    return !m_loadedFileName.isEmpty();
}

QStringList FuseEmulator::joysticksModel() const
{
    QStringList ret;
    for (int i = LIBSPECTRUM_JOYSTICK_NONE; i <= LIBSPECTRUM_JOYSTICK_FULLER; ++i )
        ret << QLatin1String(libspectrum_joystick_name(libspectrum_joystick(i)));
    return ret;
}

int FuseEmulator::selectedJoysticksIndex() const
{
    return settings_current.joystick_1_output;
}

void FuseEmulator::setSelectedJoysticksIndex(int selectedJoysticksIndex)
{
    pokeEvent([selectedJoysticksIndex, this]{
        settings_current.joystick_1_output = selectedJoysticksIndex;
        callFunction([this]{
            emit selectedJoysticksIndexChanged();
        });
    });
}

QString FuseEmulator::PC() const
{
    return formatNumber(z80.pc.w);
}

void FuseEmulator::setPC(const QString &value)
{
    setRegisterValue(z80.pc.w);
}

QString FuseEmulator::SP() const
{
    return formatNumber(z80.sp.w);
}

void FuseEmulator::setSP(const QString &value)
{
    setRegisterValue(z80.sp.w);
}

QString FuseEmulator::IX() const
{
    return formatNumber(z80.ix.w);
}

void FuseEmulator::setIX(const QString &value)
{
    setRegisterValue(z80.ix.w);
}

QString FuseEmulator::IY() const
{
    return formatNumber(z80.iy.w);
}

void FuseEmulator::setIY(const QString &value)
{
    setRegisterValue(z80.iy.w);
}

QString FuseEmulator::R() const
{
    return formatNumber(libspectrum_byte(z80.r));
}

void FuseEmulator::setR(const QString &value)
{
    setRegisterValue(z80.r);
}

QString FuseEmulator::I() const
{
    return formatNumber(z80.i);
}

void FuseEmulator::setI(const QString &value)
{
    setRegisterValue(z80.i);
}

QString FuseEmulator::R7() const
{
    return formatNumber(z80.r7);
}

void FuseEmulator::setR7(const QString &value)
{
    setRegisterValue(z80.r7);
}

QString FuseEmulator::IFF1() const
{
    return formatNumber(z80.iff1);
}

void FuseEmulator::setIFF1(const QString &value)
{
    setRegisterValue(z80.iff1);
}

QString FuseEmulator::IFF2() const
{
    return formatNumber(z80.iff2);
}

void FuseEmulator::setIFF2(const QString &value)
{
    setRegisterValue(z80.iff2);
}

QString FuseEmulator::IM() const
{
    return formatNumber(z80.im);
}

void FuseEmulator::setIM(const QString &value)
{
    setRegisterValue(z80.im);
}

QString FuseEmulator::AF() const
{
    return formatNumber(z80.af.w);
}

void FuseEmulator::setAF(const QString &value)
{
    setRegisterValue(z80.af.w);
}

QString FuseEmulator::BC() const
{
    return formatNumber(z80.bc.w);
}

void FuseEmulator::setBC(const QString &value)
{
    setRegisterValue(z80.bc.w);
}

QString FuseEmulator::DE() const
{
    return formatNumber(z80.de.w);
}

void FuseEmulator::setDE(const QString &value)
{
    setRegisterValue(z80.de.w);
}

QString FuseEmulator::HL() const
{
    return formatNumber(z80.hl.w);
}

void FuseEmulator::setHL(const QString &value)
{
    setRegisterValue(z80.hl.w);
}

QString FuseEmulator::AF_() const
{
    return formatNumber(z80.af_.w);
}

void FuseEmulator::setAF_(const QString &value)
{
    setRegisterValue(z80.af_.w);
}

QString FuseEmulator::BC_() const
{
    return formatNumber(z80.bc_.w);
}

void FuseEmulator::setBC_(const QString &value)
{
    setRegisterValue(z80.bc_.w);
}

QString FuseEmulator::DE_() const
{
    return formatNumber(z80.de_.w);
}

void FuseEmulator::setDE_(const QString &value)
{
    setRegisterValue(z80.de_.w);
}

QString FuseEmulator::HL_() const
{
    return formatNumber(z80.hl_.w);
}

void FuseEmulator::setHL_(const QString &value)
{
    setRegisterValue(z80.hl_.w);
}

void FuseEmulator::updateDebugger()
{
    m_disassambleModel.disassamble(z80.pc.w);
    callFunction([this]{
        emit registersChanged();
    });
}

int FuseEmulator::pokeFinderCount() const
{
    return pokefinder_count;
}

void FuseEmulator::setProcessJoysticksEvents(bool processJoysticksEvents)
{
   bool expect = !processJoysticksEvents;
   if (m_processJoysticksEvents.compare_exchange_strong(expect, processJoysticksEvents))
       emit processJoysticksEventsChanged();
}

void FuseEmulator::setGamepadId(int gamepadId)
{
    if (m_gamepadId == gamepadId)
        return;
    {
        QSettings s;
        s.setValue("gamepadId", gamepadId);
    }

    m_gamepadId = gamepadId;
    emit gamepadIdChanged();
}

QString FuseEmulator::saveFilePath(const QString &fileName)
{
    return dataPath().toLocalFile() + QLatin1String("Downloaded/") + fileName.left(1).toLower() +
            QLatin1Char('/') + fileName.left(3).toLower() + QLatin1Char('/') + fileName;
}

int FuseEmulator::soundLowlevelInit(const char *device, int *freqptr, int *stereoptr)
{
    return m_fuseThread.soundLowlevelInit(device, freqptr, stereoptr);
}

void FuseEmulator::soundLowlevelFrame(libspectrum_signed_word *data, int len)
{
    return m_fuseThread.soundLowlevelFrame(data, len);
}

QUrl FuseEmulator::snapshotsPath() const
{
    return QUrl::fromLocalFile(dataPath().toLocalFile() + QLatin1Literal("/Snapshots/"));
}

void FuseEmulator::load(const QUrl &filePath, bool removeOnFail)
{
    m_loadedFileName = QFileInfo(filePath.toLocalFile()).baseName();
    pokeEvent([this, removeOnFail, filePath]() {
        fuse_emulation_pause();
        if (utils_open_file(filePath.path().toUtf8().constData(), settings_current.auto_load , nullptr))
            m_loadedFileName = "";

        callFunction([this, filePath, removeOnFail]{
            if (m_loadedFileName.isEmpty() && removeOnFail) {
                QFile::remove(filePath.path());
                emit error(Warning, tr("File \"%1\" was removed").arg(filePath.path()));
            }
            emit saveSnapshotEnabledChanged();
        });
        display_refresh_all();
        fuse_emulation_unpause();
        m_resetPokeFinder = true;
    });
}

void FuseEmulator::save(const QUrl &filePath)
{
    pokeEvent([filePath]() {
        fuse_emulation_pause();
        snapshot_write(filePath.path().toUtf8().constData());
        fuse_emulation_unpause();
    });
}

void FuseEmulator::reset()
{
    pokeEvent([]() {
        machine_reset(0);
    });
}

void FuseEmulator::hardReset()
{
    pokeEvent([]() {
        machine_reset(1);
    });
}

void FuseEmulator::nmi()
{
    pokeEvent([]() {
        event_add( 0, z80_nmi_event );
    });
}

void FuseEmulator::quickSaveSnapshot()
{
    if (m_loadedFileName.isEmpty())
        return;

    const QString name = m_loadedFileName +
            QDateTime::currentDateTime().toString(".yyyy-MM-dd_hh:mm:ss") +
            QLatin1Literal(".szx");
    save(snapshotsPath().toLocalFile() + name);
    emit error(Info, tr("Snapshot saved to '%1").arg(name));
}

void FuseEmulator::quickLoadSnapshot()
{
    QDir dir(snapshotsPath().toLocalFile());
    const auto &list = dir.entryInfoList(QDir::Files, QDir::Time);
    if (list.size()) {
        load(QUrl::fromLocalFile(list.first().filePath()), true);
        emit error(Info, tr("Snapshot loaded from '%1").arg(list.first().fileName()));
    }
}

void FuseEmulator::disassamble()
{
    disassamble(z80.pc.w);
}

void FuseEmulator::disassambleFetchUp(int lines)
{
    pokeEvent([lines, this]{
        fuse_emulation_pause();
        m_disassambleModel.disassambleMore(DisassambleModel::Start, lines);
        fuse_emulation_unpause();
    });
}

void FuseEmulator::disassamble(int address, int delta, uint16_t length)
{
    pokeEvent([address, delta, length, this]{
        fuse_emulation_pause();
        m_disassambleModel.disassamble(address, delta, length);
        fuse_emulation_unpause();
    });
}

void FuseEmulator::activateDebugger()
{
    bool expected = false;
    if (!m_debuggerActivated.compare_exchange_strong(expected, true))
        return;

    fuse_emulation_pause();
    updateDebugger();
    callFunction([this]{;
        emit showDebugger();
    });
}

void FuseEmulator::deactivateDebugger(bool /*interruptable*/)
{
    bool expected = true;
    if (!m_debuggerActivated.compare_exchange_strong(expected, false))
        return;

    fuse_emulation_unpause();

//    if (interruptable)
//        callFunction([this]{
//            emit hideDebugger();
//        });
}

void FuseEmulator::pokeFinderInced()
{
    pokeEvent([this]{
        pokefinder_incremented();
        m_pokeFinderModel.update();
        callFunction([this]{
            emit pokeFinderCountChanged();
        });
    });
}

void FuseEmulator::pokeFinderDeced()
{
    pokeEvent([this]{
        pokefinder_decremented();
        m_pokeFinderModel.update();
        callFunction([this]{
            emit pokeFinderCountChanged();
        });
    });
}

void FuseEmulator::pokeFinderSearch(int value)
{
    pokeEvent([this, value]{
        pokefinder_search(value);
        m_pokeFinderModel.update();
        callFunction([this]{
            emit pokeFinderCountChanged();
        });
    });
}

void FuseEmulator::pokeFinderReset()
{
    pokeEvent([this]{
        pokefinder_clear();
        m_pokeFinderModel.update();
        callFunction([this]{
            emit pokeFinderCountChanged();
        });
    });
}

void FuseEmulator::pokeFinderResetIfNeeded()
{
    if (m_resetPokeFinder) {
        pokeFinderReset();
        m_resetPokeFinder = false;
    }

}

void FuseEmulator::pokeMemory(int address, int page, int value)
{
    if (page >= 8) {
        writebyte_internal(address, value);
    } else {
        address &= 0x3fff;
        RAM[page][address] = value;
    }
}

void FuseEmulator::debuggerTrap()
{
    pokeEvent([]{
        debugger_trap();
    });
}

void FuseEmulator::debuggerNext()
{
    pokeEvent([]{
        debugger_next();
    });
}

void FuseEmulator::debuggerRun()
{
    pokeEvent([]{
        debugger_run();
    });
}

void FuseEmulator::addBreakpoint(int address, int type)
{
    pokeEvent([this, type, address]{
        debugger_breakpoint_add_address(debugger_breakpoint_type(type), memory_source_any, 0, address, 0, DEBUGGER_BREAKPOINT_LIFE_PERMANENT, nullptr);
        m_pokeFinderModel.update();
    });
}

void FuseEmulator::addBreakpointPage(int address, int page, int type)
{
    pokeEvent([this, type, address, page]{
        debugger_breakpoint_add_address(debugger_breakpoint_type(type), memory_source_ram, page, address, 0, DEBUGGER_BREAKPOINT_LIFE_PERMANENT, nullptr);
        m_pokeFinderModel.update();
    });
}

void FuseEmulator::debuggerCommand(const QString &command)
{
    pokeEvent([this, command]{
        debugger_command_evaluate(command.toUtf8().constData());
        m_pokeFinderModel.update();
    });
}

void FuseEmulator::startFuseThread()
{
    m_fuseThread.start(QThread::HighPriority);
}
