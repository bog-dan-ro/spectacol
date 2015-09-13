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
#include "qmlui.h"

#include <debugger/breakpoint.h>
#include <fuse.h>
#include <libspectrum.h>
#include <machine.h>
#include <pokefinder/pokefinder.h>
#include <settings.h>
#include <snapshot.h>
#include <ui/ui.h>
#include <ui/scaler/scaler.h>
#include <utils.h>
#include <z80/z80.h>


#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QQmlContext>
#include <QSettings>
#include <QStandardPaths>


#include <mutex>

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

FuseEmulator::FuseEmulator(QQmlContext *ctxt, QObject *parent)
    : FuseObject(parent)
    , m_breakpointsModel(this)
    , m_disassambleModel(this)
    , m_pokeFinderModel(this)
    , m_resetPokeFinder(true)
{
    g_fuseEmulator = this;

    ctxt->setContextProperty("fuse", this);
    ctxt->setContextProperty("breakpointsModel", &m_breakpointsModel);
    ctxt->setContextProperty("disassambleModel", &m_disassambleModel);
    ctxt->setContextProperty("pokeFinderModel", &m_pokeFinderModel);

    qRegisterMetaType<ErrorLevel>("ErrorLevel");

    libspectrum_mutex_vtable_t t;
    t.create = create_mutex;
    t.lock = lock_mutex;
    t.unlock = unlock_mutex;
    t.destroy = destroy_mutex;
    libspectrum_mutex_set_vtable( &t );
    setDataPath(dataPath());
    m_debuggerActivated.store(false);
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
    pokeEvent([this, paused]() {
        if (paused)
            fuse_emulation_pause();
        else
            fuse_emulation_unpause();
        if (paused != bool(fuse_emulation_paused))
            emit pausedChanged();
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

QStringList FuseEmulator::filtersModel() const
{
    updateScalers();
    QStringList ret;
    for (int scaller : m_supportedScalers)
        ret.push_back(QLatin1String(scaler_name(scaler_type(scaller))));
    return ret;
}

int FuseEmulator::selectedFilterIndex() const
{
    auto it = std::find(m_supportedScalers.begin(), m_supportedScalers.end(), current_scaler);
    if (it != m_supportedScalers.end())
        return it - m_supportedScalers.begin();
    return -1;
}

void FuseEmulator::setSelectedFilterIndex(int selectedFilterIndex)
{
    const scaler_type scaler = scaler_type(m_supportedScalers[selectedFilterIndex]);
    pokeEvent([scaler, this]{
        scaler_select_scaler(scaler);
        callFunction([this]{
            emit selectedFilterIndexChanged();
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

QUrl FuseEmulator::snapshotsPath() const
{
    return QUrl::fromLocalFile(dataPath().toLocalFile() + QLatin1Literal("/Snapshots/"));
}

void FuseEmulator::load(const QUrl &filePath)
{
    m_loadedFileName = QFileInfo(filePath.toLocalFile()).baseName();
    pokeEvent([this, filePath]() {
        fuse_emulation_pause();
        if (utils_open_file(filePath.path().toUtf8().constData(), 1 , nullptr))
            m_loadedFileName = "";
        callFunction([this]{
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
    save(snapshotsPath().toLocalFile() + snapshotFileName(false) + QLatin1Char(' ') +
         QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") +
         QLatin1Literal(".szx"));
}

void FuseEmulator::quickLoadSnapshot()
{
    QDir dir(snapshotsPath().toLocalFile());
    const auto &list = dir.entryInfoList(QDir::Files, QDir::Time);
    if (list.size())
        load(QUrl::fromLocalFile(list.first().filePath()));
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

QString FuseEmulator::snapshotFileName(bool addExtension) const
{
    return m_loadedFileName + (addExtension ? QLatin1Literal(".szx") : QLatin1Literal(""));
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

void FuseEmulator::updateScalers() const
{
    if (!m_supportedScalers.empty())
        return;

    for (int i = SCALER_HALF; i < SCALER_NUM; ++i)
        if (scaler_is_supported(scaler_type(i)))
            m_supportedScalers.push_back(i);
}

void FuseEmulator::startFuseThread()
{
    m_fuseThread.start(QThread::HighPriority);
}
