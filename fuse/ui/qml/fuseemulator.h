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
#include "pokefindermodel.h"
#include "zxgamesmodel.h"

#include <ui/ui.h>

#include <QAudioFormat>
#include <QGamepadManager>
#include <QPointer>
#include <QThread>
#include <QUrl>

#include <atomic>

#include <libspectrum.h>

class QAudioOutput;
class QIODevice;
class QQmlContext;
class FuseSettings;

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

    Q_PROPERTY(bool paused READ paused WRITE setPaused NOTIFY pausedChanged)
    Q_PROPERTY(bool processJoysticksEvents READ processJoysticksEvents WRITE setProcessJoysticksEvents NOTIFY processJoysticksEventsChanged)
    Q_PROPERTY(int gamepadId READ gamepadId WRITE setGamepadId NOTIFY gamepadIdChanged)
    Q_PROPERTY(QUrl dataPath READ dataPath WRITE setDataPath NOTIFY dataPathChanged)

    Q_PROPERTY(bool saveSnapshotEnabled READ saveSnapshotEnabled NOTIFY saveSnapshotEnabledChanged)
    Q_PROPERTY(QStringList joysticksModel READ joysticksModel CONSTANT)
    Q_PROPERTY(int selectedJoysticksIndex READ selectedJoysticksIndex WRITE setSelectedJoysticksIndex NOTIFY selectedJoysticksIndexChanged)
    Q_PROPERTY(int pokeFinderCount READ pokeFinderCount NOTIFY pokeFinderCountChanged)

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

public:
    explicit FuseEmulator(QQmlContext *ctxt, QObject *parent = 0);
    ~FuseEmulator();

    bool paused() const;
    void setPaused(bool paused);

    QUrl dataPath() const;
    void setDataPath(const QUrl &dataPath);

    bool saveSnapshotEnabled() const;

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

    bool processJoysticksEvents() const { return m_processJoysticksEvents.load(); }
    void setProcessJoysticksEvents(bool processJoysticksEvents);

    int gamepadId() const { return m_gamepadId; }
    void setGamepadId(int gamepadId);

    QString saveFilePath(const QString& fileName);

    int soundLowlevelInit(const char */*device*/, int *freqptr, int *stereoptr);
    void soundLowlevelFrame(libspectrum_signed_word *data, int len);
    void soundLowlevelEnd(void) {}

public slots:
    QUrl snapshotsPath() const;
    void load(const QUrl &filePath, bool removeOnFail = false);
    void save(const QUrl &filePath);
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
    // debug methods

    // pokefinder methods
    void pokeFinderInced();
    void pokeFinderDeced();
    void pokeFinderSearch(int value);
    void pokeFinderReset();
    void pokeFinderResetIfNeeded();
    // pokefinder methods

    void pokeMemory(int address, int page, int value);

signals:
    void pausedChanged();
    void dataPathChanged();
    void saveSnapshotEnabledChanged();
    void selectedJoysticksIndexChanged();
    void pokeFinderCountChanged();
    void registersChanged();
    void showDebugger();
    void hideDebugger();
    void showMenu();
    void hideMenu();
    void processJoysticksEventsChanged();
    void gamepadIdChanged();

    void error(ErrorLevel level, const QString &message);

private:
    void startFuseThread();
    friend class FuseTexture;

private:
    QString m_loadedFileName;
    BreakpointsModel m_breakpointsModel;
    DisassambleModel m_disassambleModel;
    PokeFinderModel m_pokeFinderModel;
    FuseThread m_fuseThread;
    std::atomic_bool m_debuggerActivated;
    bool m_resetPokeFinder;
    std::atomic_bool m_processJoysticksEvents;
    int m_gamepadId = -1;
    ZXGamesModel m_onlineGamesModel;
    std::unique_ptr<FuseSettings> m_fuseSettings;
    Qt::ApplicationState m_applicationState = Qt::ApplicationActive;
};

extern FuseEmulator *g_fuseEmulator;

#endif // FUSEEMULATOR_H
