/*
    Copyright (c) 2015-2026, BogDan Vatra <bogdan@kde.org>

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

#include "zxemulator.h"
#include "zxrecording.h"
#include "zxsettings.h"
#include "qmlui.h"
#include "zximage.h"

#include <zxkit/input/Joystick.h>
#include <zxkit/input/Mouse.h>
#include <zxkit/media/MediaPreview.h>

#include <QAudioDevice>
#include <QAudioSink>
#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QMediaDevices>
#include <QQmlContext>
#include <QQmlEngine>
#include <QSettings>
#include <QStandardPaths>
#include <QTimer>

#include <cctype>
#include <optional>
#include <vector>

using namespace Qt::StringLiterals;

#ifdef Q_OS_ANDROID
# include <QCoreApplication>
#endif

namespace {

static constexpr std::chrono::microseconds kAudioBufferDuration{80'000};

struct MappedZxKeys {
    std::optional<zxkit::ZxKey> primary;
    std::optional<zxkit::ZxKey> modifier;
};

MappedZxKeys mapQtKeyToZx(int qtKey, int modifiers)
{
    using zxkit::ZxKey;

    if (qtKey >= Qt::Key_A && qtKey <= Qt::Key_Z) {
        static constexpr ZxKey kLetters[26] = {
            ZxKey::A, ZxKey::B, ZxKey::C, ZxKey::D, ZxKey::E, ZxKey::F, ZxKey::G,
            ZxKey::H, ZxKey::I, ZxKey::J, ZxKey::K, ZxKey::L, ZxKey::M, ZxKey::N,
            ZxKey::O, ZxKey::P, ZxKey::Q, ZxKey::R, ZxKey::S, ZxKey::T, ZxKey::U,
            ZxKey::V, ZxKey::W, ZxKey::X, ZxKey::Y, ZxKey::Z
        };
        const auto key = kLetters[qtKey - Qt::Key_A];
        if (modifiers & Qt::ShiftModifier)
            return {key, ZxKey::CapsShift};
        return {key, std::nullopt};
    }

    if (qtKey >= Qt::Key_0 && qtKey <= Qt::Key_9) {
        static constexpr ZxKey kDigits[10] = {
            ZxKey::Num0, ZxKey::Num1, ZxKey::Num2, ZxKey::Num3, ZxKey::Num4,
            ZxKey::Num5, ZxKey::Num6, ZxKey::Num7, ZxKey::Num8, ZxKey::Num9
        };
        return {kDigits[qtKey - Qt::Key_0], std::nullopt};
    }

    switch (qtKey) {
    case Qt::Key_Return:
    case Qt::Key_Enter:      return {ZxKey::Enter, std::nullopt};
    case Qt::Key_Space:      return {ZxKey::Space, std::nullopt};
    case Qt::Key_Shift:      return {ZxKey::CapsShift, std::nullopt};
    case Qt::Key_Control:
    case Qt::Key_Alt:
    case Qt::Key_Meta:       return {ZxKey::SymbolShift, std::nullopt};
    case Qt::Key_Backspace:  return {ZxKey::Num0, ZxKey::CapsShift};
    case Qt::Key_CapsLock:   return {ZxKey::Num2, ZxKey::CapsShift};
    case Qt::Key_Escape:     return {ZxKey::Space, ZxKey::CapsShift};
    case Qt::Key_Left:       return {ZxKey::Num5, ZxKey::CapsShift};
    case Qt::Key_Down:       return {ZxKey::Num6, ZxKey::CapsShift};
    case Qt::Key_Up:         return {ZxKey::Num7, ZxKey::CapsShift};
    case Qt::Key_Right:      return {ZxKey::Num8, ZxKey::CapsShift};
    case Qt::Key_Exclam:     return {ZxKey::Num1, ZxKey::SymbolShift};
    case Qt::Key_At:         return {ZxKey::Num2, ZxKey::SymbolShift};
    case Qt::Key_NumberSign: return {ZxKey::Num3, ZxKey::SymbolShift};
    case Qt::Key_Dollar:     return {ZxKey::Num4, ZxKey::SymbolShift};
    case Qt::Key_Percent:    return {ZxKey::Num5, ZxKey::SymbolShift};
    case Qt::Key_Ampersand:  return {ZxKey::Num6, ZxKey::SymbolShift};
    case Qt::Key_Apostrophe: return {ZxKey::Num7, ZxKey::SymbolShift};
    case Qt::Key_ParenLeft:  return {ZxKey::Num8, ZxKey::SymbolShift};
    case Qt::Key_ParenRight: return {ZxKey::Num9, ZxKey::SymbolShift};
    case Qt::Key_Underscore: return {ZxKey::Num0, ZxKey::SymbolShift};
    case Qt::Key_QuoteDbl:   return {ZxKey::P, ZxKey::SymbolShift};
    case Qt::Key_Semicolon:  return {ZxKey::O, ZxKey::SymbolShift};
    case Qt::Key_Comma:      return {ZxKey::N, ZxKey::SymbolShift};
    case Qt::Key_Period:     return {ZxKey::M, ZxKey::SymbolShift};
    case Qt::Key_Plus:       return {ZxKey::K, ZxKey::SymbolShift};
    case Qt::Key_Minus:      return {ZxKey::J, ZxKey::SymbolShift};
    case Qt::Key_Asterisk:   return {ZxKey::B, ZxKey::SymbolShift};
    case Qt::Key_Slash:      return {ZxKey::V, ZxKey::SymbolShift};
    case Qt::Key_Equal:      return {ZxKey::L, ZxKey::SymbolShift};
    case Qt::Key_Colon:      return {ZxKey::Z, ZxKey::SymbolShift};
    case Qt::Key_Less:       return {ZxKey::R, ZxKey::SymbolShift};
    case Qt::Key_Greater:    return {ZxKey::T, ZxKey::SymbolShift};
    case Qt::Key_Question:   return {ZxKey::C, ZxKey::SymbolShift};
    default:                 return {std::nullopt, std::nullopt};
    }
}

uint8_t toKempstonMask(QGamepadManager::GamepadButton button)
{
    switch (button) {
    case QGamepadManager::ButtonUp:    return zxkit::JoystickButtons::Up;
    case QGamepadManager::ButtonDown:  return zxkit::JoystickButtons::Down;
    case QGamepadManager::ButtonRight: return zxkit::JoystickButtons::Right;
    case QGamepadManager::ButtonLeft:  return zxkit::JoystickButtons::Left;
    default:                           return zxkit::JoystickButtons::Fire1;
    }
}

// Peripheral ids name their types, no cast can fail
template <typename P, typename F>
void withPeripheral(zxkit::Emulator &emulator, std::string_view id, F &&fn)
{
    if (auto peripheral = emulator.findPeripheral(id))
        std::forward<F>(fn)(static_cast<P &>(peripheral->get()));
}

} // namespace

#define setRegisterValue(memberExpr) \
    bool ok = false; \
    int val = value.toInt(&ok); \
    if (!ok) \
        val = value.toInt(&ok, 16); \
    if (!ok) { \
        emit error(Warning, tr("Value \"%1\" is not a number").arg(value)); \
        return; \
    } \
    emulator().writeLock()->cpu().registers().memberExpr; \
    emit registersChanged();

ZxEmulator *ZxEmulator::create(QQmlEngine *, QJSEngine *)
{
    return &instance();
}

ZxEmulator &ZxEmulator::instance()
{
    static ZxEmulator res;
    return res;
}

ZxEmulator::ZxEmulator(QObject *parent)
    : QObject(parent)
    , m_breakpointsModel(this)
    , m_disassambleModel(this)
    , m_pokeFinderModel(this)
    , m_resetPokeFinder(true)
    , m_settings(std::make_unique<ZxSettings>(*this))
    , m_tape(new ZxTape(this))
    , m_disks(new Disks(this))
    , m_recording(new ZxRecording(this))
{
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
    QQmlEngine::setObjectOwnership(m_tape, QQmlEngine::CppOwnership);
    QQmlEngine::setObjectOwnership(m_recording, QQmlEngine::CppOwnership);

    // The Engine calls these on its thread, it holds only Weak references to them
    m_videoFrameCallback = zxkit::CallbackHandle<zxkit::VideoFrameCallback>([](const zxkit::VideoFramePtr &frame) {
        ZxImage::instance()->presentFrame(frame);
    });
    m_audioCallback = zxkit::CallbackHandle<zxkit::AudioCallback>([this](std::span<const int16_t> samples, uint32_t sampleRate, uint8_t channels) {
        writeAudioSamples(samples, sampleRate, channels);
    });
    m_pauseCallback = zxkit::CallbackHandle<zxkit::PauseCallback>([this](const zxkit::PauseEvent &event) {
        QMetaObject::invokeMethod(this, [this, event] {
            emit pausedChanged();
            if (m_debuggerActivated)
                updateDebugger(); // e.g. a step over reached its instruction
            else if (event.reason != zxkit::PauseReason::UserRequest)
                activateDebugger(); // a breakpoint or a watchpoint
        }, Qt::QueuedConnection);
    });
    m_resumeCallback = zxkit::CallbackHandle<zxkit::ResumeCallback>([this] {
        QMetaObject::invokeMethod(this, &ZxEmulator::pausedChanged, Qt::QueuedConnection);
    });
    m_logCallback = zxkit::CallbackHandle<zxkit::LogCallback>([this](zxkit::LogLevel level, std::string_view msg) {
        ErrorLevel errLvl = Info;
        if (level == zxkit::LogLevel::Warning)
            errLvl = Warning;
        else if (level >= zxkit::LogLevel::Error)
            errLvl = Error;
        showMessage(QString::fromUtf8(msg.data(), qsizetype(msg.size())), errLvl);
    });

    zxkit::EmulatorConfig cfg;
    cfg.machineId = "48k";
    cfg.audioSampleRate = 48'000;
    cfg.romResolver = [](std::string_view romName, std::size_t /*expectedSize*/) -> std::vector<uint8_t> {
        QFile f(u":/roms/"_s + QString::fromUtf8(romName.data(), qsizetype(romName.size())));
        if (!f.open(QIODevice::ReadOnly))
            return {};
        const QByteArray bytes = f.readAll();
        return std::vector<uint8_t>(bytes.begin(), bytes.end());
    };

    zxkit::EngineConfig engineConfig;
    engineConfig.callbacks = {
        .videoFrame = m_videoFrameCallback.weak(),
        .audio = m_audioCallback.weak(),
        .paused = m_pauseCallback.weak(),
        .resumed = m_resumeCallback.weak(),
        .log = m_logCallback.weak(),
    };
    m_engine = std::make_unique<zxkit::Engine>(std::move(cfg), std::move(engineConfig));
    emulator().writeLock()->setRewindEnabled(true, 300);
    m_settings->applyToEmulator();

    connect(qGuiApp, &QCoreApplication::aboutToQuit, this, &ZxEmulator::stopEngine);
    connect(qGuiApp, &QGuiApplication::applicationStateChanged, this, [this](Qt::ApplicationState state) {
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
    {
        QSettings s;
        m_gamepadId = s.value("gamepadId", -1).toInt();
        emit gamepadIdChanged();
        if (s.value("first_start", true).toBool())
            QMetaObject::invokeMethod(this, &ZxEmulator::showWelcome, Qt::QueuedConnection);
        s.setValue("first_start", false);
    }

    if (!gm->connectedGamepads().contains(m_gamepadId))
        m_gamepadId = -1;

    connect(gm, &QGamepadManager::gamepadAxisEvent, this, [this](int deviceId, QGamepadManager::GamepadAxis axis, double value) {
        if (!m_processInputEvents || deviceId != m_gamepadId || axis == QGamepadManager::AxisInvalid)
            return;
        gamepadAxisEvent(axis, value);
    });

    connect(gm, &QGamepadManager::gamepadButtonPressEvent, this, [this](int deviceId, QGamepadManager::GamepadButton button, double value) {
        if (!m_processInputEvents.load() || value != 1)
            return;

        switch (button) {
        case QGamepadManager::ButtonStart:
            setGamepadId(deviceId);
            m_settings->setHasStartButton(true);
            if (m_showControlsIcons)
                emit showControlsIconsChanged(m_showControlsIcons = false);
            emit showMenu();
            return;
        default:
            break;
        }

        if (deviceId != m_gamepadId || button == QGamepadManager::ButtonInvalid)
            return;

        gamepadButtonPressEvent(button);
    });

    connect(gm, &QGamepadManager::gamepadButtonReleaseEvent, this, [this](int deviceId, QGamepadManager::GamepadButton button) {
        if (deviceId != m_gamepadId || button == QGamepadManager::ButtonInvalid || button == QGamepadManager::ButtonStart)
            return;

        auto action = m_settings->gamepadAction(button);
        switch (action) {
        case ZxSettings::ToggleKeyboard:
            if (m_paused) {
                showMessage(tr("Spectacol is paused"));
                return;
            }
            if (m_showControlsIcons)
                emit showControlsIconsChanged(m_showControlsIcons = false);
            QTimer::singleShot(0, this, [this] { emit toggleOnScreenControls(Keyboard48K); });
            return;
        case ZxSettings::ToggleCursorJoystick:
            if (m_paused) {
                showMessage(tr("Spectacol is paused"));
                return;
            }
            if (m_showControlsIcons)
                emit showControlsIconsChanged(m_showControlsIcons = false);
            QTimer::singleShot(0, this, [this] { emit toggleOnScreenControls(CursorJoystick); });
            return;
        }

        if (!m_processInputEvents.load())
            return;

        gamepadButtonReleaseEvent(button);
    });

    connect(gm, &QGamepadManager::gamepadConnected, this, [this](int deviceId) {
        if (m_gamepadId == -1) {
            setGamepadId(deviceId);
            m_settings->setHasStartButton(false);
        }
    });

    connect(gm, &QGamepadManager::gamepadDisconnected, this, [this](int deviceId) {
        if (m_gamepadId == deviceId) {
            if (QGamepadManager::instance()->connectedGamepads().size())
                setGamepadId(QGamepadManager::instance()->connectedGamepads().first());
            else
                setGamepadId(-1);

            if (m_touchscreen && !m_showControlsIcons)
                emit showControlsIconsChanged(m_showControlsIcons = true);
        }
    });

    connect(&m_breakpointsModel, &BreakpointsModel::modelReset, &m_disassambleModel, &DisassambleModel::update);

    qRegisterMetaType<ErrorLevel>("ErrorLevel");

    setDataPath(dataPath());
    m_debuggerActivated.store(false);
    m_processInputEvents.store(true);
    m_deadZone = m_settings->deadZone();
    connect(m_settings.get(), &ZxSettings::deadZoneChanged, this, [this](qreal deadZone) {
        m_deadZone = deadZone;
    });

    // If a file was passed on the command line, load it automatically
    const auto args = QCoreApplication::arguments();
    if (args.size() > 1 && QFile::exists(args[1])) {
        load(args[1], false);
    }
}

ZxEmulator::~ZxEmulator()
{
    stopEngine();
    delete m_tape;
    delete m_recording;
}

void ZxEmulator::writeAudioSamples(std::span<const int16_t> samples, uint32_t sampleRate, uint8_t channels)
{
    // Faster or slower than real time the samples would come faster or slower than
    // the audio device plays them: like FUSE, play the sound only at 100%
    if (!m_realTimeAudio.load(std::memory_order_relaxed) || !m_settings || !m_settings->soundEnabled() || samples.empty())
        return;

    // Engine thread: copy the samples and play them on the GUI thread
    QByteArray pcmBytes(QByteArrayView(std::as_bytes(samples)));
    QMetaObject::invokeMethod(this, [this, pcmBytes = std::move(pcmBytes), sampleRate, channels] {
        if (!m_audioOutput || m_audioFormat.sampleRate() != int(sampleRate) || m_audioFormat.channelCount() != int(channels)) {
            m_audioOutputDevice = nullptr;
            m_audioOutput.reset();

            const auto device = QMediaDevices::defaultAudioOutput();
            if (device.isNull())
                return;

            QAudioFormat fmt;
            fmt.setSampleFormat(QAudioFormat::Int16);
            fmt.setSampleRate(int(sampleRate));
            fmt.setChannelCount(int(channels));
            if (!device.isFormatSupported(fmt))
                return;

            m_audioFormat = fmt;
            m_audioOutput = std::make_unique<QAudioSink>(device, fmt);
            m_audioOutput->setBufferSize(fmt.bytesForDuration(kAudioBufferDuration.count()));
            m_audioOutputDevice = m_audioOutput->start();
        }
        if (m_audioOutputDevice)
            m_audioOutputDevice->write(pcmBytes);
    }, Qt::QueuedConnection);
}

bool ZxEmulator::paused() const
{
    return m_engine ? m_engine->isPaused() : m_paused;
}

// Pausing nests: every setPaused(true) must be
// balanced by a setPaused(false), the emulation runs again after the last one.
// E.g. the debugger keeps it paused while the menu that opened it closes.
void ZxEmulator::setPaused(bool paused)
{
    if (paused)
        ++m_pauseCount;
    else if (m_pauseCount > 0)
        --m_pauseCount;
    else
        return;
    if (m_engine)
        m_engine->setPaused(m_pauseCount > 0);
    emit pausedChanged();
}

QString ZxEmulator::dataPath() const
{
    static QString path;
    if (!path.isEmpty())
        return path;

    QSettings s;
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    path = s.value("dataPath", QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation).last() + u"/Spectacol/"_s).toString();
#else
    path = s.value("dataPath", QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + u"/Spectacol/"_s).toString();
#endif
    return path;
}

