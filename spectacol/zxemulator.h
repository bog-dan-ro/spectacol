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

#pragma once
#include "nextuarthooks.h"

#include "breakpointsmodel.h"
#include "debuggermodels.h"
#include "debugsymbols.h"
#include "romoverrides.h"
#include "pokemodel.h"
#include "disassamblemodel.h"
#include "disks.h"
#include "sdcardfiles.h"
#include "zxtape.h"
#include "pokefindermodel.h"
#include "zxgamesmodel.h"
#include "controllerinput.h"
#include "controllerprofiles.h"

Q_MOC_INCLUDE("zxrecording.h")
Q_MOC_INCLUDE("zxsettings.h")

#include <QGamepadManager>
#include <QPointer>
#include <QQmlEngine>
#include <QUrl>
#include <QTimer>

#include <zxkit/core/Engine.h>
#include <zxkit/core/Events.h>
#include <zxkit/media/MediaPreview.h>

#include <atomic>
#include <exception>
#include <memory>
#include <utility>

class AudioOutput;
class QIODevice;
class QKeyEvent;
class QMouseEvent;
class QQmlContext;
class ZxSettings;
class ZxRecording;

class ZxEmulator : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(NextUartHooks *uartHooks READ uartHooks CONSTANT)
    Q_PROPERTY(ControllerProfiles *controllerProfiles READ controllerProfiles CONSTANT FINAL)
    Q_PROPERTY(bool fastForward READ fastForward NOTIFY fastForwardChanged FINAL)
    Q_PROPERTY(bool touchscreen MEMBER m_touchscreen CONSTANT)
    Q_PROPERTY(bool paused READ paused WRITE setPaused NOTIFY pausedChanged)
    Q_PROPERTY(bool processInputEvents READ processInputEvents WRITE setProcessInputEvents NOTIFY processInputEventsChanged)
    Q_PROPERTY(bool mouseGrabbed READ mouseGrabbed NOTIFY mouseGrabbedChanged)
    Q_PROPERTY(int gamepadId READ gamepadId WRITE setGamepadId NOTIFY gamepadIdChanged)
    Q_PROPERTY(QString dataPath READ dataPath WRITE setDataPath NOTIFY dataPathChanged)
    Q_PROPERTY(bool saveSnapshotEnabled READ saveSnapshotEnabled NOTIFY saveSnapshotEnabledChanged)
    Q_PROPERTY(QStringList filtersModel READ filtersModel CONSTANT)
    Q_PROPERTY(int selectedFilterIndex READ selectedFilterIndex WRITE setSelectedFilterIndex NOTIFY selectedFilterIndexChanged)
    Q_PROPERTY(QStringList joysticksModel READ joysticksModel CONSTANT)
    Q_PROPERTY(int selectedJoysticksIndex READ selectedJoysticksIndex WRITE setSelectedJoysticksIndex NOTIFY selectedJoysticksIndexChanged)
    Q_PROPERTY(int pokeFinderCount READ pokeFinderCount NOTIFY pokeFinderCountChanged)
    Q_PROPERTY(const ZxTape *tape MEMBER m_tape CONSTANT)
    Q_PROPERTY(Disks *disks MEMBER m_disks CONSTANT)
    Q_PROPERTY(SdCardFiles *sdCard MEMBER m_sdCard CONSTANT)
    Q_PROPERTY(const ZxRecording *recording MEMBER m_recording CONSTANT)
    Q_PROPERTY(RomOverrides *roms READ roms CONSTANT FINAL)
    Q_PROPERTY(PokeModel *pokes READ pokes CONSTANT FINAL)
    Q_PROPERTY(DebugSymbols *symbols READ symbols CONSTANT FINAL)
    Q_PROPERTY(ZxSettings *settings READ settings CONSTANT)
    Q_PROPERTY(DebuggerMemoryModel *debugMemory READ debugMemory CONSTANT FINAL)
    Q_PROPERTY(DebuggerStateModel *debugNextRegisters READ debugNextRegisters CONSTANT FINAL)
    Q_PROPERTY(DebuggerStateModel *debugMmu READ debugMmu CONSTANT FINAL)
    Q_PROPERTY(bool debuggerReady READ debuggerReady NOTIFY pausedChanged FINAL)
    Q_PROPERTY(QString debuggerStatus READ debuggerStatus NOTIFY debuggerStatusChanged FINAL)
    Q_PROPERTY(const BreakpointsModel *breakpointsModel READ breakpointsModel CONSTANT)
    Q_PROPERTY(const DisassambleModel* disassambleModel READ disassambleModel CONSTANT)
    Q_PROPERTY(const PokeFinderModel* pokeFinderModel READ pokeFinderModel CONSTANT)
    Q_PROPERTY(ZXGamesModel* onlineGamesModel READ onlineGamesModel CONSTANT)

    Q_PROPERTY(bool showControlsIcons MEMBER m_showControlsIcons NOTIFY showControlsIconsChanged)

    /* regs properties */
    Q_PROPERTY(QString PC READ PC WRITE setPC NOTIFY registersChanged)
    Q_PROPERTY(QString SP READ SP WRITE setSP NOTIFY registersChanged)
    Q_PROPERTY(QString IX READ IX WRITE setIX NOTIFY registersChanged)
    Q_PROPERTY(QString IY READ IY WRITE setIY NOTIFY registersChanged)
    Q_PROPERTY(QString R READ R WRITE setR NOTIFY registersChanged)
    Q_PROPERTY(QString I READ I WRITE setI NOTIFY registersChanged)
    Q_PROPERTY(QString R7 READ R7 WRITE setR7 NOTIFY registersChanged)
    Q_PROPERTY(QString IFF1 READ IFF1 WRITE setIFF1 NOTIFY registersChanged)
    Q_PROPERTY(QString IFF2 READ IFF2 WRITE setIFF2 NOTIFY registersChanged)
    Q_PROPERTY(QString IM READ IM WRITE setIM NOTIFY registersChanged)

    Q_PROPERTY(QString AF READ AF WRITE setAF NOTIFY registersChanged)
    Q_PROPERTY(QString BC READ BC WRITE setBC NOTIFY registersChanged)
    Q_PROPERTY(QString DE READ DE WRITE setDE NOTIFY registersChanged)
    Q_PROPERTY(QString HL READ HL WRITE setHL NOTIFY registersChanged)
    Q_PROPERTY(QString AF_ READ AF_ WRITE setAF_ NOTIFY registersChanged)
    Q_PROPERTY(QString BC_ READ BC_ WRITE setBC_ NOTIFY registersChanged)
    Q_PROPERTY(QString DE_ READ DE_ WRITE setDE_ NOTIFY registersChanged)
    Q_PROPERTY(QString HL_ READ HL_ WRITE setHL_ NOTIFY registersChanged)

