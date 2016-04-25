/*
    Copyright (c) 2016, BogDan Vatra <bogdan@kde.org>

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

#include "fusesettings.h"
#include "qmlui.h"

#include <QSettings>

#include <machine.h>
#include <settings.h>
#include <sound.h>
#include <fuseemulator.h>

#ifdef Q_OS_ANDROID
# include <QtAndroid>
#endif

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
        S = strdup(V.toLatin1().constData()); \
        callFunction([this]{ emit settingsCurrentChanged(); }); \
    });

FuseSettings::FuseSettings(QObject *parent)
    : FuseObject(parent)
{
    pokeEvent([this]{
        callFunction([this]{ emit settingsCurrentChanged(); });
    });

    {
        QSettings s;
        s.beginGroup(QLatin1String("General"));
        m_hasStartButton = s.value("hasStartButton", false).toBool();
    }
    setScreenOrientation(screenOrientation());
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
        g_fuseEmulator->resetLoadedFile();
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

bool FuseSettings::full48kOSK() const
{
    QSettings s;
    s.beginGroup(QLatin1String("Peripherals"));
    return s.value("full48kOSK", true).toBool();
}

void FuseSettings::setFull48kOSK(bool full48kOSK)
{
    QSettings s;
    s.beginGroup(QLatin1String("Peripherals"));
    s.setValue("full48kOSK", full48kOSK);
    emit settingsCurrentChanged();
}

bool FuseSettings::restrictToSpectacol() const
{
    QSettings s;
    s.beginGroup(QLatin1String("General"));
    return s.value("restrictToSpectacol", true).toBool();
}

void FuseSettings::setRestrictToSpectacol(bool restrictBrowse)
{
    if (restrictToSpectacol() == restrictBrowse)
        return;

    QSettings s;
    s.beginGroup(QLatin1String("General"));
    s.setValue("restrictToSpectacol", restrictBrowse);
    emit settingsCurrentChanged();
}

bool FuseSettings::hasStartButton() const
{
    return m_hasStartButton;
}

void FuseSettings::setHasStartButton(bool hasStartButton)
{
    if (m_hasStartButton == hasStartButton)
        return;

    QSettings s;
    s.beginGroup(QLatin1String("General"));
    s.setValue("hasStartButton", hasStartButton);

    m_hasStartButton = hasStartButton;
    emit hasStartButtonChanged(hasStartButton);
}

bool FuseSettings::showOrientationChooser() const
{
#ifndef Q_OS_ANDROID
    return false;
#else
    static const auto uiMode = QAndroidJniObject::getStaticObjectField("android/content/Context", "UI_MODE_SERVICE", "Ljava/lang/String;");
    auto service =  QtAndroid::androidActivity().callObjectMethod("getSystemService",
                                                            "(Ljava/lang/String;)Ljava/lang/Object;",
                                                            uiMode.object());
    static const auto typeTv = QAndroidJniObject::getStaticField<jint>("android.content.res.Configuration", "UI_MODE_TYPE_TELEVISION");
    return service.callMethod<jint>("getCurrentModeType") != typeTv;
#endif
}


int FuseSettings::screenOrientation() const
{
#ifndef Q_OS_ANDROID
    return -1;
#else
    QSettings s;
    s.beginGroup(QLatin1String("Screen"));
    return s.value("orientation", Landscape).toInt();
#endif
}

void FuseSettings::setScreenOrientation(int orientation)
{
#ifndef Q_OS_ANDROID
    return;
#else
    static const int fullSensor = QAndroidJniObject::getStaticField<jint>("android/content/pm/ActivityInfo", "SCREEN_ORIENTATION_FULL_SENSOR");
    static const int sensorLandscape = QAndroidJniObject::getStaticField<jint>("android/content/pm/ActivityInfo", "SCREEN_ORIENTATION_SENSOR_LANDSCAPE");
    static const int sensorPortrait = QAndroidJniObject::getStaticField<jint>("android/content/pm/ActivityInfo", "SCREEN_ORIENTATION_SENSOR_PORTRAIT");
    int request;
    switch (orientation) {
    case Sensors:
        request = fullSensor;
        break;
    case Landscape:
        request = sensorLandscape;
        break;
    case Portrait:
        request = sensorPortrait;
        break;
    default:
        return;
    }
    QtAndroid::runOnAndroidThread([request]{
        QtAndroid::androidActivity().callMethod<void>("setRequestedOrientation", "(I)V", request);
    });
    QSettings s;
    s.beginGroup(QLatin1String("Screen"));
    s.setValue("orientation", orientation);
    emit screenOrientationChanged(orientation);
#endif
}

FuseSettings::FillMode FuseSettings::fillMode() const
{
    QSettings s;
    s.beginGroup(QLatin1String("Screen"));
    return (FillMode) s.value("fillMode", PreserveAspectFit).toInt();
}

void FuseSettings::setFillMode(FuseSettings::FillMode fill)
{
    if (fillMode() == fill)
        return;

    QSettings s;
    s.beginGroup(QLatin1String("Screen"));
    s.setValue("fillMode", fill);

    emit fillModeChanged(fill);
}