void ZxEmulator::setDataPath(const QString &dataPath)
{
    {
        QSettings s;
        s.setValue("dataPath", dataPath);
    }
    QDir dir(dataPath);
    dir.mkpath(u"Snapshots"_s);
    dir.mkpath(u"Recordings"_s);
    dir.mkpath(u"Saves/Snapshots"_s);
    emit dataPathChanged();
}

bool ZxEmulator::saveSnapshotEnabled() const
{
    return !m_loadedFileName.isEmpty();
}

QStringList ZxEmulator::filtersModel() const
{
    return {
        u"Normal"_s,
        u"CRT Scanlines"_s,
        u"2xSaI"_s,
        u"HQ2x"_s
    };
}

int ZxEmulator::selectedFilterIndex() const
{
    return m_selectedFilterIndex;
}

void ZxEmulator::setSelectedFilterIndex(int selectedFilterIndex)
{
    if (m_selectedFilterIndex == selectedFilterIndex)
        return;
    m_selectedFilterIndex = selectedFilterIndex;
    emit selectedFilterIndexChanged();
}

QStringList ZxEmulator::joysticksModel() const
{
    return {
        u"None"_s,
        u"Cursor"_s,
        u"Kempston"_s,
        u"Sinclair 1"_s,
        u"Sinclair 2"_s,
        u"Timex 1"_s,
        u"Timex 2"_s,
        u"Fuller"_s
    };
}