public:
    enum ErrorLevel {
        Info,
        Warning,
        Error
    };
    Q_ENUM(ErrorLevel)

    enum ControlType {
        CursorJoystick,
        Keyboard48K
    };
    Q_ENUM(ControlType)

    enum UiItemType {
        Disk = 0,
        Microdrive,
        Mouse,
        Paused,
        Tape,
    };
    Q_ENUM(UiItemType)

    enum UiState {
        Active = 0,
        Inactive,
        Gone,
    };
    Q_ENUM(UiState)

public:
    static ZxEmulator *create(QQmlEngine *, QJSEngine *);
    static ZxEmulator &instance();
    ~ZxEmulator();

    ControllerProfiles *controllerProfiles() const { return m_controllerProfiles.get(); }
    bool fastForward() const { return m_fastForwardIndicator; }
    NextUartHooks *uartHooks() const { return m_uartHooks.get(); }
    zxkit::Engine &engine() { return *m_engine; }
    const zxkit::Engine &engine() const { return *m_engine; }
    /// The emulated machine, shared with the Engine thread: lock it to use it.
    /// The Engine holds the lock for a scanline at a time, so a lock never waits long.
    const zxkit::Shared<zxkit::Emulator> &emulator() const { return m_engine->emulator(); }

    /// Runs `fn(emulator)` with the emulator locked, shows the error if it throws.
    template <typename F>
    bool withEmulator(F &&fn)
    {
        try {
            auto emulator = m_engine->emulator().writeLock();
            std::forward<F>(fn)(*emulator);
            return true;
        } catch (const std::exception &e) {
            showMessage(QString::fromUtf8(e.what()), Error);
            return false;
        }
    }
    /// Loads any media file (the file is read before locking the emulator).
    bool loadMediaFile(const QString &filePath);
    /// Copy a document-provider file into app storage while its URI grant is valid.
    Q_INVOKABLE QString importFile(const QUrl &url);
    /// Saves a snapshot, the tape or a screenshot (the file is written after unlocking the emulator).
    bool saveMediaFile(const QString &filePath);
    /// Replaces local files atomically; selected Android documents use direct writes.
    /// Reports directory, write and commit failures.
    bool writeFileBytes(const QString &filePath, std::span<const uint8_t> bytes);
    void releaseInputs();
    /// A consumer drive-A insertion selects a game without reading the media twice.
    void setLoadedGame(const QString &path);
    zxkit::Z80Registers registers() const;

    bool paused() const;
    bool emulationQuiescent() const { return m_engineQuiescent; }
    bool canResumeRzxInput() const { return m_pauseCount == 1 && !m_paused && m_processInputEvents.load(); }
    void setPaused(bool paused);

    QString dataPath() const;
    void setDataPath(const QString &dataPath);

    bool saveSnapshotEnabled() const;

    QStringList filtersModel() const;
    int selectedFilterIndex() const;
    void setSelectedFilterIndex(int selectedFilterIndex);

    QStringList joysticksModel() const;
    int selectedJoysticksIndex() const;
    void setSelectedJoysticksIndex(int selectedJoysticksIndex);
    Q_INVOKABLE bool joystickAvailable(int index) const;

    QString PC() const;
    void setPC(const QString &value);
    QString SP() const;
    void setSP(const QString &value);
    QString IX() const;
    void setIX(const QString &value);
    QString IY() const;
    void setIY(const QString &value);
    QString R() const;
    void setR(const QString &value);
    QString I() const;
    void setI(const QString &value);
    QString R7() const;
    void setR7(const QString &value);
    QString IFF1() const;
    void setIFF1(const QString &value);
    QString IFF2() const;
    void setIFF2(const QString &value);
    QString IM() const;
    void setIM(const QString &value);

    QString AF() const;
    void setAF(const QString &value);
    QString BC() const;
    void setBC(const QString &value);
    QString DE() const;
    void setDE(const QString &value);
    QString HL() const;
    void setHL(const QString &value);
    QString AF_() const;
    void setAF_(const QString &value);
    QString BC_() const;
    void setBC_(const QString &value);
    QString DE_() const;
    void setDE_(const QString &value);
    QString HL_() const;
    void setHL_(const QString &value);

    BreakpointsModel *breakpointsModel() { return &m_breakpointsModel; }
    void updateDebugger();
    DebuggerMemoryModel *debugMemory();
    DebuggerStateModel *debugNextRegisters();
    DebuggerStateModel *debugMmu();
    bool debuggerReady() const { return paused() && m_engineQuiescent; }
    QString debuggerStatus() const;
    RomOverrides *roms() const { return m_roms.get(); }
    PokeModel *pokes() const { return m_pokes.get(); }
    DebugSymbols *symbols() { return &m_symbols; }
    Q_INVOKABLE int resolveDebuggerAddress(const QString &text);
    Q_INVOKABLE int symbolAddress(int row);


    int pokeFinderCount() const;

    bool processInputEvents() const { return m_processInputEvents.load(); }
    // Like paused, it nests: every false must be balanced by a true.
    void setProcessInputEvents(bool processEvents);

    int gamepadId() const { return m_gamepadId; }
    void setGamepadId(int gamepadId);

    QString saveFilePath(const QString& fileName);

    void writeAudioSamples(std::span<const int16_t> samples, uint32_t sampleRate, uint8_t channels);
    /// Emulation speed, in percent of the machine's real speed.
    void setSpeed(int percent);

    // Kempston mouse
    void mouseButton(Qt::MouseButton button, bool pressed);
    void mouseMotion(int dx, int dy);
    void setMouseGrabSuspended(bool suspended);
    bool mouseGrabbed() const { return m_mouseGrabbed; }
    void setMouseGrabbed(bool grabbed);

    ZxSettings *settings() const;
    void resetLoadedFile();

    const DisassambleModel *disassambleModel() const;

    const PokeFinderModel *pokeFinderModel() const;
    ZXGamesModel* onlineGamesModel();

