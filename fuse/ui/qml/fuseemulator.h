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

#ifndef FUSEEMULATOR_H
#define FUSEEMULATOR_H

#include "breakpointsmodel.h"
#include "disassamblemodel.h"
#include "fuseobject.h"
#include "fusetape.h"
#include "pokefindermodel.h"
#include "zxgamesmodel.h"

#include <ui/ui.h>

#include <QAudioFormat>
#include <QGamepadManager>
#include <QPointer>
#include <QSemaphore>
#include <QThread>
#include <QUrl>

#include <atomic>

#include <libspectrum.h>

class QAudioOutput;
class QIODevice;
class QKeyEvent;
class QMouseEvent;
class QQmlContext;
class FuseSettings;
class FuseRecording;

class FuseThread : public QThread
{
public:
    FuseThread();
    int soundLowlevelInit(const char */*device*/, int *freqptr, int *stereoptr);
    void soundLowlevelFrame(libspectrum_signed_word *data, int len);
    void soundLowlevelEnd(void) {}

protected:
    void run();

private:
    QScopedPointer<QAudioOutput> m_audioOutput;
    QAudioFormat m_audioFormat;
    QPointer<QIODevice> m_audioOutputDevice;
    std::chrono::time_point<std::chrono::steady_clock> m_startFrameTime;
    int64_t m_uSleepTotal = 0;
};

class FuseEmulator : public FuseObject
{
    Q_OBJECT
    Q_PROPERTY(bool touchscreen MEMBER m_touchscreen CONSTANT)
    Q_PROPERTY(bool paused READ paused WRITE setPaused NOTIFY pausedChanged)
    Q_PROPERTY(bool processInputEvents READ processInputEvents WRITE setProcessInputEvents NOTIFY processInputEventsChanged)
    Q_PROPERTY(int gamepadId READ gamepadId WRITE setGamepadId NOTIFY gamepadIdChanged)
    Q_PROPERTY(QString dataPath READ dataPath WRITE setDataPath NOTIFY dataPathChanged)
    Q_PROPERTY(bool saveSnapshotEnabled READ saveSnapshotEnabled NOTIFY saveSnapshotEnabledChanged)
    Q_PROPERTY(QStringList filtersModel READ filtersModel CONSTANT)
    Q_PROPERTY(int selectedFilterIndex READ selectedFilterIndex WRITE setSelectedFilterIndex NOTIFY selectedFilterIndexChanged)
    Q_PROPERTY(QStringList joysticksModel READ joysticksModel CONSTANT)
    Q_PROPERTY(int selectedJoysticksIndex READ selectedJoysticksIndex WRITE setSelectedJoysticksIndex NOTIFY selectedJoysticksIndexChanged)
    Q_PROPERTY(int pokeFinderCount READ pokeFinderCount NOTIFY pokeFinderCountChanged)
    Q_PROPERTY(FuseTape *tape MEMBER m_tape CONSTANT)
    Q_PROPERTY(FuseRecording *recording MEMBER m_recording CONSTANT)
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
    enum UiQuery {
        UiNo = 0,
        UiYes = 1
    };

    enum ErrorLevel {
        Info,
        Warning,
        Error
    };
    enum ControlType {
        CursorJoystick,
        Keyboard48K
    };

    enum UiItemType {
        Disk = UI_STATUSBAR_ITEM_DISK,
        Microdrive = UI_STATUSBAR_ITEM_MICRODRIVE,
        Mouse = UI_STATUSBAR_ITEM_MOUSE,
        Paused = UI_STATUSBAR_ITEM_PAUSED,
        Tape = UI_STATUSBAR_ITEM_TAPE,
    };

    enum UiState {
        Active = UI_STATUSBAR_STATE_ACTIVE,
        Inactive = UI_STATUSBAR_STATE_INACTIVE,
        Gone = UI_STATUSBAR_STATE_NOT_AVAILABLE,
    };

    Q_ENUMS(ErrorLevel ControlType UiItemType UiState UiQuery)
public:
    explicit FuseEmulator(QQmlContext *ctxt, QObject *parent = 0);
    ~FuseEmulator();

    bool paused() const;
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

    int pokeFinderCount() const;

    bool processInputEvents() const { return m_processInputEvents.load(); }
    void setProcessInputEvents(bool processEvents);