int ZxEmulator::selectedJoysticksIndex() const
{
    return m_selectedJoystickIndex;
}

void ZxEmulator::setSelectedJoysticksIndex(int selectedJoysticksIndex)
{
    m_selectedJoystickIndex = selectedJoysticksIndex;
    m_settings->setKempstonJoystick(selectedJoysticksIndex == 2);
    m_settings->setInterface1(selectedJoysticksIndex == 3);
    m_settings->setInterface2(selectedJoysticksIndex == 4);
    emit selectedJoysticksIndexChanged();
}

QString ZxEmulator::PC() const { return formatNumber(registers().PC); }
void ZxEmulator::setPC(const QString &value) { setRegisterValue(PC = uint16_t(val)); }

QString ZxEmulator::SP() const { return formatNumber(registers().SP); }
void ZxEmulator::setSP(const QString &value) { setRegisterValue(SP = uint16_t(val)); }

QString ZxEmulator::IX() const { return formatNumber(registers().IX); }
void ZxEmulator::setIX(const QString &value) { setRegisterValue(IX = uint16_t(val)); }

QString ZxEmulator::IY() const { return formatNumber(registers().IY); }
void ZxEmulator::setIY(const QString &value) { setRegisterValue(IY = uint16_t(val)); }

QString ZxEmulator::R() const { return formatNumber(registers().R); }
void ZxEmulator::setR(const QString &value) { setRegisterValue(R = uint8_t(val & 0x7F)); }

