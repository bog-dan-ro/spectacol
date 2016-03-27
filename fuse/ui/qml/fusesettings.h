#ifndef FUSESETTINGS_H
#define FUSESETTINGS_H

#include "fuseobject.h"

class FuseSettings : public FuseObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList machinesModel READ machinesModel CONSTANT)
    Q_PROPERTY(QString currentMachine READ currentMachine NOTIFY currentMachineChanged)
    Q_PROPERTY(int currentMachineIndex READ currentMachineIndex WRITE setCurrentMachineIndex NOTIFY currentMachineChanged)

    // General Options
    Q_PROPERTY(int emulationSpeed READ emulationSpeed WRITE setEmulationSpeed NOTIFY settingsCurrentChanged)
    Q_PROPERTY(bool fastLoad READ fastLoad WRITE setFastLoad NOTIFY settingsCurrentChanged)
    Q_PROPERTY(bool accelerateLoader READ accelerateLoader WRITE setAccelerateLoader NOTIFY settingsCurrentChanged)
    Q_PROPERTY(bool autoLoad READ autoLoad WRITE setAutoLoad NOTIFY settingsCurrentChanged)
    Q_PROPERTY(bool detectLoaders READ detectLoaders WRITE setDetectLoaders NOTIFY settingsCurrentChanged)
    Q_PROPERTY(bool restrictToSpectacol READ restrictToSpectacol WRITE setRestrictToSpectacol NOTIFY settingsCurrentChanged)


    // Sound Options
    Q_PROPERTY(bool soundEnabled READ soundEnabled WRITE setSoundEnabled NOTIFY settingsCurrentChanged)
    Q_PROPERTY(bool loadingSound READ loadingSound WRITE setLoadingSound NOTIFY settingsCurrentChanged)
    Q_PROPERTY(bool soundForce8Bit READ soundForce8Bit WRITE setSoundForce8Bit NOTIFY settingsCurrentChanged)
    Q_PROPERTY(QString AYStereoSeparation READ AYStereoSeparation WRITE setAYStereoSeparation NOTIFY settingsCurrentChanged)
    Q_PROPERTY(QString speakerType READ speakerType WRITE setSpeakerType NOTIFY settingsCurrentChanged)


    // Peripherals Options
    Q_PROPERTY(bool kempstonJoystick READ kempstonJoystick WRITE setKempstonJoystick NOTIFY settingsCurrentChanged)
    Q_PROPERTY(bool kempstonMouse READ kempstonMouse WRITE setKempstonMouse NOTIFY settingsCurrentChanged)
    Q_PROPERTY(bool interface1 READ interface1 WRITE setInterface1 NOTIFY settingsCurrentChanged)
    Q_PROPERTY(bool interface2 READ interface2 WRITE setInterface2 NOTIFY settingsCurrentChanged)
    Q_PROPERTY(bool full48kOSK READ full48kOSK WRITE setFull48kOSK NOTIFY settingsCurrentChanged)

public:
    explicit FuseSettings(QObject *parent = 0);

    QStringList machinesModel() const;
    QString currentMachine();
    int currentMachineIndex() const;
    void setCurrentMachineIndex(int idx);

    int emulationSpeed() const;
    void setEmulationSpeed(int speed);

    bool fastLoad() const;
    void setFastLoad(bool fastLoad);

    bool accelerateLoader() const;
    void setAccelerateLoader(bool accelerateLoader);

    bool autoLoad() const;
    void setAutoLoad(bool autoLoad);

    bool detectLoaders() const;
    void setDetectLoaders(bool detectLoaders);

    bool soundEnabled() const;
    void setSoundEnabled(bool soundEnabled);

    bool loadingSound() const;
    void setLoadingSound(bool loadingSound);

    bool soundForce8Bit() const;
    void setSoundForce8Bit(bool force8Bit);

    QString AYStereoSeparation() const;
    void setAYStereoSeparation(const QString &AYStereoSeparation);

    QString speakerType() const;
    void setSpeakerType(const QString &speakerType);

    bool kempstonJoystick() const;
    void setKempstonJoystick(bool kempstonJoystick);

    bool kempstonMouse() const;
    void setKempstonMouse(bool kempstonMouse);

    bool interface1() const;
    void setInterface1(bool interface1);

    bool interface2() const;
    void setInterface2(bool interface2);

    bool full48kOSK() const;
    void setFull48kOSK(bool full48kOSK);

    bool restrictToSpectacol() const;
    void setRestrictToSpectacol(bool restrictToSpectacol);

public slots:

signals:
    void currentMachineChanged();
    void settingsCurrentChanged();
};

#endif // FUSESETTINGS_H
