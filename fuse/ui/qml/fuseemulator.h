#ifndef FUSEEMULATOR_H
#define FUSEEMULATOR_H

#include "breakpointsmodel.h"
#include "disassamblemodel.h"

#include <QObject>
#include <QThread>
#include <QUrl>

#include <atomic>

class QQmlContext;

class FuseThread : public QThread
{
protected:
    void run();
};

class FuseEmulator : public QObject
{
    Q_OBJECT
    Q_ENUMS(ErrorLevel)

    Q_PROPERTY(bool paused READ paused WRITE setPaused NOTIFY pausedChanged)
    Q_PROPERTY(QUrl dataPath READ dataPath WRITE setDataPath NOTIFY dataPathChanged)
    Q_PROPERTY(bool saveSnapshotEnabled READ saveSnapshotEnabled NOTIFY saveSnapshotEnabledChanged)
    Q_PROPERTY(QStringList filtersModel READ filtersModel)
    Q_PROPERTY(int selectedFilterIndex READ selectedFilterIndex WRITE setSelectedFilterIndex NOTIFY selectedFilterIndexChanged)

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

    QStringList filtersModel() const;

    int selectedFilterIndex() const;
    void setSelectedFilterIndex(int selectedFilterIndex);

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

public slots:
    QUrl snapshotsPath() const;
    void load(const QUrl &filePath);
    void save(const QUrl &filePath);
    void reset();
    void hardReset();
    void quickSaveSnapshot();
    void quickLoadSnapshot();
    QString snapshotFileName(bool addExtension = true) const;

    // debug methods
    void debuggerTrap();
    void debuggerStep();
    void debuggerRun();

    void disassamble();
    void disassambleFetchUp(int lines);
    void disassamble(int address, int delta = -10, uint16_t length = 0xff);
    // debug methods

    void activateDebugger();
    void deactivateDebugger(bool interruptable);
signals:
    void pausedChanged();
    void dataPathChanged();
    void saveSnapshotEnabledChanged();
    void selectedFilterIndexChanged();
    void registersChanged();
    void showDebugger();
    void hideDebugger();

    void error(ErrorLevel level, const QString &message);

private:
    void updateScalers() const;
    void startFuseThread();
    friend class FuseTexture;

private:
    QString m_loadedFileName;
    mutable std::vector<int> m_supportedScalers;
    BreakpointsModel m_breakpointsModel;
    DisassambleModel m_disassambleModel;
    FuseThread m_fuseThread;
    std::atomic_bool m_debuggerActivated;
};

extern FuseEmulator *g_fuseEmulator;

#endif // FUSEEMULATOR_H