QString ZxEmulator::I() const { return formatNumber(registers().I); }
void ZxEmulator::setI(const QString &value) { setRegisterValue(I = uint8_t(val)); }

QString ZxEmulator::R7() const { return formatNumber(registers().R7); }
void ZxEmulator::setR7(const QString &value) { setRegisterValue(R7 = uint8_t(val & 0x80)); }

QString ZxEmulator::IFF1() const { return formatNumber(registers().IFF1); }
void ZxEmulator::setIFF1(const QString &value) { setRegisterValue(IFF1 = uint8_t(val)); }

QString ZxEmulator::IFF2() const { return formatNumber(registers().IFF2); }
void ZxEmulator::setIFF2(const QString &value) { setRegisterValue(IFF2 = uint8_t(val)); }

QString ZxEmulator::IM() const { return formatNumber(registers().IM); }
void ZxEmulator::setIM(const QString &value) { setRegisterValue(IM = uint8_t(val & 0x03)); }

QString ZxEmulator::AF() const { return formatNumber(registers().AF); }
void ZxEmulator::setAF(const QString &value) { setRegisterValue(AF = uint16_t(val)); }

QString ZxEmulator::BC() const { return formatNumber(registers().BC); }
void ZxEmulator::setBC(const QString &value) { setRegisterValue(BC = uint16_t(val)); }

QString ZxEmulator::DE() const { return formatNumber(registers().DE); }
void ZxEmulator::setDE(const QString &value) { setRegisterValue(DE = uint16_t(val)); }

QString ZxEmulator::HL() const { return formatNumber(registers().HL); }
void ZxEmulator::setHL(const QString &value) { setRegisterValue(HL = uint16_t(val)); }

QString ZxEmulator::AF_() const { return formatNumber(registers().AF_); }
void ZxEmulator::setAF_(const QString &value) { setRegisterValue(AF_ = uint16_t(val)); }

QString ZxEmulator::BC_() const { return formatNumber(registers().BC_); }
void ZxEmulator::setBC_(const QString &value) { setRegisterValue(BC_ = uint16_t(val)); }

QString ZxEmulator::DE_() const { return formatNumber(registers().DE_); }
void ZxEmulator::setDE_(const QString &value) { setRegisterValue(DE_ = uint16_t(val)); }

QString ZxEmulator::HL_() const { return formatNumber(registers().HL_); }
void ZxEmulator::setHL_(const QString &value) { setRegisterValue(HL_ = uint16_t(val)); }

zxkit::Z80Registers ZxEmulator::registers() const
{
    return emulator().readLock()->registers();
}

void ZxEmulator::updateDebugger()
{
    m_disassambleModel.disassamble(registers().PC);
    emit registersChanged();
}

