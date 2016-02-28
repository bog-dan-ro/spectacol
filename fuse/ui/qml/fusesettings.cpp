#include "fusesettings.h"
#include "qmlui.h"

#include <QDebug>

#include <machine.h>
#include <settings.h>
#include <sound.h>

#define safe_set(S,V) \
    pokeEvent([this, V]{ \
        if (S == V) \
            return; \
        S = V; \
        callFunction([this]{ emit settingsCurrentChanged(); }); \
    });

#define safe_set_string(S,V) \
    pokeEvent([this, V]{ \
        if (QLatin1String(S) == V) \
            return; \
        free(S); \
        S = strdup(V.toLatin1().constData()); qDebug() << S; \
        callFunction([this]{ emit settingsCurrentChanged(); }); \
    });

FuseSettings::FuseSettings(QObject *parent)
    : FuseObject(parent)
{
    pokeEvent([this]{
        callFunction([this]{ emit settingsCurrentChanged(); });
    });
}

QStringList FuseSettings::machinesModel() const
{
    QStringList res;
    for (int i = 0; i < machine_count; i++)
        res << QLatin1String(libspectrum_machine_name(machine_types[i]->machine));
    return res;
}

QString FuseSettings::currentMachine()
{
    if (machine_current)
        return QLatin1String(libspectrum_machine_name(machine_current->machine));

    pokeEvent([this]{
        callFunction([this]{ emit currentMachineChanged(); });
    });
    return QString();
}

int FuseSettings::currentMachineIndex() const
{
    for (int i = 0 ; i < machine_count; i++)
        if (machine_types[i]->machine == machine_current->machine)
            return i;
    return 0;
}

void FuseSettings::setCurrentMachineIndex(int idx)
{
    pokeEvent([this, idx]{
        if (machine_current->machine == machine_types[idx]->machine)
            return;
        machine_select(machine_types[idx]->machine);
        callFunction([this]{ emit currentMachineChanged(); emit settingsCurrentChanged();});
    });
}

int FuseSettings::emulationSpeed() const
{
    return settings_current.emulation_speed;
}

void FuseSettings::setEmulationSpeed(int speed)
{
    safe_set(settings_current.emulation_speed,speed);
}

bool FuseSettings::fastLoad() const
{
    return settings_current.fastload;
}

void FuseSettings::setFastLoad(bool fastLoad)
{
    safe_set(settings_current.fastload, fastLoad);
}

bool FuseSettings::accelerateLoader() const
{
    return settings_current.accelerate_loader;
}

void FuseSettings::setAccelerateLoader(bool accelerateLoader)
{
    safe_set(settings_current.accelerate_loader, accelerateLoader);
}

bool FuseSettings::autoLoad() const
{
    return settings_current.auto_load;
}

void FuseSettings::setAutoLoad(bool autoLoad)
{
    safe_set(settings_current.auto_load, autoLoad);
}

bool FuseSettings::detectLoaders() const
{
    return settings_current.detect_loader;
}

void FuseSettings::setDetectLoaders(bool detectLoaders)
{
    safe_set(settings_current.detect_loader, detectLoaders);
}

bool FuseSettings::soundEnabled() const
{
    return settings_current.sound;
}

void FuseSettings::setSoundEnabled(bool soundEnabled)
{
    safe_set(settings_current.sound, soundEnabled);
}

bool FuseSettings::loadingSound() const
{
    return settings_current.sound_load;
}

void FuseSettings::setLoadingSound(bool loadingSound)
{
    safe_set(settings_current.sound_load, loadingSound);
}

bool FuseSettings::soundForce8Bit() const
{
    return settings_current.sound_force_8bit;
}

void FuseSettings::setSoundForce8Bit(bool force8Bit)
{
    safe_set(settings_current.sound_force_8bit, force8Bit);
}

QString FuseSettings::AYStereoSeparation() const
{
    return QLatin1String(settings_current.stereo_ay);
}

void FuseSettings::setAYStereoSeparation(const QString &AYStereoSeparation)
{
    safe_set_string(settings_current.stereo_ay, AYStereoSeparation);
}

QString FuseSettings::speakerType() const
{
    return QLatin1String(settings_current.speaker_type);
}

void FuseSettings::setSpeakerType(const QString &speakerType)
{
    safe_set_string(settings_current.speaker_type, speakerType);
}

bool FuseSettings::kempstonJoystick() const
{
    return settings_current.joy_kempston;
}

void FuseSettings::setKempstonJoystick(bool kempstonJoystick)
{
    safe_set(settings_current.joy_kempston, kempstonJoystick);
}

bool FuseSettings::kempstonMouse() const
{
    return settings_current.kempston_mouse;
}

void FuseSettings::setKempstonMouse(bool kempstonMouse)
{
    safe_set(settings_current.kempston_mouse, kempstonMouse);
}

bool FuseSettings::interface1() const
{
    return settings_current.interface1;
}

void FuseSettings::setInterface1(bool interface1)
{
    safe_set(settings_current.interface1, interface1);
}

bool FuseSettings::interface2() const
{
    return settings_current.interface2;
}

void FuseSettings::setInterface2(bool interface2)
{
    safe_set(settings_current.interface2, interface2);
}
