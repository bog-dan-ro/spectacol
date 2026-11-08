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
#include <QPointer>
#include <QWindow>
#include <QQuickWindow>
#include <QQuickItem>
#include <QInputMethod>
#include "zxrecording.h"
#include "zxsettings.h"
#include "qmlui.h"
#include "zximage.h"
#include "screenfilters.h"

#include <zxkit/input/Joystick.h>
#include <zxkit/input/Mouse.h>
#include <zxkit/media/MediaPreview.h>
#include <zxkit/media/SnapshotFormats.h>
#include <zxkit/machines/CloneMachines.h>
#include <zxkit/peripherals/expansion/ExpansionPeripherals.h>

#include "audiooutput.h"
#include <QBuffer>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QSettings>
#include <QSaveFile>
#include <QStandardPaths>
#include <QTimer>

#include <algorithm>
#include <array>
#include <cctype>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace Qt::StringLiterals;
using namespace std::literals;

#ifdef Q_OS_ANDROID
#include <QCoreApplication>
#include <QJniObject>
#include "androidintegration.h"
#endif

namespace {

constexpr quint64 kPhysicalKeys = quint64(1) << 32;
constexpr quint64 kVirtualKeys = quint64(2) << 32;
constexpr quint64 kControllerKeys = quint64(3) << 32;
constexpr quint64 kJoystickKeys = quint64(4) << 32;
constexpr quint64 kCobraKeys = quint64(5) << 32;
constexpr quint64 kRecreatedKeys = quint64(6) << 32;
static_assert(ControllerInput::LeftBit == zxkit::JoystickButtons::Left
              && ControllerInput::RightBit == zxkit::JoystickButtons::Right
              && ControllerInput::UpBit == zxkit::JoystickButtons::Up
              && ControllerInput::DownBit == zxkit::JoystickButtons::Down
              && ControllerInput::FireBit == zxkit::JoystickButtons::Fire1);

QString saveName(QString name, const QString &fallback)
{
    name = name.trimmed();
    for (QChar &ch : name) {
        if (ch.unicode() < 32 || ch.unicode() == 127 || u"<>:\"/\\|?*"_s.contains(ch))
            ch = u'_';
    }
    while (name.endsWith(u'.') || name.endsWith(u' '))
        name.chop(1);
    if (name.isEmpty())
        return fallback;
    const QString stem = name.section(u'.', 0, 0).toUpper();
    if (stem == u"CON"_s || stem == u"PRN"_s || stem == u"AUX"_s || stem == u"NUL"_s
        || (stem.size() == 4 && (stem.startsWith(u"COM"_s) || stem.startsWith(u"LPT"_s))
            && stem.back() >= u'1' && stem.back() <= u'9'))
        name.prepend(u'_');
    return name;
}

QString saveTimestamp()
{
    return QDateTime::currentDateTime().toString(u"yyyy-MM-dd_hh_mm_ss_zzz"_s);
}

struct MappedZxKeys
{
    std::optional<zxkit::ZxKey> primary;
    std::optional<zxkit::ZxKey> modifier;
};

// The CoBra 1's keyboard has a sixth column: CTRL, NO SCROLL, LINE FEED, ESC and F1-F4. The
// host's keys for them, on a physical keyboard (the on-screen keyboard's Control is Symbol Shift).
// Spectacol's own keys start at F5.
std::optional<zxkit::ZxKey> cobraKey(int qtKey, int modifiers)
{
    using zxkit::ZxKey;
    switch (qtKey) {
    case Qt::Key_Control:
        return ZxKey::Ctrl;
    case Qt::Key_Escape:
        return ZxKey::Esc;
    case Qt::Key_ScrollLock:
        return ZxKey::NoScroll;
    case Qt::Key_Enter:
        // the keypad's Enter
        if (modifiers & Qt::KeypadModifier)
            return ZxKey::LineFeed;
        return std::nullopt;
    case Qt::Key_F1:
        return ZxKey::F1;
    case Qt::Key_F2:
        return ZxKey::F2;
    case Qt::Key_F3:
        return ZxKey::F3;
    case Qt::Key_F4:
        return ZxKey::F4;
    default:
        return std::nullopt;
    }
}

bool hasCobraKeyboard(const zxkit::Emulator &emulator)
{
    return emulator.machine().id().starts_with("cobra1"sv);
}

MappedZxKeys mapQtKeyToZx(int qtKey, int modifiers)
{
    using zxkit::ZxKey;

    if (qtKey >= Qt::Key_A && qtKey <= Qt::Key_Z) {
        static constexpr ZxKey kLetters[26] = { ZxKey::A, ZxKey::B, ZxKey::C, ZxKey::D, ZxKey::E,
                                                ZxKey::F, ZxKey::G, ZxKey::H, ZxKey::I, ZxKey::J,
                                                ZxKey::K, ZxKey::L, ZxKey::M, ZxKey::N, ZxKey::O,
                                                ZxKey::P, ZxKey::Q, ZxKey::R, ZxKey::S, ZxKey::T,
                                                ZxKey::U, ZxKey::V, ZxKey::W, ZxKey::X, ZxKey::Y,
                                                ZxKey::Z };
        const auto key = kLetters[qtKey - Qt::Key_A];
        if (modifiers & Qt::ShiftModifier)
            return { key, ZxKey::CapsShift };
        return { key, std::nullopt };
    }

    if (qtKey >= Qt::Key_0 && qtKey <= Qt::Key_9) {
        static constexpr ZxKey kDigits[10] = { ZxKey::Num0, ZxKey::Num1, ZxKey::Num2, ZxKey::Num3,
                                               ZxKey::Num4, ZxKey::Num5, ZxKey::Num6, ZxKey::Num7,
                                               ZxKey::Num8, ZxKey::Num9 };
        return { kDigits[qtKey - Qt::Key_0], std::nullopt };
    }

    switch (qtKey) {
    case Qt::Key_Return:
    case Qt::Key_Enter:
        return { ZxKey::Enter, std::nullopt };
    case Qt::Key_Space:
        return { ZxKey::Space, std::nullopt };
    case Qt::Key_Shift:
        return { ZxKey::CapsShift, std::nullopt };
    case Qt::Key_Control:
    case Qt::Key_Alt:
    case Qt::Key_Meta:
        return { ZxKey::SymbolShift, std::nullopt };
    case Qt::Key_Backspace:
        return { ZxKey::Num0, ZxKey::CapsShift };
    case Qt::Key_CapsLock:
        return { ZxKey::Num2, ZxKey::CapsShift };
    case Qt::Key_Escape:
        return { ZxKey::Space, ZxKey::CapsShift };
    case Qt::Key_Left:
        return { ZxKey::Num5, ZxKey::CapsShift };
    case Qt::Key_Down:
        return { ZxKey::Num6, ZxKey::CapsShift };
    case Qt::Key_Up:
        return { ZxKey::Num7, ZxKey::CapsShift };
    case Qt::Key_Right:
        return { ZxKey::Num8, ZxKey::CapsShift };
    case Qt::Key_Exclam:
        return { ZxKey::Num1, ZxKey::SymbolShift };
    case Qt::Key_At:
        return { ZxKey::Num2, ZxKey::SymbolShift };
    case Qt::Key_NumberSign:
        return { ZxKey::Num3, ZxKey::SymbolShift };
    case Qt::Key_Dollar:
        return { ZxKey::Num4, ZxKey::SymbolShift };
    case Qt::Key_Percent:
        return { ZxKey::Num5, ZxKey::SymbolShift };
    case Qt::Key_Ampersand:
        return { ZxKey::Num6, ZxKey::SymbolShift };
    case Qt::Key_Apostrophe:
        return { ZxKey::Num7, ZxKey::SymbolShift };
    case Qt::Key_ParenLeft:
        return { ZxKey::Num8, ZxKey::SymbolShift };
    case Qt::Key_ParenRight:
        return { ZxKey::Num9, ZxKey::SymbolShift };
    case Qt::Key_Underscore:
        return { ZxKey::Num0, ZxKey::SymbolShift };
    case Qt::Key_QuoteDbl:
        return { ZxKey::P, ZxKey::SymbolShift };
    case Qt::Key_Semicolon:
        return { ZxKey::O, ZxKey::SymbolShift };
    case Qt::Key_Comma:
        return { ZxKey::N, ZxKey::SymbolShift };
    case Qt::Key_Period:
        return { ZxKey::M, ZxKey::SymbolShift };
    case Qt::Key_Plus:
        return { ZxKey::K, ZxKey::SymbolShift };
    case Qt::Key_Minus:
        return { ZxKey::J, ZxKey::SymbolShift };
    case Qt::Key_Asterisk:
        return { ZxKey::B, ZxKey::SymbolShift };
    case Qt::Key_Slash:
        return { ZxKey::V, ZxKey::SymbolShift };
    case Qt::Key_Equal:
        return { ZxKey::L, ZxKey::SymbolShift };
    case Qt::Key_Colon:
        return { ZxKey::Z, ZxKey::SymbolShift };
    case Qt::Key_Less:
        return { ZxKey::R, ZxKey::SymbolShift };
    case Qt::Key_Greater:
        return { ZxKey::T, ZxKey::SymbolShift };
    case Qt::Key_Question:
        return { ZxKey::C, ZxKey::SymbolShift };
    default:
        return { std::nullopt, std::nullopt };
    }
}

std::optional<zxkit::NextKey> nextExtendedKey(int key)
{
    using K = zxkit::NextKey;
    switch (key) {
    case Qt::Key_Right:
        return K::Right;
    case Qt::Key_Left:
        return K::Left;
    case Qt::Key_Down:
        return K::Down;
    case Qt::Key_Up:
        return K::Up;
    case Qt::Key_Period:
        return K::Period;
    case Qt::Key_Comma:
        return K::Comma;
    case Qt::Key_QuoteDbl:
        return K::Quote;
    case Qt::Key_Semicolon:
        return K::Semicolon;
    case Qt::Key_Tab:
        return K::Extend;
    case Qt::Key_CapsLock:
    case Qt::Key_F2:
        return K::CapsLock;
    case Qt::Key_Home:
        return K::Graph;
    case Qt::Key_F3:
        return K::TrueVideo;
    case Qt::Key_F4:
        return K::InvVideo;
    case Qt::Key_Escape:
        return K::Break;
    case Qt::Key_Insert:
    case Qt::Key_F1:
        return K::Edit;
    case Qt::Key_Backspace:
    case Qt::Key_Delete:
        return K::Delete;
    default:
        return std::nullopt;
    }
}

// The Recreated ZX Spectrum keyboard (Elite Systems), in its game mode, types a character when a
// key goes down and another when it goes up (FUSE's input.c). The characters, as US keys
// (upper case and the shifted symbols: typed with Shift):
//   down: a c e g i k m o q s u w y A C E G I K M O Q S U W Y 0 2 4 6 8 < - [ ; , / { ! %
//   up:   b d f h j l n p r t v x z B D F H J L N P R T V X Z 1 3 5 7 9 > = ] : . ? } $ ^
// for the keys 1 2 3 4 5 6 7 8 9 0 Q W E R T Y U I O P A S D F G H J K L ENTER CAPS Z X C V B N M SYMBOL SPACE
std::optional<std::pair<zxkit::ZxKey, bool>> recreatedKey(int qtKey, int modifiers)
{
    using zxkit::ZxKey;
    static constexpr auto kDown = "acegikmoqsuwyACEGIKMOQSUWY02468<-[;,/{!%"sv;
    static constexpr auto kUp = "bdfhjlnprtvxzBDFHJLNPRTVXZ13579>=]:.?}$^"sv;
    static constexpr std::array<ZxKey, 40> kKeys{
        ZxKey::Num1,      ZxKey::Num2, ZxKey::Num3,        ZxKey::Num4, ZxKey::Num5, ZxKey::Num6,
        ZxKey::Num7,      ZxKey::Num8, ZxKey::Num9,        ZxKey::Num0, ZxKey::Q,    ZxKey::W,
        ZxKey::E,         ZxKey::R,    ZxKey::T,           ZxKey::Y,    ZxKey::U,    ZxKey::I,
        ZxKey::O,         ZxKey::P,    ZxKey::A,           ZxKey::S,    ZxKey::D,    ZxKey::F,
        ZxKey::G,         ZxKey::H,    ZxKey::J,           ZxKey::K,    ZxKey::L,    ZxKey::Enter,
        ZxKey::CapsShift, ZxKey::Z,    ZxKey::X,           ZxKey::C,    ZxKey::V,    ZxKey::B,
        ZxKey::N,         ZxKey::M,    ZxKey::SymbolShift, ZxKey::Space
    };

    char c = 0;
    if (qtKey >= Qt::Key_A && qtKey <= Qt::Key_Z)
        c = static_cast<char>(((modifiers & Qt::ShiftModifier) ? 'A' : 'a') + (qtKey - Qt::Key_A));
    else if (qtKey >= Qt::Key_0 && qtKey <= Qt::Key_9)
        c = static_cast<char>('0' + (qtKey - Qt::Key_0));
    else if (qtKey >= 0x20 && qtKey < 0x7F)
        c = static_cast<char>(
                qtKey); // Qt's key codes for the other printable keys are their ASCII codes
    if (c == 0)
        return std::nullopt;
    if (const auto down = kDown.find(c); down != std::string_view::npos)
        return std::pair{ kKeys[down], true };
    if (const auto up = kUp.find(c); up != std::string_view::npos)
        return std::pair{ kKeys[up], false };
    return std::nullopt;
}

uint8_t toJoystickMask(QGamepadManager::GamepadButton button)
{
    switch (button) {
    case QGamepadManager::ButtonUp:
        return zxkit::JoystickButtons::Up;
    case QGamepadManager::ButtonDown:
        return zxkit::JoystickButtons::Down;
    case QGamepadManager::ButtonRight:
        return zxkit::JoystickButtons::Right;
    case QGamepadManager::ButtonLeft:
        return zxkit::JoystickButtons::Left;
    default:
        return zxkit::JoystickButtons::Fire1;
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

#define setRegisterValue(memberExpr)                                        \
    bool ok = false;                                                        \
    int val = value.toInt(&ok);                                             \
    if (!ok)                                                                \
        val = value.toInt(&ok, 16);                                         \
    if (!ok) {                                                              \
        emit error(Warning, tr("Value \"%1\" is not a number").arg(value)); \
        return;                                                             \
    }                                                                       \
    emulator().writeLock()->cpu().registers().memberExpr;                   \
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
    : QObject(parent),
      m_breakpointsModel(this),
      m_disassambleModel(this),
      m_pokeFinderModel(this),
      m_resetPokeFinder(true),
      m_settings(std::make_unique<ZxSettings>(*this)),
      m_controllerProfiles(std::make_unique<ControllerProfiles>(this)),
      m_tape(new ZxTape(this)),
      m_disks(new Disks(this)),
      m_sdCard(new SdCardFiles(this)),
      m_recording(new ZxRecording(this))
{
#ifdef Q_OS_ANDROID
    const auto manager =
            QJniObject(QNativeInterface::QAndroidApplication::context())
                    .callObjectMethod("getPackageManager", "()Landroid/content/pm/PackageManager;");
    const auto feature = QJniObject::fromString(u"android.hardware.touchscreen"_s);
    m_touchscreen = manager.callMethod<jboolean>("hasSystemFeature", "(Ljava/lang/String;)Z",
                                                 feature.object<jstring>());
#endif
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
    QQmlEngine::setObjectOwnership(m_tape, QQmlEngine::CppOwnership);
    QQmlEngine::setObjectOwnership(m_recording, QQmlEngine::CppOwnership);

    m_audioOutput = std::make_unique<AudioOutput>();
    connect(m_audioOutput.get(), &AudioOutput::errorOccurred, this,
            [this](const QString &message) { showMessage(message, Warning); });
    connect(m_settings.get(), &ZxSettings::settingsCurrentChanged, this,
            [this] { m_audioOutput->setMuted(!m_settings->soundEnabled()); });
    m_audioOutput->setMuted(!m_settings->soundEnabled());
    m_audioOutput->start();

    // The Engine calls these on its thread, it holds only Weak references to them
    m_videoFrameCallback = zxkit::CallbackHandle<zxkit::VideoFrameCallback>(
            [](const zxkit::VideoFramePtr &frame) { ZxImage::instance()->presentFrame(frame); });
    m_audioCallback = zxkit::CallbackHandle<zxkit::AudioCallback>(
            [this](std::span<const int16_t> samples, uint32_t sampleRate, uint8_t channels) {
                writeAudioSamples(samples, sampleRate, channels);
            });
    m_pauseCallback =
            zxkit::CallbackHandle<zxkit::PauseCallback>([this](const zxkit::PauseEvent &event) {
                m_audioOutput->setPaused(true);
                QMetaObject::invokeMethod(
                        this,
                        [this, event] {
                            m_engineQuiescent = true;
                            m_debugPause = event;
                            emit debuggerStatusChanged();
                            m_recording->refresh();
                            emit pausedChanged();
                            if (m_debuggerActivated)
                                updateDebugger(); // e.g. a step over reached its instruction
                            else if (m_recording->playbackError().isEmpty()
                                     && event.reason != zxkit::PauseReason::UserRequest)
                                activateDebugger(); // a breakpoint or a watchpoint
                        },
                        Qt::QueuedConnection);
            });
    m_resumeCallback = zxkit::CallbackHandle<zxkit::ResumeCallback>([this] {
        m_audioOutput->setPaused(false);
        QMetaObject::invokeMethod(
                this,
                [this] {
                    m_engineQuiescent = false;
                    if (m_debugMemory)
                        m_debugMemory->setEditable(false);
                    emit debuggerStatusChanged();
                    emit pausedChanged();
                },
                Qt::QueuedConnection);
    });
    m_logCallback = zxkit::CallbackHandle<zxkit::LogCallback>(
            [this](zxkit::LogLevel level, std::string_view msg) {
                ErrorLevel errLvl = Info;
                if (level == zxkit::LogLevel::Warning)
                    errLvl = Warning;
                else if (level >= zxkit::LogLevel::Error)
                    errLvl = Error;
                showMessage(QString::fromUtf8(msg.data(), qsizetype(msg.size())), errLvl);
            });

    m_mediaActivityCallback = zxkit::CallbackHandle<zxkit::MediaActivityCallback>(
            [this](const zxkit::MediaActivity &activity) {
                QMetaObject::invokeMethod(
                        this, [this, activity] { updateMediaActivity(activity); },
                        Qt::QueuedConnection);
            });

    zxkit::EmulatorConfig cfg;
    cfg.machineId = "48k"s;
    cfg.audioSampleRate = 48'000;
    cfg.fullBorder = true;
    m_roms = std::make_unique<RomOverrides>();
    cfg.machineRomResolver = [this](std::string_view machine, std::string_view name,
                                    std::size_t size) {
        return m_roms->resolve(machine, name, size);
    };

    m_uartHooks = std::make_unique<NextUartHooks>();
    QQmlEngine::setObjectOwnership(m_uartHooks.get(), QQmlEngine::CppOwnership);
    zxkit::EngineConfig engineConfig;
    engineConfig.startPaused = true;
    engineConfig.threadHooks = m_uartHooks->threadHooks();
    engineConfig.callbacks = {
        .videoFrame = m_videoFrameCallback.weak(),
        .audio = m_audioCallback.weak(),
        .paused = m_pauseCallback.weak(),
        .resumed = m_resumeCallback.weak(),
        .log = m_logCallback.weak(),
        .mediaActivity = m_mediaActivityCallback.weak(),
    };
    m_engine = std::make_unique<zxkit::Engine>(std::move(cfg), std::move(engineConfig));
    m_pokes = std::make_unique<PokeModel>(emulator());
    QQmlEngine::setObjectOwnership(m_pokes.get(), QQmlEngine::CppOwnership);
    QQmlEngine::setObjectOwnership(m_roms.get(), QQmlEngine::CppOwnership);
    QQmlEngine::setObjectOwnership(&m_symbols, QQmlEngine::CppOwnership);
    connect(m_pokes.get(), &PokeModel::failed, this,
            [this](const QString &message) { showMessage(message, Error); });
    connect(m_pokes.get(), &PokeModel::memoryChanged, this, &ZxEmulator::debuggerRefresh);
    connect(&m_symbols, &DebugSymbols::changed, this, [this] {
        if (debuggerReady())
            m_disassambleModel.update();
    });
    emulator().writeLock()->setRewindEnabled(true, 300);
    m_settings->applyToEmulator();
    {
        QSettings settings;
        settings.beginGroup(u"Screen"_s);
        const auto savedFilter = settings.value(u"softwareFilter"_s, u"normal"_s).toString();
        const auto presets = screenFilterPresets();
        for (std::size_t index = 0; index < presets.size(); ++index) {
            if (presets[index].id == savedFilter) {
                m_selectedFilterIndex = int(index);
                break;
            }
        }
    }
    m_selectedJoystickIndex = std::clamp(QSettings().value("joystick", int(CursorKeys)).toInt(),
                                         int(NoJoystick), int(Timex2));
    attachJoystick(m_selectedJoystickIndex);

    m_joystickPromptTimer.setSingleShot(true);
    m_joystickPromptTimer.setInterval(500);
    connect(&m_joystickPromptTimer, &QTimer::timeout, this, [this] {
        if (m_settings->joystickPrompt())
            emit configureJoystick();
    });

    connect(m_controllerProfiles.get(), &ControllerProfiles::failed, this,
            [this](const QString &message) { showMessage(message, Error); });
    connect(m_controllerProfiles.get(), &ControllerProfiles::activationRequested, this,
            [this](const QString &name) {
                if (!m_settings->controllerOverride())
                    m_defaultControllerMapping = controllerMapping();
                applyControllerMapping(name.isEmpty() ? m_defaultControllerMapping
                                                      : m_controllerProfiles->mapping(name),
                                       !name.isEmpty());
            });
    connect(m_settings.get(), &ZxSettings::settingsCurrentChanged, this, [this] {
        m_tapeFastForward = m_mediaActivity.tapePlaying && m_settings->loaderAcceleration() > 1;
        updateFastForwardIndicator();
    });

    connect(qGuiApp, &QCoreApplication::aboutToQuit, this, &ZxEmulator::stopEngine);
    connect(qGuiApp, &QGuiApplication::applicationStateChanged, this,
            [this](Qt::ApplicationState state) {
                const bool wasActive = m_applicationState == Qt::ApplicationActive;
                const bool active = state == Qt::ApplicationActive;
                m_applicationState = state;
                if (wasActive != active)
                    setPaused(!active);
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

    connect(gm, &QGamepadManager::gamepadAxisEvent, this,
            [this](int deviceId, QGamepadManager::GamepadAxis axis, double value) {
                if (deviceId != m_gamepadId || axis == QGamepadManager::AxisInvalid)
                    return;
                gamepadAxisEvent(axis, value);
            });

    connect(gm, &QGamepadManager::gamepadButtonPressEvent, this,
            [this](int deviceId, QGamepadManager::GamepadButton button, double value) {
                if (value < 0.5)
                    return;

                switch (button) {
                case QGamepadManager::ButtonStart:
                    if (!m_processInputEvents)
                        return;
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

    connect(gm, &QGamepadManager::gamepadButtonReleaseEvent, this,
            [this](int deviceId, QGamepadManager::GamepadButton button) {
                if (deviceId != m_gamepadId || button == QGamepadManager::ButtonInvalid
                    || button == QGamepadManager::ButtonStart)
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
            auto devices = QGamepadManager::instance()->connectedGamepads();
            devices.removeAll(deviceId);
            setGamepadId(devices.isEmpty() ? -1 : devices.first());

            if (m_touchscreen && !m_showControlsIcons)
                emit showControlsIconsChanged(m_showControlsIcons = true);
        }
    });

    connect(&m_breakpointsModel, &BreakpointsModel::modelReset, &m_disassambleModel,
            &DisassambleModel::update);

    qRegisterMetaType<ErrorLevel>("ErrorLevel");

    setDataPath(dataPath());
    m_debuggerActivated.store(false);
    m_processInputEvents.store(true);
    configureController();
    connect(m_settings.get(), &ZxSettings::controllerMappingChanged, this,
            &ZxEmulator::configureController);
    connect(m_settings.get(), &ZxSettings::deadZoneChanged, this,
            [this] { configureController(); });

    // Engine::run may start after this constructor clears its initial pause;
    // do not depend on observing a Resume event to enable the first packet.
    m_audioOutput->setPaused(m_pauseCount > 0);
    m_engine->setPaused(m_pauseCount > 0);

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

void ZxEmulator::writeAudioSamples(std::span<const int16_t> samples, uint32_t sampleRate,
                                   uint8_t channels)
{
    // Engine thread -> bounded queue -> Qt audio thread. No GUI event delivery.
    m_audioOutput->write(samples, sampleRate, channels,
                         m_engine->isTurbo() ? 0 : m_engine->speed());
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
    if (paused && m_pauseCount == 0)
        releaseInputs();
    if (paused)
        ++m_pauseCount;
    else if (m_pauseCount > 0)
        --m_pauseCount;
    else
        return;
    if (m_engine) {
        if (!m_pauseCount)
            m_engineQuiescent = false;
        if ((m_pauseCount > 0) != m_engine->isPaused() || !m_engineQuiescent)
            m_engine->setPaused(m_pauseCount > 0);
    }
    emit pausedChanged();
}

QString ZxEmulator::dataPath() const
{
    static QString path;
    if (!path.isEmpty())
        return path;

    QSettings s;
#if defined(Q_OS_ANDROID)
    const auto appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    path = s.value("dataPath", appData).toString();
    const auto relative = QDir(appData).relativeFilePath(path);
    // Shared external paths saved by the Qt 5 app are no longer writable under
    // scoped storage. Keep its files in place; imports go through the picker.
    if (QDir::isAbsolutePath(relative) || relative == u".."_s || relative.startsWith(u"../"_s)) {
        s.setValue(u"Android/legacyDataPath"_s, path);
        path = appData;
    }
#elif defined(Q_OS_IOS)
    path = s.value("dataPath",
                   QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation).last()
                           + u"/Spectacol/"_s)
                   .toString();
#else
    path = s.value("dataPath",
                   QStandardPaths::writableLocation(QStandardPaths::HomeLocation)
                           + u"/Spectacol/"_s)
                   .toString();
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
    QStringList names;
    for (const auto &preset : screenFilterPresets())
        names.push_back(preset.name);
    return names;
}

int ZxEmulator::selectedFilterIndex() const
{
    return m_selectedFilterIndex;
}

void ZxEmulator::setSelectedFilterIndex(int selectedFilterIndex)
{
    const auto presets = screenFilterPresets();
    if (selectedFilterIndex < 0 || std::size_t(selectedFilterIndex) >= presets.size()
        || m_selectedFilterIndex == selectedFilterIndex)
        return;
    m_selectedFilterIndex = selectedFilterIndex;
    {
        QSettings settings;
        settings.beginGroup(u"Screen"_s);
        settings.setValue(u"softwareFilter"_s, presets[std::size_t(selectedFilterIndex)].id);
    }
    emit selectedFilterIndexChanged();
}

QStringList ZxEmulator::joysticksModel() const
{
    // The order of JoystickKind
    return { u"None"_s,
             u"Cursor"_s,
             u"Kempston"_s,
             u"Sinclair 1"_s,
             u"Sinclair 2"_s,
             u"Fuller"_s,
             u"Timex 1 (TC/TS2068)"_s,
             u"Timex 2 (TC/TS2068)"_s };
}

int ZxEmulator::selectedJoysticksIndex() const
{
    return m_selectedJoystickIndex;
}

void ZxEmulator::setSelectedJoysticksIndex(int selectedJoysticksIndex)
{
    if (selectedJoysticksIndex < NoJoystick || selectedJoysticksIndex > Timex2)
        return;
    if (!joystickAvailable(selectedJoysticksIndex)) {
        showMessage(tr("Timex joysticks require a TC2068 or TS2068 machine"), Info);
        return;
    }
    m_joystickPromptTimer.stop();
    if (selectedJoysticksIndex == m_selectedJoystickIndex) {
        attachJoystick(selectedJoysticksIndex);
        return;
    }
    releaseInputs();
    setJoystickButtons(0); // release everything on the old one
    m_selectedJoystickIndex = selectedJoysticksIndex;
    attachJoystick(selectedJoysticksIndex);
    if (!m_settings->controllerOverride())
        QSettings().setValue("joystick", selectedJoysticksIndex);
    emit selectedJoysticksIndexChanged();
}

bool ZxEmulator::joystickAvailable(int index) const
{
    if (index < NoJoystick || index > Timex2)
        return false;
    if (index != Timex1 && index != Timex2)
        return true;
    const auto core = emulator().readLock();
    const auto *timex = dynamic_cast<const zxkit::TimexMachine *>(&core->machine());
    return timex && timex->hasTimexJoysticks();
}

void ZxEmulator::attachJoystick(int kind)
{
    // The Sinclair (Interface 2) and Cursor joysticks are keys; the Kempston and Fuller ones are interfaces
    if (kind == KempstonJoystick)
        m_settings->setKempstonJoystick(true);
    withEmulator([kind](zxkit::Emulator &emulator) {
        if (kind == KempstonJoystick && !emulator.findPeripheral("kempston"sv))
            emulator.attachPeripheral(std::make_unique<zxkit::KempstonJoystickPeripheral>());
        const bool attached = emulator.findPeripheral("fuller"sv).has_value();
        if (kind == FullerJoystick && !attached)
            emulator.attachPeripheral(std::make_unique<zxkit::FullerBoxPeripheral>());
        else if (kind != FullerJoystick && attached)
            emulator.detachPeripheral("fuller"sv);
    });
}

void ZxEmulator::setJoystickButtons(uint8_t buttons)
{
    using B = zxkit::JoystickButtons;
    using K = zxkit::ZxKey;
    const uint8_t changed = buttons ^ m_joystickButtons;
    m_joystickButtons = buttons;
    if (!changed)
        return;

    // Left, right, up, down, fire (FUSE's tables)
    static constexpr std::array<uint8_t, 5> kBits{ B::Left, B::Right, B::Up, B::Down, B::Fire1 };
    static constexpr std::array<K, 5> kCursorKeys{ K::Num5, K::Num8, K::Num7, K::Num6, K::Num0 };
    static constexpr std::array<K, 5> kSinclair1Keys{ K::Num6, K::Num7, K::Num9, K::Num8, K::Num0 };
    static constexpr std::array<K, 5> kSinclair2Keys{ K::Num1, K::Num2, K::Num4, K::Num3, K::Num5 };

    std::span<const K> keys;
    switch (m_selectedJoystickIndex) {
    case CursorKeys:
        keys = kCursorKeys;
        break;
    case Sinclair1:
        keys = kSinclair1Keys;
        break;
    case Sinclair2:
        keys = kSinclair2Keys;
        break;
    }
    if (!keys.empty()) {
        QSet<int> held;
        for (std::size_t i = 0; i < kBits.size(); ++i)
            if (buttons & kBits[i])
                held.insert(int(keys[i]));
        setInputKeys(kJoystickKeys, held);
        return;
    }
    auto emulator = this->emulator().writeLock();
    switch (m_selectedJoystickIndex) {
    case KempstonJoystick:
        withPeripheral<zxkit::KempstonJoystickPeripheral>(
                *emulator, "kempston"sv,
                [buttons](auto &kempston) { kempston.setButtons(buttons); });
        break;
    case FullerJoystick:
        withPeripheral<zxkit::FullerBoxPeripheral>(
                *emulator, "fuller"sv, [buttons](auto &fuller) { fuller.setButtons(buttons); });
        break;
    case Timex1:
    case Timex2:
        if (auto *timex = dynamic_cast<zxkit::TimexMachine *>(&emulator->machine()))
            timex->setJoystickButtons(m_selectedJoystickIndex == Timex1 ? 0 : 1, buttons);
        break;
    default:
        break;
    }
}

QString ZxEmulator::PC() const
{
    return formatNumber(registers().PC);
}
void ZxEmulator::setPC(const QString &value)
{
    setRegisterValue(PC = uint16_t(val));
}

QString ZxEmulator::SP() const
{
    return formatNumber(registers().SP);
}
void ZxEmulator::setSP(const QString &value)
{
    setRegisterValue(SP = uint16_t(val));
}

QString ZxEmulator::IX() const
{
    return formatNumber(registers().IX);
}
void ZxEmulator::setIX(const QString &value)
{
    setRegisterValue(IX = uint16_t(val));
}

QString ZxEmulator::IY() const
{
    return formatNumber(registers().IY);
}
void ZxEmulator::setIY(const QString &value)
{
    setRegisterValue(IY = uint16_t(val));
}

QString ZxEmulator::R() const
{
    return formatNumber(registers().R);
}
void ZxEmulator::setR(const QString &value)
{
    setRegisterValue(R = uint8_t(val & 0x7F));
}

QString ZxEmulator::I() const
{
    return formatNumber(registers().I);
}
void ZxEmulator::setI(const QString &value)
{
    setRegisterValue(I = uint8_t(val));
}

QString ZxEmulator::R7() const
{
    return formatNumber(registers().R7);
}
void ZxEmulator::setR7(const QString &value)
{
    setRegisterValue(R7 = uint8_t(val & 0x80));
}

QString ZxEmulator::IFF1() const
{
    return formatNumber(registers().IFF1);
}
void ZxEmulator::setIFF1(const QString &value)
{
    setRegisterValue(IFF1 = uint8_t(val));
}

QString ZxEmulator::IFF2() const
{
    return formatNumber(registers().IFF2);
}
void ZxEmulator::setIFF2(const QString &value)
{
    setRegisterValue(IFF2 = uint8_t(val));
}

QString ZxEmulator::IM() const
{
    return formatNumber(registers().IM);
}
void ZxEmulator::setIM(const QString &value)
{
    setRegisterValue(IM = uint8_t(val & 0x03));
}

QString ZxEmulator::AF() const
{
    return formatNumber(registers().AF);
}
void ZxEmulator::setAF(const QString &value)
{
    setRegisterValue(AF = uint16_t(val));
}

QString ZxEmulator::BC() const
{
    return formatNumber(registers().BC);
}
void ZxEmulator::setBC(const QString &value)
{
    setRegisterValue(BC = uint16_t(val));
}

QString ZxEmulator::DE() const
{
    return formatNumber(registers().DE);
}
void ZxEmulator::setDE(const QString &value)
{
    setRegisterValue(DE = uint16_t(val));
}

QString ZxEmulator::HL() const
{
    return formatNumber(registers().HL);
}
void ZxEmulator::setHL(const QString &value)
{
    setRegisterValue(HL = uint16_t(val));
}

QString ZxEmulator::AF_() const
{
    return formatNumber(registers().AF_);
}
void ZxEmulator::setAF_(const QString &value)
{
    setRegisterValue(AF_ = uint16_t(val));
}

QString ZxEmulator::BC_() const
{
    return formatNumber(registers().BC_);
}
void ZxEmulator::setBC_(const QString &value)
{
    setRegisterValue(BC_ = uint16_t(val));
}

QString ZxEmulator::DE_() const
{
    return formatNumber(registers().DE_);
}
void ZxEmulator::setDE_(const QString &value)
{
    setRegisterValue(DE_ = uint16_t(val));
}

QString ZxEmulator::HL_() const
{
    return formatNumber(registers().HL_);
}
void ZxEmulator::setHL_(const QString &value)
{
    setRegisterValue(HL_ = uint16_t(val));
}

zxkit::Z80Registers ZxEmulator::registers() const
{
    return emulator().readLock()->registers();
}

DebuggerMemoryModel *ZxEmulator::debugMemory()
{
    if (!m_debugMemory) {
        m_debugMemory = std::make_unique<DebuggerMemoryModel>(emulator());
        connect(m_debugMemory.get(), &DebuggerMemoryModel::errorChanged, this, [this] {
            if (m_debugMemory->error().isEmpty())
                m_engine->refreshFrame();
        });
    }
    m_debugMemory->setEditable(debuggerReady());
    return m_debugMemory.get();
}
DebuggerStateModel *ZxEmulator::debugNextRegisters()
{
    if (!m_debugNextRegisters)
        m_debugNextRegisters = std::make_unique<DebuggerStateModel>(
                emulator(), DebuggerStateModel::Kind::NextRegisters);
    return m_debugNextRegisters.get();
}
DebuggerStateModel *ZxEmulator::debugMmu()
{
    if (!m_debugMmu)
        m_debugMmu =
                std::make_unique<DebuggerStateModel>(emulator(), DebuggerStateModel::Kind::Mmu);
    return m_debugMmu.get();
}
QString ZxEmulator::debuggerStatus() const
{
    if (!debuggerReady())
        return paused() ? tr("Waiting for emulation to pause…")
                        : tr("Running — views show the last paused state");
    QString reason;
    switch (m_debugPause.reason) {
    case zxkit::PauseReason::ExecutionBreakpoint:
        reason = tr("Execution breakpoint");
        break;
    case zxkit::PauseReason::MemoryWatchpoint:
        reason = tr("Memory watchpoint at %1").arg(formatNumber(m_debugPause.address));
        break;
    case zxkit::PauseReason::PortWatchpoint:
        reason = tr("Port watchpoint at %1").arg(formatNumber(m_debugPause.address));
        break;
    case zxkit::PauseReason::NextRegWatchpoint:
        reason = tr("Next register write at %1").arg(formatNumber(m_debugPause.address));
        break;
    case zxkit::PauseReason::RasterBreakpoint:
        reason = tr("Raster line %1").arg(m_debugPause.address);
        break;
    case zxkit::PauseReason::EventTrap:
        reason = m_debugPause.address ? tr("NMI accepted") : tr("Interrupt accepted");
        break;
    case zxkit::PauseReason::StepComplete:
        reason = tr("Step complete");
        break;
    default:
        reason = tr("Paused");
        break;
    }
    return reason + tr(" · PC %1").arg(PC());
}
void ZxEmulator::updateDebugger()
{
    m_breakpointsModel.breakpointsUpdated();
    m_disassambleModel.disassamble(registers().PC);
    if (m_debugMemory) {
        m_debugMemory->setEditable(debuggerReady());
        m_debugMemory->refresh();
    }
    if (m_debugNextRegisters)
        m_debugNextRegisters->refresh();
    if (m_debugMmu)
        m_debugMmu->refresh();
    emit registersChanged();
    emit debuggerStatusChanged();
}
void ZxEmulator::debuggerRefresh()
{
    if (debuggerReady()) {
        updateDebugger();
        m_engine->refreshFrame();
    }
}

int ZxEmulator::pokeFinderCount() const
{
    return int(emulator().readLock()->debugger().pokeFinder().count());
}

bool ZxEmulator::loadMediaFile(const QString &filePath)
{
    return loadMediaWithType(filePath).first;
}

QString ZxEmulator::importFile(const QUrl &url)
{
    if (url.scheme() != u"content"_s)
        return url.isLocalFile() ? url.toLocalFile() : url.toString();
#ifdef Q_OS_ANDROID
    const auto name = AndroidIntegration::displayName(url);
#else
    const auto name = QFileInfo(url.path()).fileName();
#endif
    QFile input(url.toString());
    if (!input.open(QIODevice::ReadOnly)) {
        showMessage(tr("Can't open '%1': %2").arg(name, input.errorString()), Error);
        return {};
    }
    const QDir folder(QDir(dataPath()).filePath(u"Imported"_s));
    if (!folder.mkpath(u"."_s)) {
        showMessage(tr("Can't create the Imported folder."), Error);
        return {};
    }
    const QFileInfo info(saveName(name, u"Imported"_s));
    QString path = folder.filePath(info.fileName());
    for (unsigned n = 1; QFileInfo::exists(path); ++n)
        path = folder.filePath(info.completeBaseName() + u'_' + QString::number(n)
                               + (info.suffix().isEmpty() ? QString{} : u'.' + info.suffix()));
    QSaveFile output(path);
    if (output.open(QIODevice::WriteOnly)) {
        bool written = true;
        for (;;) {
            const auto bytes = input.read(64 * 1024);
            if (input.error() != QFileDevice::NoError) {
                showMessage(tr("Can't read '%1': %2").arg(name, input.errorString()), Error);
                return {};
            }
            if (bytes.isEmpty())
                break;
            if (output.write(bytes) != bytes.size()) {
                written = false;
                break;
            }
        }
        if (written && output.commit())
            return path;
    }
    showMessage(tr("Can't import '%1': %2").arg(name, output.errorString()), Error);
    return {};
}

std::pair<bool, zxkit::MediaClass> ZxEmulator::loadMediaWithType(const QString &filePath)
{
    m_joystickPromptTimer.stop();
    zxkit::MediaFile file;
    zxkit::SnapshotJoystickInfo joystick;
    try {
        // The slow part (reading, unzipping) runs before taking the lock
        const QUrl url(filePath);
        QFile input(url.isLocalFile() ? url.toLocalFile() : filePath);
        if (!input.open(QIODevice::ReadOnly)) {
            showMessage(tr("Can't open '%1': %2").arg(filePath, input.errorString()), Error);
            return { false, zxkit::MediaClass::Unknown };
        }
        const auto bytes = input.readAll();
        if (input.error() != QFileDevice::NoError) {
            showMessage(tr("Can't read '%1': %2").arg(filePath, input.errorString()), Error);
            return { false, zxkit::MediaClass::Unknown };
        }
        // QFile supports Android's content providers; std::ifstream does not.
#ifdef Q_OS_ANDROID
        const auto name = AndroidIntegration::displayName(url);
#else
        const auto name = QFileInfo(input.fileName()).fileName();
#endif
        file = zxkit::MediaFile::fromBytes(std::vector<uint8_t>(bytes.begin(), bytes.end()),
                                           QFileInfo(name).suffix().toStdString());
        joystick = zxkit::SnapshotFormats::joystickInfo(file.data, file.extension);
    } catch (const std::exception &e) {
        showMessage(QString::fromUtf8(e.what()), Error);
        return { false, zxkit::MediaClass::Unknown };
    }
    releaseInputs();
    const bool loaded =
            withEmulator([&file](zxkit::Emulator &emulator) { emulator.loadMedia(file); });
    if (loaded) {
        m_pokes->gameLoaded(filePath);
        m_symbols.clear();
        const auto id = std::string(emulator().readLock()->machine().id());
        if (m_settings)
            m_settings->syncCurrentMachine(id);
        m_engine->refreshFrame();
        const bool profileApplied = activateControllerGame(m_loadedFileName);
        if (zxkit::classifyMediaFile(file.extension, file.data) == zxkit::MediaClass::Snapshot) {
            std::optional<int> selected = std::nullopt;
            if (joystick.type) {
                using J = zxkit::JoystickType;
                switch (*joystick.type) {
                case J::None:
                    selected = NoJoystick;
                    break;
                case J::Cursor:
                    selected = CursorKeys;
                    break;
                case J::Kempston:
                    selected = KempstonJoystick;
                    break;
                case J::Sinclair1:
                    selected = Sinclair1;
                    break;
                case J::Sinclair2:
                    selected = Sinclair2;
                    break;
                case J::Fuller:
                    selected = FullerJoystick;
                    break;
                case J::Timex1:
                    selected = Timex1;
                    break;
                case J::Timex2:
                    selected = Timex2;
                    break;
                default:
                    break;
                }
            }
            if (selected && joystickAvailable(*selected)) {
                setSelectedJoysticksIndex(*selected);
            } else if (!profileApplied && m_settings && m_settings->joystickPrompt()) {
                // File pickers clear their page after load() returns. Also allow
                // time for the selecting touch/button to be released.
                m_joystickPromptTimer.start();
            }
        }
    }
    return { loaded, zxkit::classifyMediaFile(file.extension, file.data) };
}

bool ZxEmulator::saveMediaFile(const QString &filePath)
{
    std::vector<uint8_t> bytes;
#ifdef Q_OS_ANDROID
    const auto name = AndroidIntegration::displayName(QUrl(filePath));
#else
    const auto url = QUrl(filePath);
    const auto name = url.isLocalFile() ? url.toLocalFile() : filePath;
#endif
    const QString suffix = QFileInfo(name).suffix();
    if (suffix.compare(u"png"_s, Qt::CaseInsensitive) == 0) {
        // Save the native cropped picture, before software filters or window scaling.
        QByteArray encoded;
        QBuffer buffer(&encoded);
        buffer.open(QIODevice::WriteOnly);
        if (!ZxImage::instance()->image().save(&buffer, "PNG")) {
            showMessage(tr("Can't encode the screenshot"), Error);
            return false;
        }
        bytes.assign(encoded.begin(), encoded.end());
    } else if (!withEmulator([&bytes, &suffix](zxkit::Emulator &emulator) {
                   bytes = emulator.saveMedia(suffix.toStdString());
               })) {
        return false;
    }
    const auto extension = (u"."_s + suffix.toLower()).toStdString();
    if (extension == ".szx"sv || extension == ".z80"sv) {
        static constexpr std::array types{
            zxkit::JoystickType::None,      zxkit::JoystickType::Cursor,
            zxkit::JoystickType::Kempston,  zxkit::JoystickType::Sinclair1,
            zxkit::JoystickType::Sinclair2, zxkit::JoystickType::Fuller,
            zxkit::JoystickType::Timex1,    zxkit::JoystickType::Timex2
        };
        zxkit::SnapshotFormats::setJoystickInfo(bytes, extension, types[m_selectedJoystickIndex]);
    }
    if (!writeFileBytes(filePath, bytes))
        return false;
    const auto profile = m_controllerProfiles->gameProfile();
    if (!profile.isEmpty()
        && (extension == ".szx"sv || extension == ".z80"sv || extension == ".sna"sv
            || extension == ".nex"sv))
        m_controllerProfiles->rememberGame(QFileInfo(name).completeBaseName(), profile);
    return true;
}

bool ZxEmulator::writeFileBytes(const QString &filePath, std::span<const uint8_t> bytes)
{
    const QUrl url(filePath);
    const auto path = url.isLocalFile() ? url.toLocalFile() : filePath;
    const QDir folder = QFileInfo(path).absoluteDir();
    const bool contentUri = filePath.startsWith(u"content://"_s);
    if (!contentUri && !folder.mkpath(u"."_s)) {
        const QString message = tr("Can't create folder '%1'").arg(folder.absolutePath());
        showMessage(message, Error);
        return false;
    }
    QSaveFile file(path);
    // A document provider cannot atomically rename a temporary file. Qt writes
    // directly to the document explicitly selected by the user in that case.
    file.setDirectWriteFallback(contentUri);
    if (!file.open(QIODevice::WriteOnly)
        || file.write(reinterpret_cast<const char *>(bytes.data()), qint64(bytes.size()))
                != qint64(bytes.size())
        || !file.commit()) {
        const QString message = tr("Can't save '%1': %2").arg(filePath, file.errorString());
        showMessage(message, Error);
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
    if (!processEvents)
        releaseInputs();
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

    m_fastForwardControl.clearPhysical();
    syncFastForward();
    m_controller.clearPhysical();
    m_pressedRzxButtons.clear();
    m_rzxAxisActive.fill(false);
    applyControllerState();
    m_gamepadId = gamepadId;
    if (m_gamepadId != -1)
        emit error(Info, tr("A new default gamepad was selected"));
    else
        emit error(Warning, tr("No gamepad was found, please connect a gamepad"));

    emit gamepadIdChanged();
}

QString ZxEmulator::saveFilePath(const QString &fileName)
{
    return dataPath() + u"/Downloaded/"_s + fileName.left(1).toLower() + u'/'
            + fileName.left(3).toLower() + u'/' + fileName;
}

void ZxEmulator::mouseButton(Qt::MouseButton button, bool pressed)
{
    if (!m_processInputEvents)
        return;

    uint8_t bit = 0;
    switch (button) {
    case Qt::RightButton:
        bit = 0x01;
        break;
    case Qt::LeftButton:
        bit = 0x02;
        break;
    case Qt::MiddleButton:
        bit = 0x04;
        break;
    default:
        return;
    }

    withPeripheral<zxkit::KempstonMousePeripheral>(
            *emulator().writeLock(), "kempston_mouse"sv,
            [bit, pressed](auto &mouse) { mouse.setButton(bit, pressed); });
}

void ZxEmulator::mouseMotion(int dx, int dy)
{
    if (!m_processInputEvents)
        return;

    withPeripheral<zxkit::KempstonMousePeripheral>(*emulator().writeLock(), "kempston_mouse"sv,
                                                   [dx, dy](auto &mouse) { mouse.move(dx, dy); });
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
    m_joystickPromptTimer.stop();
    activateControllerGame(QString{});
    m_loadedFileName = "";
    if (m_pokes)
        m_pokes->gameLoaded({});
    m_symbols.clear();
    emit saveSnapshotEnabledChanged();
    m_recording->refresh();
}

void ZxEmulator::quit()
{
    if (m_settings->autoSaveOnExit())
        quickSaveSnapshot();

    QCoreApplication::quit();
}

QString ZxEmulator::snapshotsPath() const
{
    return QDir(dataPath()).filePath(u"Snapshots/"_s);
}

QString ZxEmulator::recordingsPath() const
{
    return QDir(dataPath()).filePath(u"Recordings/"_s);
}

QString ZxEmulator::savesPath() const
{
    return QDir(dataPath()).filePath(u"Saves/"_s);
}

QString ZxEmulator::recordingFilePath() const
{
    return QDir(recordingsPath())
            .filePath(saveName(m_loadedFileName, u"Generic"_s) + u'.' + saveTimestamp()
                      + u".rzx"_s);
}

QString ZxEmulator::tapeFileName(const QString &programName) const
{
    return saveName(programName, u"Unnamed"_s) + u'_' + saveTimestamp() + u".tap"_s;
}

QString ZxEmulator::tapeFilePath(const QString &fileName) const
{
    QString name = fileName.trimmed();
    const QString suffix = name.endsWith(u".tzx"_s, Qt::CaseInsensitive) ? u".tzx"_s : u".tap"_s;
    if (name.endsWith(suffix, Qt::CaseInsensitive))
        name.chop(4);
    return QDir(savesPath()).filePath(saveName(name, u"Unnamed"_s) + suffix);
}

QStringList ZxEmulator::snapshotFormats() const
{
    const auto id = emulator().readLock()->machine().id();
    QStringList result = { u"szx"_s };
    if (id == "next"sv)
        result << u"nex"_s;
    if (id == "48k"sv || id == "128k"sv || id == "plus2"sv || id == "plus2a"sv || id == "plus3"sv
        || id == "plus3e"sv || id == "pentagon"sv)
        result << u"z80"_s;
    if (id == "48k"sv || id == "128k"sv || id == "plus2"sv || id == "pentagon"sv)
        result << u"sna"_s;
    return result;
}

QString ZxEmulator::snapshotFileName() const
{
    return saveName(m_loadedFileName, u"Snapshot"_s) + u".szx"_s;
}

QString ZxEmulator::snapshotFilePath(const QString &folder, const QString &fileName) const
{
    QString name = fileName.trimmed();
    QString suffix = u".szx"_s;
    for (const auto &extension : { u".szx"_s, u".z80"_s, u".sna"_s, u".nex"_s }) {
        if (name.endsWith(extension, Qt::CaseInsensitive)) {
            suffix = extension;
            name.chop(extension.size());
            break;
        }
    }
    return QDir(folder.trimmed().isEmpty() ? snapshotsPath() : folder)
            .filePath(saveName(name, u"Snapshot"_s) + suffix);
}

void ZxEmulator::updateMediaActivity(const zxkit::MediaActivity &activity)
{
    const zxkit::MediaActivity previous = std::exchange(m_mediaActivity, activity);
    m_tapeFastForward = activity.tapePlaying && m_settings->loaderAcceleration() > 1;
    updateFastForwardIndicator();
    if (activity.tapePlaying != previous.tapePlaying) {
        emit uiIconUpdate(Tape, activity.tapePlaying ? Active : Inactive);
        m_tape->setPlaying(activity.tapePlaying);
    }
    // One icon for all the storage: the floppy drives and the SD card
    const bool storageBusy = activity.busyDisks != 0 || activity.sdCardBusy;
    if (storageBusy != (previous.busyDisks != 0 || previous.sdCardBusy))
        emit uiIconUpdate(Disk, storageBusy ? Active : Inactive);
    if (activity.tapeBlock != previous.tapeBlock) {
        m_tape->refreshData();
        emit tapeBlockChanged(int(activity.tapeBlock));
    }
}

void ZxEmulator::load(const QString &filePath, bool removeOnFail)
{
    const auto previousName = m_loadedFileName;
    m_loadedFileName = QFileInfo(filePath).completeBaseName();
    const auto [loaded, mediaClass] = loadMediaWithType(filePath);
    m_recording->refresh();
    if (!loaded) {
        m_loadedFileName = previousName;
        if (removeOnFail) {
            QFile::remove(filePath);
            emit error(Info, tr("File \"%1\" was removed").arg(filePath));
        }
        emit saveSnapshotEnabledChanged();
        return;
    }

    m_recording->mediaLoaded();

    if (m_settings && m_settings->autoLoad()) {
        if (mediaClass == zxkit::MediaClass::Tape)
            withEmulator([](zxkit::Emulator &emulator) { emulator.autoLoadTape(); });
        else if (mediaClass == zxkit::MediaClass::Disk)
            withEmulator([](zxkit::Emulator &emulator) { emulator.autoBootDisk(); });
        m_engine->refreshFrame();
    }

    m_resetPokeFinder = true;
    m_tape->refreshData();
    emit saveSnapshotEnabledChanged();
}

bool ZxEmulator::save(const QString &filePath)
{
    if (!saveMediaFile(filePath))
        return false;
    showMessage(tr("File saved to '%1'").arg(filePath), Info);
    return true;
}

bool ZxEmulator::copyToFavourites(const QString &filePath)
{
    QDir d(dataPath());
    d.mkpath(u"Favourites"_s);
    const QString dest = d.filePath(u"Favourites/"_s + QFileInfo(filePath).fileName());
    if (!QFile::exists(dest) && !QFile::copy(filePath, dest)) {
        emit error(Error, tr("Can't copy \"%1\" to \"%2\"").arg(filePath).arg(dest));
        return false;
    }
    load(dest);
    return !m_loadedFileName.isEmpty();
}

void ZxEmulator::remove(const QString &path)
{
    QFileInfo inf(path);
    if (inf.isDir()) {
        QDir dir(path);
        for (const QFileInfo &item :
             dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot))
            remove(item.absoluteFilePath());
        dir.rmdir(path);
    } else {
        QFile::remove(path);
    }
}

bool ZxEmulator::isFolder(const QString &path)
{
    return QFileInfo{ path }.isDir();
}

QString ZxEmulator::fileName(const QString &path)
{
    return QFileInfo{ path }.fileName();
}

QString ZxEmulator::folderName(const QString &path)
{
    return QFileInfo{ path }.absolutePath();
}

bool ZxEmulator::copy(const QString &from, const QString &to, bool override)
{
    QFileInfo inf{ from };
    if (inf.isDir()) {
        QDir src{ from };
        QDir dst{ to + QDir::separator() + src.dirName() };
        if (!override && dst.exists())
            return false;

        QDirIterator it{ src.absolutePath(), QDir::Files, QDirIterator::Subdirectories };
        while (it.hasNext()) {
            it.next();
            const auto item = it.fileInfo();
            auto path = dst.absolutePath() + QDir::separator()
                    + src.relativeFilePath(item.absolutePath());
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
    releaseInputs();
    withEmulator([](zxkit::Emulator &core) { core.reset(zxkit::ResetKind::Soft); });
    m_recording->refresh();
    m_engine->refreshFrame();
}

void ZxEmulator::hardReset()
{
    resetLoadedFile();
    releaseInputs();
    withEmulator([](zxkit::Emulator &core) { core.reset(zxkit::ResetKind::Hard); });
    m_recording->refresh();
    m_engine->refreshFrame();
}

bool ZxEmulator::canNextNmi(bool multiface) const
{
    return emulator().readLock()->canTriggerNextNmi(multiface);
}

void ZxEmulator::nextNmi(bool multiface)
{
    if (!emulator().writeLock()->triggerNextNmi(multiface))
        showMessage(tr("This Next NMI button is disabled or another device is handling NMI."));
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

    const QString base = saveName(m_loadedFileName, u"Snapshot"_s);
    const QString name = base + u'/' + base + u'.' + saveTimestamp() + u".szx"_s;
    save(QDir(snapshotsPath()).filePath(name));
}

void ZxEmulator::quickLoadSnapshot()
{
    QFileInfo latest;
    QDirIterator files(snapshotsPath(), { u"*.szx"_s, u"*.sna"_s, u"*.z80"_s }, QDir::Files,
                       QDirIterator::Subdirectories);
    while (files.hasNext()) {
        files.next();
        const auto candidate = files.fileInfo();
        if (latest.filePath().isEmpty() || candidate.lastModified() > latest.lastModified())
            latest = candidate;
    }
    if (!latest.filePath().isEmpty()) {
        // User snapshots are never deleted when loading fails.
        load(latest.absoluteFilePath());
        if (!m_loadedFileName.isEmpty())
            showMessage(tr("Snapshot loaded from '%1'").arg(latest.fileName()));
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
    const bool wasActive = m_debuggerActivated.exchange(true);
    if (!m_debuggerOwnsPause) {
        m_debuggerOwnsPause = true;
        setPaused(true);
    } else if (!debuggerReady())
        m_engine->setPaused(true);
    if (debuggerReady())
        updateDebugger();
    emit debuggerStatusChanged();
    if (!wasActive)
        emit showDebugger();
}

void ZxEmulator::deactivateDebugger(bool /*interruptable*/)
{
    if (!m_debuggerActivated.exchange(false))
        return;
    debuggerContinue();
    emit hideDebugger();
}

QString ZxEmulator::dumpData(const QString &base64CompressedData, int offset, int charsPerLine)
{
    if (base64CompressedData.isEmpty())
        return QString();

    auto data = qUncompress(
            QByteArray::fromBase64(base64CompressedData.toLatin1(), QByteArray::Base64UrlEncoding));

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

void ZxEmulator::applyKeyChanges(const HeldKeyState::Changes &changes)
{
    if (changes.isEmpty() || !m_engine)
        return;
    auto emulator = this->emulator().writeLock();
    for (const auto &[key, pressed] : changes) {
        if (key >= 0x100 && key < 0x110)
            emulator->setNextKeyState(static_cast<zxkit::NextKey>(key - 0x100), pressed);
        else
            emulator->setKeyState(static_cast<zxkit::ZxKey>(key), pressed);
    }
}

void ZxEmulator::setInputKeys(quint64 source, const QSet<int> &keys)
{
    applyKeyChanges(m_heldKeys.setSource(source, keys));
}

void ZxEmulator::releaseInputs()
{
    m_fastForwardControl.clear();
    syncFastForward();
    m_controller.clear();
    applyControllerState();
    applyKeyChanges(m_heldKeys.clear());
}

void ZxEmulator::configureController()
{
    m_fastForwardControl.clear();
    m_fastForwardControl.setToggle(m_settings->fastForwardToggle());
    syncFastForward();
    m_controller.configure(m_settings->directionMapping(), m_settings->directionKeys(),
                           m_settings->deadZone());
    applyControllerState();
}

void ZxEmulator::applyControllerState()
{
    const auto state = m_controller.state();
    QSet<int> keys;
    for (const int key : state.keys) {
        const auto mapped = mapQtKeyToZx(key, 0);
        if (mapped.primary)
            keys.insert(int(*mapped.primary));
        if (mapped.modifier)
            keys.insert(int(*mapped.modifier));
    }
    setInputKeys(kControllerKeys, keys);
    setJoystickButtons(state.joystick);
}

void ZxEmulator::setNextKeyState(int key, bool pressed)
{
    if (key < 0 || key >= 16 || (pressed && m_paused))
        return;
    setInputKeys((quint64(7) << 32) | unsigned(key),
                 pressed ? QSet<int>{ 0x100 + key } : QSet<int>{});
}

void ZxEmulator::setCobraKeyState(int qtKey, bool pressed)
{
    if (pressed) {
        if (m_paused)
            return;
        const auto emulator = this->emulator().readLock();
        if (!hasCobraKeyboard(*emulator))
            return;
    }
    QSet<int> keys;
    if (pressed)
        if (const auto key = cobraKey(qtKey, Qt::KeypadModifier))
            keys.insert(int(*key));
    setInputKeys(kCobraKeys | quint32(qtKey), keys);
}

void ZxEmulator::keyPress(int qtKey, int modifiers, bool autoRepeat, bool virtualKeyboard)
{
    if (m_paused || autoRepeat)
        return;
    if (!virtualKeyboard && m_settings->recreatedSpectrum()) {
        if (const auto key = recreatedKey(qtKey, modifiers))
            setInputKeys(kRecreatedKeys | int(key->first),
                         key->second ? QSet<int>{ int(key->first) } : QSet<int>{});
        return;
    }
    QSet<int> keys;
    bool cobra = false;
    std::optional<zxkit::NextKey> nextKey;
    if (!virtualKeyboard) {
        const auto emulator = this->emulator().readLock();
        cobra = hasCobraKeyboard(*emulator);
        if (emulator->machine().id() == "next"sv)
            nextKey = nextExtendedKey(qtKey);
    }
    if (nextKey) {
        keys.insert(0x100 + int(*nextKey));
    } else if (const auto key = cobra ? cobraKey(qtKey, modifiers) : std::nullopt) {
        keys.insert(int(*key));
    } else {
        const auto mapped = mapQtKeyToZx(qtKey, modifiers);
        if (mapped.modifier)
            keys.insert(int(*mapped.modifier));
        if (mapped.primary)
            keys.insert(int(*mapped.primary));
    }
    const bool hostModifier = !virtualKeyboard
            && (qtKey == Qt::Key_Shift || qtKey == Qt::Key_Control || qtKey == Qt::Key_Alt
                || qtKey == Qt::Key_Meta);
    if (!keys.isEmpty() && !hostModifier)
        m_recording->resumeOnGameplayInput();
    setInputKeys((virtualKeyboard ? kVirtualKeys : kPhysicalKeys) | quint32(qtKey), keys);
}

void ZxEmulator::keyRelease(int qtKey, int modifiers, bool autoRepeat, bool virtualKeyboard)
{
    Q_UNUSED(modifiers)
    // Releases remain valid after pausing or opening a page. The stored press
    // owns its modifiers, even if the host release reports different ones.
    if (autoRepeat || (!virtualKeyboard && m_settings->recreatedSpectrum()))
        return;
    setInputKeys((virtualKeyboard ? kVirtualKeys : kPhysicalKeys) | quint32(qtKey), {});
}

void ZxEmulator::gamepadAxisEvent(QGamepadManager::GamepadAxis axis, double value, bool touch)
{
    ControllerInput::Axis target;
    switch (axis) {
    case QGamepadManager::AxisLeftX:
        target = touch ? ControllerInput::TouchX : ControllerInput::LeftX;
        break;
    case QGamepadManager::AxisLeftY:
        target = touch ? ControllerInput::TouchY : ControllerInput::LeftY;
        break;
    case QGamepadManager::AxisRightX:
        if (touch)
            return;
        target = ControllerInput::RightX;
        break;
    case QGamepadManager::AxisRightY:
        if (touch)
            return;
        target = ControllerInput::RightY;
        break;
    default:
        return;
    }
    const bool active = std::abs(value) > m_settings->deadZone();
    const bool fresh = active && !m_rzxAxisActive[target];
    m_rzxAxisActive[target] = active;
    const bool resumed = fresh && m_recording->resumeOnGameplayInput();
    if ((m_pauseCount || m_paused || !m_processInputEvents) && !resumed) {
        // A release can arrive before the queued transport resume.
        if (!active) {
            m_controller.axis(target, 0);
            applyControllerState();
        }
        return;
    }
    m_controller.axis(target, value);
    applyControllerState();
}

void ZxEmulator::gamepadButtonPressEvent(QGamepadManager::GamepadButton button, bool touch)
{
    if (button == QGamepadManager::ButtonInvalid || button == QGamepadManager::ButtonStart)
        return;
    const int inputId = int(button) + (touch ? ControllerInput::TouchButtonOffset : 0);
    const bool fresh = !m_pressedRzxButtons.contains(inputId);
    m_pressedRzxButtons.insert(inputId);
    if (!m_processInputEvents)
        return;
    const int action = m_settings->gamepadAction(button);
    const bool resumed = fresh
            && (action == ZxSettings::JoystickFire || ControllerInput::isSpectrumKey(action))
            && m_recording->resumeOnGameplayInput();
    if ((m_pauseCount || m_paused) && !resumed
        && (action != ZxSettings::TogglePause || (!m_paused && !m_recording->waitingForInput())
            || m_pauseCount != 1))
        return;
    const quint8 direction = button == QGamepadManager::ButtonUp
                    || button == QGamepadManager::ButtonDown
                    || button == QGamepadManager::ButtonLeft
                    || button == QGamepadManager::ButtonRight
            ? toJoystickMask(button)
            : 0;
    if (action == ZxSettings::ToggleEmulationSpeed && fresh) {
        m_fastForwardControl.press(inputId);
        syncFastForward();
    }
    m_controller.press(inputId, action, direction);
    applyControllerState();
}

void ZxEmulator::gamepadButtonReleaseEvent(QGamepadManager::GamepadButton button, bool touch,
                                           bool cancelled)
{
    const int source = int(button) + (touch ? ControllerInput::TouchButtonOffset : 0);
    m_fastForwardControl.release(source, cancelled);
    syncFastForward();
    m_pressedRzxButtons.remove(source);
    const auto action =
            m_controller.release(int(button) + (touch ? ControllerInput::TouchButtonOffset : 0));
    applyControllerState();
    if (!action || cancelled)
        return; // A page, mapping change or disconnect may have cleared the press.
    switch (*action) {
    case ZxSettings::ToggleKeyboard:
    case ZxSettings::ToggleCursorJoystick:
        if (m_paused)
            return;
        QMetaObject::invokeMethod(
                this,
                [this, action] {
                    emit toggleOnScreenControls(
                            *action == ZxSettings::ToggleKeyboard ? Keyboard48K : CursorJoystick);
                },
                Qt::QueuedConnection);
        break;
    case ZxSettings::TogglePause:
        togglePaused();
        break;
    case ZxSettings::QuickSaveSnapshot:
        if (!m_paused)
            quickSaveSnapshot();
        break;
    case ZxSettings::QuickLoadSnapshot:
        if (!m_paused)
            quickLoadSnapshot();
        break;
    }
}

void ZxEmulator::releaseTouchJoystick()
{
    m_fastForwardControl.clearTouch();
    syncFastForward();
    m_rzxAxisActive[ControllerInput::TouchX] = m_rzxAxisActive[ControllerInput::TouchY] = false;
    m_controller.clearTouch();
    applyControllerState();
}

void ZxEmulator::showMessage(QString message, ZxEmulator::ErrorLevel level)
{
    if (level != Info)
        qWarning().noquote() << message;
    // Any thread (e.g. the Engine's log callback)
    QMetaObject::invokeMethod(
            this, [this, message, level] { emit error(level, message); }, Qt::QueuedConnection);
}

void ZxEmulator::setSpeed(int percent)
{
    m_engine->setSpeed(uint32_t(std::max(percent, 1)));
}

void ZxEmulator::speedup()
{
    if (m_pauseCount || m_paused || !m_processInputEvents)
        return;
    m_fastForwardControl.press(ControllerInput::TouchButtonOffset + 256);
    syncFastForward();
}

void ZxEmulator::slowdown()
{
    m_fastForwardControl.release(ControllerInput::TouchButtonOffset + 256);
    syncFastForward();
}

void ZxEmulator::cancelFastForward()
{
    m_fastForwardControl.release(ControllerInput::TouchButtonOffset + 256, true);
    syncFastForward();
}

void ZxEmulator::syncFastForward()
{
    const bool active = m_fastForwardControl.active();
    if (m_manualFastForward != active) {
        m_manualFastForward = active;
        if (m_engine)
            m_engine->setTurbo(active);
    }
    updateFastForwardIndicator();
}

void ZxEmulator::updateFastForwardIndicator()
{
    const bool active = m_manualFastForward || m_tapeFastForward;
    if (active == m_fastForwardIndicator)
        return;
    m_fastForwardIndicator = active;
    emit fastForwardChanged();
}

void ZxEmulator::togglePaused()
{
    if (m_recording->waitingForInput()) {
        m_recording->resume();
        return;
    }
    m_paused = !m_paused;
    if (!m_touchscreen || !m_showControlsIcons)
        showMessage(m_paused ? tr("Pause spectacol") : tr("Resume spectacol"));
    setPaused(m_paused);
}

void ZxEmulator::sendNavigationKey(int key)
{
    if (m_processInputEvents)
        return;
    QPointer<QQuickWindow> window = qobject_cast<QQuickWindow *>(QGuiApplication::focusWindow());
    if (!window)
        return;
    const auto showKeyboard = [&window] {
        const auto *item = window ? window->activeFocusItem() : nullptr;
        if (item && item->flags().testFlag(QQuickItem::ItemAcceptsInputMethod)
            && !item->property("readOnly").toBool()) {
            QGuiApplication::inputMethod()->show();
            return true;
        }
        return false;
    };
    if (key == Qt::Key_Select) {
        if (showKeyboard())
            return;
        auto *item = window->activeFocusItem();
        if (item && item->isEnabled()) {
            if (auto *popup = item->property("popup").value<QObject *>();
                popup && !popup->property("visible").toBool()) {
                QMetaObject::invokeMethod(popup, "open");
                return;
            }
            if (item->metaObject()->indexOfMethod("click()") >= 0) {
                QMetaObject::invokeMethod(item, "click");
                return;
            }
        }
        key = Qt::Key_Return;
    }
    const auto modifiers = key == Qt::Key_Backtab ? Qt::ShiftModifier : Qt::NoModifier;
    QKeyEvent press(QEvent::KeyPress, key, modifiers);
    QCoreApplication::sendEvent(window, &press);
    if (window) {
        QKeyEvent release(QEvent::KeyRelease, key, modifiers);
        QCoreApplication::sendEvent(window, &release);
        if (key == Qt::Key_Return)
            showKeyboard();
    }
}

void ZxEmulator::debuggerTrap()
{
    activateDebugger();
}

void ZxEmulator::debuggerStep()
{
    if (!debuggerReady())
        return;
    try {
        {
            auto core = emulator().writeLock();
            core->stepInstruction();
            if (auto event = core->takePauseEvent())
                m_debugPause = *event;
        }
        m_engine->refreshFrame();
        updateDebugger();
    } catch (const std::exception &e) {
        showMessage(QString::fromUtf8(e.what()), Warning);
    }
}
void ZxEmulator::debuggerNext()
{
    if (!debuggerReady())
        return;
    try {
        m_engine->stepOver();
        if (!paused())
            m_engineQuiescent = false;
        else {
            auto core = emulator().writeLock();
            if (auto event = core->takePauseEvent())
                m_debugPause = *event;
        }
        emit pausedChanged();
        if (debuggerReady())
            updateDebugger();
        else {
            if (m_debugMemory)
                m_debugMemory->setEditable(false);
            emit debuggerStatusChanged();
        }
    } catch (const std::exception &e) {
        showMessage(QString::fromUtf8(e.what()), Warning);
    }
}
void ZxEmulator::debuggerOut()
{
    if (!debuggerReady())
        return;
    m_engine->stepOut();
    m_engineQuiescent = false;
    if (m_debugMemory)
        m_debugMemory->setEditable(false);
    emit pausedChanged();
    emit debuggerStatusChanged();
}
void ZxEmulator::debuggerRunTo(int address)
{
    if (!debuggerReady() || address < 0 || address > 65535)
        return;
    m_engine->runTo(uint16_t(address));
    if (!paused())
        m_engineQuiescent = false;
    if (m_debugMemory)
        m_debugMemory->setEditable(debuggerReady());
    emit pausedChanged();
    emit debuggerStatusChanged();
}
void ZxEmulator::debuggerContinue()
{
    emulator().writeLock()->cancelRunControl();
    if (m_debugMemory)
        m_debugMemory->setEditable(false);
    if (m_paused) {
        m_paused = false;
        setPaused(false);
    }
    if (m_debuggerOwnsPause) {
        m_debuggerOwnsPause = false;
        setPaused(false);
    } else if (!m_pauseCount) {
        m_engineQuiescent = false;
        m_engine->setPaused(false);
        emit pausedChanged();
    }
    emit debuggerStatusChanged();
}
void ZxEmulator::debuggerRun()
{
    deactivateDebugger(true);
}

bool ZxEmulator::createBreakpoint(int type, const QString &text, const QString &maskText,
                                  bool temporary)
{
    quint32 address = 0, mask = 0;
    bool valid = type >= 0 && type <= int(zxkit::BreakpointType::Event);
    if (type == int(zxkit::BreakpointType::RasterLine)
        || type == int(zxkit::BreakpointType::Event)) {
        bool ok = false;
        address = text.toUInt(&ok, 10);
        valid &= ok;
        const auto core = emulator().readLock();
        valid &= type == int(zxkit::BreakpointType::Event)
                ? address <= 1
                : address < core->clock().timings().linesPerFrame;
    } else
        valid &= DebuggerMemoryModel::parseHex(text, 65535, address);
    valid &= DebuggerMemoryModel::parseHex(maskText, 65535, mask);
    if (type == int(zxkit::BreakpointType::NextRegWrite))
        valid &= address <= 255 && emulator().readLock()->machine().id() == "next"sv;
    if (!valid) {
        showMessage(tr("Invalid breakpoint: use hexadecimal addresses/masks, decimal raster lines, "
                       "or event 0 (IRQ) / 1 (NMI)."),
                    Warning);
        return false;
    }
    zxkit::Breakpoint bp;
    bp.type = zxkit::BreakpointType(type);
    bp.address = uint16_t(address);
    bp.mask = uint16_t(mask);
    bp.temporary = temporary;
    emulator().writeLock()->addBreakpoint(std::move(bp));
    m_breakpointsModel.breakpointsUpdated();
    return true;
}
void ZxEmulator::watchMemory(int type)
{
    if (!debuggerReady() || (type != 1 && type != 2))
        return;
    auto *memory = debugMemory();
    zxkit::Breakpoint bp;
    bp.type = zxkit::BreakpointType(type);
    bp.address = uint16_t(memory->address());
    if (memory->bankId() > 0) {
        bp.bankId = zxkit::BankId(memory->bankId());
        bp.page1k = uint16_t(memory->address() / 1024);
        bp.address &= 1023;
        bp.mask = 1023;
    }
    emulator().writeLock()->addBreakpoint(std::move(bp));
    m_breakpointsModel.breakpointsUpdated();
}
void ZxEmulator::enableBreakpoint(int id, bool enabled)
{
    emulator().writeLock()->setBreakpointEnabled(uint32_t(id), enabled);
    m_breakpointsModel.breakpointsUpdated();
}

void ZxEmulator::toggleBreakpoint(int address)
{
    if (!debuggerReady() || address < 0 || address > 0xFFFF)
        return;
    {
        auto core = emulator().writeLock();
        std::vector<uint32_t> matches;
        for (const auto &bp : core->debugger().breakpoints().breakpoints()) {
            if (bp.type == zxkit::BreakpointType::Execute && bp.address == address
                && bp.mask == 0xFFFF && !bp.bankId && !bp.page1k && !bp.condition)
                matches.push_back(bp.id);
        }
        if (matches.empty()) {
            zxkit::Breakpoint bp;
            bp.address = uint16_t(address);
            core->addBreakpoint(std::move(bp));
        } else {
            // Remove any duplicates left by the previous add-only shortcut too.
            for (const auto id : matches)
                core->removeBreakpoint(id);
        }
    }
    m_breakpointsModel.breakpointsUpdated();
    m_pokeFinderModel.update();
}

void ZxEmulator::addBreakpointPage(int address, int page, int type)
{
    zxkit::Breakpoint bp;
    bp.type = zxkit::BreakpointType(type);
    bp.address = uint16_t(address);
    bp.page1k = uint16_t(page * 16 + ((address & 0x3FFF) >> 10));
    bp.bankId = emulator().readLock()->machine().ramBankId();
    bp.address &= 1023;
    bp.mask = 1023;
    emulator().writeLock()->addBreakpoint(std::move(bp));
    m_breakpointsModel.breakpointsUpdated();
    m_pokeFinderModel.update();
}

int ZxEmulator::resolveDebuggerAddress(const QString &text)
{
    const auto label = text.trimmed();
    int address;
    {
        const auto core = emulator().readLock();
        address = m_symbols.resolve(*core, label);
    }
    if (address >= 0)
        return address;
    if (address == -2) {
        showMessage(tr("Ambiguous or unmapped symbol: %1").arg(label), Error);
        return -1;
    }
    quint32 value = 0;
    if (DebuggerMemoryModel::parseHex(label, 65535, value))
        return int(value);
    showMessage(tr("Unknown, ambiguous or unmapped symbol/address: %1").arg(label), Error);
    return -1;
}
int ZxEmulator::symbolAddress(int row)
{
    const auto address = m_symbols.rowAddress(*emulator().readLock(), row);
    if (address < 0)
        showMessage(tr("This label's RAM page is not currently mapped; select it in the emulated "
                       "program first"),
                    Error);
    return address;
}

void ZxEmulator::debuggerCommand(const QString &command)
{
    const QString trimmed = command.trimmed();
    if (trimmed.startsWith(u"del"_s, Qt::CaseInsensitive)
        || trimmed.startsWith(u"clear"_s, Qt::CaseInsensitive)) {
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
    m_joystickPromptTimer.stop();
    if (m_engine)
        m_engine->stop();
    if (m_audioOutput)
        m_audioOutput->stop();
}

const DisassambleModel *ZxEmulator::disassambleModel() const
{
    return &m_disassambleModel;
}

const PokeFinderModel *ZxEmulator::pokeFinderModel() const
{
    return &m_pokeFinderModel;
}

ZXGamesModel *ZxEmulator::onlineGamesModel()
{
    return &m_onlineGamesModel;
}

QVariantMap ZxEmulator::controllerMapping()
{
    auto result = m_settings->controllerMapping();
    result.insert(u"joystick"_s, m_selectedJoystickIndex);
    return result;
}

bool ZxEmulator::applyControllerMapping(const QVariantMap &mapping, bool temporary)
{
    if (!ControllerProfiles::validMapping(mapping))
        return false;
    releaseInputs();
    m_settings->applyControllerMapping(mapping, temporary);
    const int joystick = mapping.value(u"joystick"_s).toInt();
    const bool available = joystickAvailable(joystick);
    setSelectedJoysticksIndex(available ? joystick : NoJoystick);
    if (!available)
        showMessage(tr("This profile's Timex joystick needs a TC2068 or TS2068"), Info);
    return available;
}

bool ZxEmulator::saveControllerProfile(const QString &name)
{
    return m_controllerProfiles->save(name.trimmed(), controllerMapping());
}

bool ZxEmulator::loadControllerProfile(const QString &name)
{
    const auto mapping = m_controllerProfiles->mapping(name);
    if (mapping.isEmpty()) {
        showMessage(tr("Controller profile does not exist"), Error);
        return false;
    }
    return applyControllerMapping(mapping, m_settings->controllerOverride());
}

bool ZxEmulator::activateControllerGame(const QString &name)
{
    if (m_settings->controllerOverride())
        applyControllerMapping(m_defaultControllerMapping, false);
    else
        m_defaultControllerMapping = controllerMapping();
    m_controllerProfiles->setGame(name);
    const auto profile = m_controllerProfiles->gameProfile();
    return !profile.isEmpty()
            && applyControllerMapping(m_controllerProfiles->mapping(profile), true);
}

void ZxEmulator::setLoadedGame(const QString &path)
{
    m_loadedFileName = QFileInfo(path).completeBaseName();
    activateControllerGame(m_loadedFileName);
    emit saveSnapshotEnabledChanged();
}