int ZxEmulator::pokeFinderCount() const
{
    return int(emulator().readLock()->debugger().pokeFinder().count());
}

bool ZxEmulator::loadMediaFile(const QString &filePath)
{
    zxkit::MediaFile file;
    try {
        // The slow part (reading, unzipping) runs before taking the lock
        file = zxkit::MediaFile::read(filePath.toStdString());
    } catch (const std::exception &e) {
        showMessage(QString::fromUtf8(e.what()), Error);
        return false;
    }
    const bool loaded = withEmulator([&file](zxkit::Emulator &emulator) {
        emulator.loadMedia(file);
    });
    if (loaded)
        m_engine->refreshFrame();
    return loaded;
}

bool ZxEmulator::saveMediaFile(const QString &filePath)
{
    std::vector<uint8_t> bytes;
    const QString suffix = QFileInfo(filePath).suffix();
    if (!withEmulator([&bytes, &suffix](zxkit::Emulator &emulator) { bytes = emulator.saveMedia(suffix.toStdString()); }))
        return false;
    QFile f(filePath);
    if (!f.open(QIODevice::WriteOnly) || f.write(QByteArrayView(bytes).data(), qint64(bytes.size())) != qint64(bytes.size())) {
        showMessage(tr("Can't write '%1'").arg(filePath), Error);
        return false;
    }
    return true;
}

void ZxEmulator::setProcessInputEvents(bool processEvents)
{
    if (processEvents) {
        if (!m_inputEventsBlockers || --m_inputEventsBlockers)
            return;
    } else if (m_inputEventsBlockers++) {
        return;
    }
    m_processInputEvents.store(processEvents);
    emit processInputEventsChanged();
}

void ZxEmulator::setGamepadId(int gamepadId)
{
    if (m_gamepadId == gamepadId)
        return;
    {
        QSettings s;
        s.setValue("gamepadId", gamepadId);
    }

    m_gamepadId = gamepadId;
    if (m_gamepadId != -1)
        emit error(Info, tr("A new default gamepad was selected"));
    else
        emit error(Warning, tr("No gamepad was found, please connect a gamepad"));

    emit gamepadIdChanged();
}

QString ZxEmulator::saveFilePath(const QString &fileName)
{
    return dataPath() + u"/Downloaded/"_s + fileName.left(1).toLower() +
           u'/' + fileName.left(3).toLower() + u'/' + fileName;
}

void ZxEmulator::mouseButton(Qt::MouseButton button, bool pressed)
{
    if (!m_processInputEvents)
        return;

    uint8_t bit = 0;
    switch (button) {
    case Qt::RightButton:  bit = 0x01; break;
    case Qt::LeftButton:   bit = 0x02; break;
    case Qt::MiddleButton: bit = 0x04; break;
    default: return;
    }

    withPeripheral<zxkit::KempstonMousePeripheral>(*emulator().writeLock(), "kempston_mouse", [bit, pressed](auto &mouse) {
        mouse.setButton(bit, pressed);
    });
}

void ZxEmulator::mouseMotion(int dx, int dy)
{
    if (!m_processInputEvents)
        return;

    withPeripheral<zxkit::KempstonMousePeripheral>(*emulator().writeLock(), "kempston_mouse", [dx, dy](auto &mouse) {
        mouse.move(dx, dy);
    });
}

void ZxEmulator::setMouseGrabSuspended(bool suspended)
{
    setMouseGrabbed(!suspended && m_settings && m_settings->kempstonMouse());
}

void ZxEmulator::setMouseGrabbed(bool grabbed)
{
    if (m_mouseGrabbed == grabbed)
        return;
    m_mouseGrabbed = grabbed;
    emit mouseGrabbedChanged(grabbed);
}

ZxSettings *ZxEmulator::settings() const
{
    return m_settings.get();
}

void ZxEmulator::resetLoadedFile()
{
    m_loadedFileName = "";
    emit saveSnapshotEnabledChanged();
}

void ZxEmulator::quit()
{
    if (m_settings->autoSaveOnExit())
        quickSaveSnapshot();

    QCoreApplication::quit();
}

QString ZxEmulator::snapshotsPath() const
{
    return dataPath() + u"Snapshots/"_s;
}

QString ZxEmulator::recordingsPath() const
{
    return dataPath() + u"Recordings/"_s;
}

QString ZxEmulator::savesPath() const
{
    return dataPath() + u"Saves/"_s;
}

QString ZxEmulator::recordingFilePath() const
{
    return recordingsPath() + (m_loadedFileName.isEmpty() ? u"Generic"_s : m_loadedFileName) +
           QDateTime::currentDateTime().toString(u".yyyy-MM-dd_hh:mm:ss"_s) + u".rzx"_s;
}

QString ZxEmulator::saveSnapshotsFilePath(const QString &filePath) const
{
    return savesPath() + "Snapshots/" + (filePath.isEmpty() ? u"Unnamed"_s : filePath) +
           QDateTime::currentDateTime().toString(u"_yyyy-MM-dd_hh:mm:ss"_s) + u".tzx"_s;
}