public slots:
    void quit();
    QString snapshotsPath() const;
    QString recordingsPath() const;
    QString savesPath() const;
    QString recordingFilePath() const;
    QString tapeFileName(const QString &programName) const;
    QString tapeFilePath(const QString &fileName) const;
    QStringList snapshotFormats() const;
    QString snapshotFileName() const;
    QString snapshotFilePath(const QString &folder, const QString &fileName) const;
    void load(const QString &filePath, bool removeOnFail = false);
    bool save(const QString &filePath);
    bool copyToFavourites(const QString &filePath);
    void remove(const QString &path);
    bool isFolder(const QString &path);
    QString fileName(const QString &path);
    QString folderName(const QString &path);

    bool copy(const QString &from, const QString &to, bool override = false);

    void reset();
    void hardReset();
    void nmi();
    bool canNextNmi(bool multiface) const;
    void nextNmi(bool multiface);
    Q_INVOKABLE void setNextKeyState(int key, bool pressed);
    void quickSaveSnapshot();
    void quickLoadSnapshot();

    // debug methods
    void debuggerTrap();
    void sendNavigationKey(int key);
    void debuggerStep();
    void debuggerNext();
    void debuggerRun();
    void debuggerContinue();
    void debuggerOut();
    void debuggerRunTo(int address);
    void debuggerRefresh();
    bool createBreakpoint(int type, const QString &address, const QString &mask, bool temporary);
    void watchMemory(int type);
    void enableBreakpoint(int id, bool enabled);


    void toggleBreakpoint(int address);
    void addBreakpointPage(int address, int page, int type = BreakpointsModel::BreakOnWrite);
    void debuggerCommand(const QString &command);


    void disassamble();
    void disassambleFetchUp(int lines);
    void disassamble(int address, int delta = -10, uint16_t length = 0xff);
    void activateDebugger();
    void deactivateDebugger(bool interruptable);
    QString dumpData(const QString &base64CompressedData, int offset, int charsPerLine);
    // debug methods

    // pokefinder methods
    void pokeFinderInced();
    void pokeFinderDeced();
    void pokeFinderSearch(int value);
    void pokeFinderReset();
    void pokeFinderResetIfNeeded();
    // pokefinder methods

    void pokeMemory(int address, int page, int value);

    void keyPress(int qtKey, int modifiers = 0, bool autoRepeat = false, bool virtualKeyboard = true);
    void keyRelease(int qtKey, int modifiers = 0, bool autoRepeat = false, bool virtualKeyboard = true);
    // The on-screen CoBra column, independent of the Spectrum modifier mapping.
    void setCobraKeyState(int qtKey, bool pressed);
    void gamepadAxisEvent(QGamepadManager::GamepadAxis axis, double value, bool touch = false);
    void gamepadButtonPressEvent(QGamepadManager::GamepadButton button, bool touch = false);
    void gamepadButtonReleaseEvent(QGamepadManager::GamepadButton button, bool touch = false, bool cancelled = false);
    void releaseTouchJoystick();


    void showMessage(QString message, ZxEmulator::ErrorLevel level = Info);

    // Fast forward, while the button is held
    void speedup();
    void cancelFastForward();
    bool saveControllerProfile(const QString &name);
    bool loadControllerProfile(const QString &name);
    void slowdown();

    void togglePaused();