    int gamepadId() const { return m_gamepadId; }
    void setGamepadId(int gamepadId);

    QString saveFilePath(const QString& fileName);

    int soundLowlevelInit(const char */*device*/, int *freqptr, int *stereoptr);
    void soundLowlevelFrame(libspectrum_signed_word *data, int len);
    void soundLowlevelEnd(void) {}

    void mousePress(QMouseEvent *event);
    void mouseMove(QMouseEvent *event);
    void mouseRelease(QMouseEvent *event);

    FuseSettings *settings() const;
    void resetLoadedFile();

    void uiStatusbarUpdate(ui_statusbar_item item, ui_statusbar_state state);

    char *uiOpenFilename(const QByteArray &title, const QString &path = QString());
    char *uiSaveFilename(const QByteArray &title);
    int uiQuery(const QByteArray &message);
    void uiPokememSelector(const char *filePath);
    int uiGetListIndex(const QStringList &list, const QString &title);

public slots:
    void quit();
    QString snapshotsPath() const;
    QString recordingsPath() const;
    QString recordingFilePath() const;
    void load(const QString &filePath, bool removeOnFail = false);
    void save(const QString &filePath);
    void copyToFavourites(const QString &filePath);
    void remove(const QString &file);

    void reset();
    void hardReset();
    void nmi();
    void quickSaveSnapshot();
    void quickLoadSnapshot();

    // debug methods
    void debuggerTrap();
    void debuggerNext();
    void debuggerRun();

    void addBreakpoint(int address, int type = BreakpointsModel::BreakOnExecute);
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

    void keyPress(int qtKey, int modifiers = 0, bool autoRepeat = false);
    void keyRelease(int qtKey, int modifiers = 0, bool autoRepeat = false);
    void gamepadAxisEvent(QGamepadManager::GamepadAxis axis, double value);
    void gamepadButtonPressEvent(QGamepadManager::GamepadButton button);
    void gamepadButtonReleaseEvent(QGamepadManager::GamepadButton button);

    void setOpenSaveFile(const QByteArray &filePath);
    void setQuery(UiQuery result);

    void showMessage(QString message, ErrorLevel level = Info);
    void setListIndex(int index);

    void speedup();
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
    void showDebugger();
    void hideDebugger();
    void showMenu();
    void hideMenu();
    void processInputEventsChanged();
    void gamepadIdChanged();
    void toggleOnScreenControls(ControlType type);
    void error(ErrorLevel level, const QString &message);
    void uiIconUpdate(UiItemType item, UiState state);
    void configureJoystick();
    void showControlsIconsChanged(bool showControlsIcons);
    void openFile(const QString &title, const QString &path);
    void saveFile(const QString &title);
    void query(const QString &message);
    void showPokememSelector();
    void getListIndex(const QStringList &list, const QString &title);

private:
    void updateScalers() const;
    void startFuseThread();
    friend class FuseTexture;

private:
    bool m_touchscreen = false;
    QString m_loadedFileName;
    mutable std::vector<int> m_supportedScalers;
    BreakpointsModel m_breakpointsModel;
    DisassambleModel m_disassambleModel;
    PokeFinderModel m_pokeFinderModel;
    FuseThread m_fuseThread;
    std::atomic_bool m_debuggerActivated;
    bool m_resetPokeFinder;
    std::atomic_bool m_processInputEvents;
    int m_gamepadId = -1;
    ZXGamesModel m_onlineGamesModel;
    std::unique_ptr<FuseSettings> m_fuseSettings;
    Qt::ApplicationState m_applicationState = Qt::ApplicationActive;
    friend int ui_tape_browser_update(ui_tape_browser_update_type /*change*/, libspectrum_tape_block */*block*/);
    FuseTape *m_tape = nullptr;
    friend int ui_menu_activate(ui_menu_item item, int active);
    FuseRecording* m_recording = nullptr;
    bool m_showControlsIcons = true;
    QSemaphore m_waitSemaphore;
    QByteArray m_openSaveFilePath;
    UiQuery m_queryResult = UiNo;
    int m_listIndex = -1;
    bool m_paused = false;
};

extern FuseEmulator *g_fuseEmulator;

#endif // FUSEEMULATOR_H