void ZxEmulator::load(const QString &filePath, bool removeOnFail)
{
    m_loadedFileName = QFileInfo(filePath).baseName();
    if (!loadMediaFile(filePath)) {
        m_loadedFileName.clear();
        if (removeOnFail) {
            QFile::remove(filePath);
            emit error(Info, tr("File \"%1\" was removed").arg(filePath));
        }
        emit saveSnapshotEnabledChanged();
        return;
    }

    // If a tape was loaded and autoLoad is enabled, type LOAD "" + Enter
    const auto mediaClass = zxkit::classifyMediaFile(filePath.toStdString());
    if (mediaClass == zxkit::MediaClass::Tape && m_settings && m_settings->autoLoad()) {
        withEmulator([](zxkit::Emulator &emulator) {
            emulator.reset(zxkit::ResetKind::Soft);
            emulator.runFrames(90); // the ROM initializes
            const auto tapKey = [&emulator](std::optional<zxkit::ZxKey> k1, std::optional<zxkit::ZxKey> k2) {
                for (const bool pressed : {true, false}) {
                    if (k1)
                        emulator.setKeyState(*k1, pressed);
                    if (k2)
                        emulator.setKeyState(*k2, pressed);
                    emulator.runFrames(5); // the ROM debounces keys for a few frames
                }
            };
            const auto id = emulator.machine().id();
            if (id == "48k" || id == "16k" || id == "48k_ntsc") {
                tapKey(zxkit::ZxKey::J, std::nullopt);              // LOAD
                tapKey(zxkit::ZxKey::P, zxkit::ZxKey::SymbolShift); // "
                tapKey(zxkit::ZxKey::P, zxkit::ZxKey::SymbolShift); // "
                tapKey(zxkit::ZxKey::Enter, std::nullopt);          // ENTER
            } else {
                tapKey(zxkit::ZxKey::Enter, std::nullopt);          // 128K Tape Loader
            }
        });
        m_engine->refreshFrame();
    }

    m_resetPokeFinder = true;
    m_tape->refreshData();
    emit saveSnapshotEnabledChanged();
}

void ZxEmulator::save(const QString &filePath)
{
    QDir d = QFileInfo(filePath).absoluteDir();
    d.mkpath(d.absolutePath());
    if (!saveMediaFile(filePath))
        QFile::remove(filePath);
    else
        showMessage(tr("File saved to '%1'").arg(filePath), Info);
}

void ZxEmulator::copyToFavourites(const QString &filePath)
{
    QDir d(dataPath());
    d.mkpath(u"Favourites"_s);
    const QString &dest = dataPath() + u"Favourites/"_s + QFileInfo(filePath).fileName();
    if (!QFile::exists(dest) && !QFile::copy(filePath, dest)) {
        emit error(Error, tr("Can't copy \"%1\" to \"%2\"").arg(filePath).arg(dest));
        return;
    }
    load(dest, true);
}

void ZxEmulator::remove(const QString &path)
{
    QFileInfo inf(path);
    if (inf.isDir()) {
        QDir dir(path);
        for (const QFileInfo &item : dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot))
            remove(item.absoluteFilePath());
        dir.rmdir(path);
    } else {
        QFile::remove(path);
    }
}

bool ZxEmulator::isFolder(const QString &path)
{
    return QFileInfo{path}.isDir();
}

QString ZxEmulator::fileName(const QString &path)
{
    return QFileInfo{path}.fileName();
}

QString ZxEmulator::folderName(const QString &path)
{
    return QFileInfo{path}.absolutePath();
}

bool ZxEmulator::copy(const QString &from, const QString &to, bool override)
{
    QFileInfo inf{from};
    if (inf.isDir()) {
        QDir src{from};
        QDir dst{to + QDir::separator() + src.dirName()};
        if (!override && dst.exists())
            return false;

        QDirIterator it{src.absolutePath(), QDir::Files, QDirIterator::Subdirectories};
        while (it.hasNext()) {
            it.next();
            const auto item = it.fileInfo();
            auto path = dst.absolutePath() + QDir::separator() + src.relativeFilePath(item.absolutePath());
            dst.mkpath(path);
            auto destFile = path + QDir::separator() + item.fileName();
            QFile::remove(destFile);
            QFile::copy(item.absoluteFilePath(), destFile);
        }
    } else {
        auto destFile = to + QDir::separator() + inf.fileName();
        if (!override && QFile::exists(destFile))
            return false;
        QFile::remove(destFile);
        QFile::copy(inf.absoluteFilePath(), destFile);
    }
    return true;
}

void ZxEmulator::reset()
{
    resetLoadedFile();
    emulator().writeLock()->reset(zxkit::ResetKind::Soft);
    m_engine->refreshFrame();
}

void ZxEmulator::hardReset()
{
    resetLoadedFile();
    emulator().writeLock()->reset(zxkit::ResetKind::Hard);
    m_engine->refreshFrame();
}

void ZxEmulator::nmi()
{
    resetLoadedFile();
    emulator().writeLock()->triggerNmi();
}

void ZxEmulator::quickSaveSnapshot()
{
    if (m_loadedFileName.isEmpty())
        return;

    const QString name = m_loadedFileName + u'/' + m_loadedFileName +
                         QDateTime::currentDateTime().toString(u".yyyy-MM-dd_hh:mm:ss"_s) +
                         u".szx"_s;
    save(snapshotsPath() + name);
}

void ZxEmulator::quickLoadSnapshot()
{
    QDir dir(snapshotsPath());
    auto list = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Time);
    if (list.size())
        list = QDir(list.first().absoluteFilePath()).entryInfoList(QDir::Files, QDir::Time);
    else
        list = dir.entryInfoList(QDir::Files, QDir::Time);

    if (list.size()) {
        load(list.first().filePath(), true);
        showMessage(tr("Snapshot loaded from '%1'").arg(list.first().fileName()));
    }
}

void ZxEmulator::disassamble()
{
    disassamble(registers().PC);
}

void ZxEmulator::disassambleFetchUp(int lines)
{
    m_disassambleModel.disassambleMore(DisassambleModel::Start, lines);
}

void ZxEmulator::disassamble(int address, int delta, uint16_t length)
{
    m_disassambleModel.disassamble(uint16_t(address), delta, length);
}

void ZxEmulator::activateDebugger()
{
    bool expected = false;
    if (!m_debuggerActivated.compare_exchange_strong(expected, true))
        return;

    setPaused(true);
    updateDebugger();
    emit showDebugger();
}

void ZxEmulator::deactivateDebugger(bool /*interruptable*/)
{
    bool expected = true;
    if (!m_debuggerActivated.compare_exchange_strong(expected, false))
        return;

    setPaused(false);
}