signals:
    void pausedChanged();
    void dataPathChanged();
    void saveSnapshotEnabledChanged();
    void selectedFilterIndexChanged();
    void selectedJoysticksIndexChanged();
    void pokeFinderCountChanged();
    void registersChanged();
    void showWelcome();
    void debuggerStatusChanged();
    void showDebugger();
    void hideDebugger();
    void showMenu();
    void processInputEventsChanged();
    void mouseGrabbedChanged(bool mouseGrabbed);
    void gamepadIdChanged();
    void toggleOnScreenControls(ZxEmulator::ControlType type);
    void error(ZxEmulator::ErrorLevel level, const QString &message);
    void uiIconUpdate(ZxEmulator::UiItemType item, ZxEmulator::UiState state);
    /// The tape moved to another block (it plays, or a block was loaded)
    void tapeBlockChanged(int block);
    void configureJoystick();
    void showControlsIconsChanged(bool showControlsIcons);
    void fastForwardChanged();

private:
    explicit ZxEmulator(QObject *parent = 0);
    void stopEngine();
    void updateMediaActivity(const zxkit::MediaActivity &activity);
    void configureController();
    void applyControllerState();
    void syncFastForward();
    void updateFastForwardIndicator();
    QVariantMap controllerMapping();
    bool applyControllerMapping(const QVariantMap &mapping, bool temporary);
    bool activateControllerGame(const QString &title);
    void setInputKeys(quint64 source, const QSet<int> &keys);
    void applyKeyChanges(const HeldKeyState::Changes &changes);
    std::pair<bool, zxkit::MediaClass> loadMediaWithType(const QString &filePath);
    friend class ZxImage;