QString ZxEmulator::dumpData(const QString &base64CompressedData, int offset, int charsPerLine)
{
    if (base64CompressedData.isEmpty())
        return QString();

    auto data = qUncompress(QByteArray::fromBase64(base64CompressedData.toLatin1(), QByteArray::Base64UrlEncoding));

    const int sz = data.size() - offset;
    if (sz <= 0)
        return QString();

    auto ptr = data.constData() + offset;
    QString res;

    int pos = 0;
    for (; pos < sz; ++pos) {
        if (!(pos % charsPerLine)) {
            if (!res.isEmpty())
                res += u'\n';
            res += u"%1"_s.arg(offset + pos, 6, 10, u'0').toUpper();
        }

        res += u" %1"_s.arg(uint8_t(*(ptr + pos)), 2, 16, u'0').toUpper();
        if ((pos % charsPerLine) == charsPerLine - 1) {
            res += u"   "_s;
            auto pt = ptr + pos - charsPerLine;
            for (int i = 0; i < charsPerLine; ++i) {
                auto ch = *pt++;
                if (std::isprint(static_cast<unsigned char>(ch)))
                    res += QChar::fromLatin1(ch);
                else
                    res += u"."_s;
            }
        }
    }

    for (int i = pos % charsPerLine; i < charsPerLine; ++i)
        res += u"   "_s;

    res += u"   "_s;

    auto pt = ptr + pos - pos % charsPerLine;
    for (int i = 0; i < pos % charsPerLine; ++i) {
        auto ch = *pt++;
        if (std::isprint(static_cast<unsigned char>(ch)))
            res += QChar::fromLatin1(ch);
        else
            res += u"."_s;
    }

    return res;
}

void ZxEmulator::pokeFinderInced()
{
    withEmulator([](zxkit::Emulator &emulator) { emulator.pokeFinderSearchIncremented(); });
    m_pokeFinderModel.update();
    emit pokeFinderCountChanged();
}

void ZxEmulator::pokeFinderDeced()
{
    withEmulator([](zxkit::Emulator &emulator) { emulator.pokeFinderSearchDecremented(); });
    m_pokeFinderModel.update();
    emit pokeFinderCountChanged();
}

void ZxEmulator::pokeFinderSearch(int value)
{
    withEmulator([value](zxkit::Emulator &emulator) { emulator.pokeFinderSearch(uint8_t(value)); });
    m_pokeFinderModel.update();
    emit pokeFinderCountChanged();
}

void ZxEmulator::pokeFinderReset()
{
    withEmulator([](zxkit::Emulator &emulator) { emulator.pokeFinderReset(); });
    m_pokeFinderModel.update();
    emit pokeFinderCountChanged();
}

void ZxEmulator::pokeFinderResetIfNeeded()
{
    if (m_resetPokeFinder) {
        pokeFinderReset();
        m_resetPokeFinder = false;
    }
}

void ZxEmulator::pokeMemory(int address, int page, int value)
{
    auto emulator = this->emulator().writeLock();
    auto &mmu = emulator->mmu();
    if (page >= 8) {
        mmu.poke(uint16_t(address), uint8_t(value), true);
    } else {
        auto &ram = mmu.bank(emulator->machine().ramBankId());
        if (size_t(page) < ram.count16kPages())
            ram.span16k(size_t(page))[address & 0x3FFF] = uint8_t(value);
    }
}

void ZxEmulator::keyPress(int qtKey, int modifiers, bool autoRepeat, bool /*virtualKeyboard*/)
{
    if (m_paused || autoRepeat)
        return;

    const auto mapped = mapQtKeyToZx(qtKey, modifiers);
    if (!mapped.primary && !mapped.modifier)
        return;

    auto emulator = this->emulator().writeLock();
    if (mapped.modifier)
        emulator->setKeyState(*mapped.modifier, true);
    if (mapped.primary)
        emulator->setKeyState(*mapped.primary, true);
}

void ZxEmulator::keyRelease(int qtKey, int modifiers, bool autoRepeat, bool /*virtualKeyboard*/)
{
    if (m_paused || autoRepeat)
        return;

    const auto mapped = mapQtKeyToZx(qtKey, modifiers);
    if (!mapped.primary && !mapped.modifier)
        return;

    auto emulator = this->emulator().writeLock();
    if (mapped.primary)
        emulator->setKeyState(*mapped.primary, false);
    if (mapped.modifier)
        emulator->setKeyState(*mapped.modifier, false);
}

void ZxEmulator::gamepadAxisEvent(QGamepadManager::GamepadAxis axis, double value)
{
    const double deadZone = m_deadZone;
    withPeripheral<zxkit::KempstonJoystickPeripheral>(*emulator().writeLock(), "kempston", [deadZone, axis, value](auto &kemp) {
        uint8_t negBit = 0;
        uint8_t posBit = 0;
        switch (axis) {
        case QGamepadManager::AxisLeftX:
        case QGamepadManager::AxisRightX:
            negBit = zxkit::JoystickButtons::Left;
            posBit = zxkit::JoystickButtons::Right;
            break;
        case QGamepadManager::AxisLeftY:
        case QGamepadManager::AxisRightY:
            negBit = zxkit::JoystickButtons::Up;
            posBit = zxkit::JoystickButtons::Down;
            break;
        default:
            return;
        }

        uint8_t buttons = kemp.buttons() & static_cast<uint8_t>(~(negBit | posBit));
        if (value <= -deadZone)
            buttons |= negBit;
        else if (value >= deadZone)
            buttons |= posBit;
        kemp.setButtons(buttons);
    });
}