private:
    bool m_touchscreen = false;
    QString m_loadedFileName;
    int m_selectedFilterIndex = 0;
    // The joysticks the gamepad can be: the order of joysticksModel()
    enum JoystickKind : int { NoJoystick = 0, CursorKeys, KempstonJoystick, Sinclair1, Sinclair2, FullerJoystick, Timex1, Timex2 };
    void setJoystickButtons(uint8_t buttons);
    void attachJoystick(int kind);
    int m_selectedJoystickIndex = CursorKeys;
    QTimer m_joystickPromptTimer;
    uint8_t m_joystickButtons = 0; // zxkit::JoystickButtons
    ControllerInput m_controller;
    HeldKeyState m_heldKeys;
    FastForwardControl m_fastForwardControl;
    bool m_manualFastForward = false;
    bool m_tapeFastForward = false;
    bool m_fastForwardIndicator = false;
    QVariantMap m_defaultControllerMapping;
    std::unique_ptr<DebuggerMemoryModel> m_debugMemory;
    std::unique_ptr<DebuggerStateModel> m_debugNextRegisters, m_debugMmu;
    zxkit::PauseEvent m_debugPause;
    bool m_debuggerOwnsPause = false;
    BreakpointsModel m_breakpointsModel;
    DisassambleModel m_disassambleModel;
    PokeFinderModel m_pokeFinderModel;
    // Owned here, the Engine only gets Weak references to them
    zxkit::CallbackHandle<zxkit::VideoFrameCallback> m_videoFrameCallback;
    zxkit::CallbackHandle<zxkit::AudioCallback> m_audioCallback;
    zxkit::CallbackHandle<zxkit::PauseCallback> m_pauseCallback;
    zxkit::CallbackHandle<zxkit::ResumeCallback> m_resumeCallback;
    zxkit::CallbackHandle<zxkit::LogCallback> m_logCallback;
    zxkit::CallbackHandle<zxkit::MediaActivityCallback> m_mediaActivityCallback;
    zxkit::MediaActivity m_mediaActivity; // the last one the GUI saw
    std::unique_ptr<NextUartHooks> m_uartHooks;
    std::unique_ptr<RomOverrides> m_roms;
    std::unique_ptr<zxkit::Engine> m_engine;
    std::unique_ptr<PokeModel> m_pokes;
    DebugSymbols m_symbols;
    std::unique_ptr<AudioOutput> m_audioOutput;
    std::atomic_bool m_debuggerActivated{false};
    bool m_resetPokeFinder = true;
    std::atomic_bool m_processInputEvents{true};
    int m_inputEventsBlockers = 0;
    bool m_mouseGrabbed = false;
    int m_gamepadId = -1;
    ZXGamesModel m_onlineGamesModel;
    std::unique_ptr<ZxSettings> m_settings;
    std::unique_ptr<ControllerProfiles> m_controllerProfiles;
    Qt::ApplicationState m_applicationState = Qt::ApplicationActive;
    ZxTape *m_tape = nullptr;
    Disks *m_disks = nullptr;
    SdCardFiles *m_sdCard = nullptr;
    ZxRecording* m_recording = nullptr;
    bool m_showControlsIcons = true;
    bool m_paused = false; // togglePaused()
    bool m_engineQuiescent = true;
    QSet<int> m_pressedRzxButtons;
    std::array<bool, ControllerInput::AxisCount> m_rzxAxisActive{};
    int m_pauseCount = 0;  // setPaused() nests
};