void ZxEmulator::gamepadButtonPressEvent(QGamepadManager::GamepadButton button)
{
    auto action = m_settings->gamepadAction(button);
    switch (action) {
    case ZxSettings::JoystickFire:
        withPeripheral<zxkit::KempstonJoystickPeripheral>(*emulator().writeLock(), "kempston", [button](auto &kemp) {
            kemp.setButtons(kemp.buttons() | toKempstonMask(button));
        });
        break;
    case ZxSettings::ToggleEmulationSpeed:
        speedup();
        break;
    case ZxSettings::ToggleKeyboard:
    case ZxSettings::ToggleCursorJoystick:
    case ZxSettings::TogglePause:
    case ZxSettings::QuickSaveSnapshot:
    case ZxSettings::QuickLoadSnapshot:
        break;
    default:
        keyPress(action);
        break;
    }
}

void ZxEmulator::gamepadButtonReleaseEvent(QGamepadManager::GamepadButton button)
{
    auto action = m_settings->gamepadAction(button);
    switch (action) {
    case ZxSettings::JoystickFire:
        if (m_paused) {
            showMessage(tr("Spectacol is paused"));
            return;
        }
        withPeripheral<zxkit::KempstonJoystickPeripheral>(*emulator().writeLock(), "kempston", [button](auto &kemp) {
            kemp.setButtons(kemp.buttons() & static_cast<uint8_t>(~toKempstonMask(button)));
        });
        break;
    case ZxSettings::ToggleEmulationSpeed:
        slowdown();
        break;
    case ZxSettings::ToggleKeyboard:
    case ZxSettings::ToggleCursorJoystick:
        break;
    case ZxSettings::TogglePause:
        togglePaused();
        break;
    case ZxSettings::QuickSaveSnapshot:
        if (m_paused) {
            showMessage(tr("Spectacol is paused"));
            return;
        }
        quickSaveSnapshot();
        break;
    case ZxSettings::QuickLoadSnapshot:
        if (m_paused) {
            showMessage(tr("Spectacol is paused"));
            return;
        }
        quickLoadSnapshot();
        break;
    default:
        if (m_paused) {
            showMessage(tr("Spectacol is paused"));
            return;
        }
        keyRelease(action);
        break;
    }
}

void ZxEmulator::showMessage(QString message, ZxEmulator::ErrorLevel level)
{
    if (level != Info)
        qWarning().noquote() << message;
    // Any thread (e.g. the Engine's log callback)
    QMetaObject::invokeMethod(this, [this, message, level] {
        emit error(level, message);
    }, Qt::QueuedConnection);
}

void ZxEmulator::setSpeed(int percent)
{
    m_engine->setSpeed(uint32_t(std::max(percent, 1)));
    m_realTimeAudio = m_engine->isRealTime();
}

void ZxEmulator::speedup()
{
    m_engine->setTurbo(true);
    m_realTimeAudio = false;
}

void ZxEmulator::slowdown()
{
    m_engine->setTurbo(false);
    m_realTimeAudio = m_engine->isRealTime();
}

void ZxEmulator::togglePaused()
{
    m_paused = !m_paused;
    if (!m_touchscreen || !m_showControlsIcons)
        showMessage(m_paused ? tr("Pause spectacol") : tr("Resume spectacol"));
    setPaused(m_paused);
}

void ZxEmulator::debuggerTrap()
{
    activateDebugger();
}

void ZxEmulator::debuggerStep()
{
    emulator().writeLock()->stepInstruction();
    m_engine->refreshFrame();
    updateDebugger();
}

void ZxEmulator::debuggerNext()
{
    // A call or a block instruction runs until the next one: the pause callback updates the debugger
    m_engine->stepOver();
    updateDebugger();
}

void ZxEmulator::debuggerRun()
{
    deactivateDebugger(true);
}

void ZxEmulator::addBreakpoint(int address, int type)
{
    zxkit::Breakpoint bp;
    bp.type = zxkit::BreakpointType(type);
    bp.address = uint16_t(address);
    emulator().writeLock()->addBreakpoint(std::move(bp));
    m_breakpointsModel.breakpointsUpdated();
    m_pokeFinderModel.update();
}

void ZxEmulator::addBreakpointPage(int address, int page, int type)
{
    zxkit::Breakpoint bp;
    bp.type = zxkit::BreakpointType(type);
    bp.address = uint16_t(address);
    bp.page1k = uint16_t(page * 16 + ((address & 0x3FFF) >> 10));
    emulator().writeLock()->addBreakpoint(std::move(bp));
    m_breakpointsModel.breakpointsUpdated();
    m_pokeFinderModel.update();
}

void ZxEmulator::debuggerCommand(const QString &command)
{
    const QString trimmed = command.trimmed();
    if (trimmed.startsWith(u"del"_s, Qt::CaseInsensitive) ||
        trimmed.startsWith(u"clear"_s, Qt::CaseInsensitive)) {
        const auto parts = trimmed.split(u' ', Qt::SkipEmptyParts);
        if (parts.size() >= 2) {
            const uint32_t id = parts[1].toUInt();
            emulator().writeLock()->removeBreakpoint(id);
            m_breakpointsModel.breakpointsUpdated();
            m_pokeFinderModel.update();
            return;
        }
        emulator().writeLock()->clearBreakpoints();
        m_breakpointsModel.breakpointsUpdated();
        m_pokeFinderModel.update();
    } else if (trimmed.startsWith(u"base"_s, Qt::CaseInsensitive)) {
        const auto parts = trimmed.split(u' ', Qt::SkipEmptyParts);
        if (parts.size() >= 2) {
            setDebuggerOutputBase(parts[1].toInt());
            updateDebugger();
            m_breakpointsModel.breakpointsUpdated();
        }
    }
}

void ZxEmulator::stopEngine()
{
    if (m_engine)
        m_engine->stop();
}

const DisassambleModel *ZxEmulator::disassambleModel() const
{
    return &m_disassambleModel;
}

const PokeFinderModel *ZxEmulator::pokeFinderModel() const
{
    return &m_pokeFinderModel;
}

const ZXGamesModel *ZxEmulator::onlineGamesModel() const
{
    return &m_onlineGamesModel;
}
