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

#include "zxsettings.h"
#include "zxemulator.h"
#include "nextsdcard.h"

#include <zxkit/core/Exception.h>
#include <zxkit/input/Joystick.h>
#include <zxkit/input/Mouse.h>
#include <zxkit/machines/MachineProfile.h>
#include <zxkit/peripherals/expansion/ExpansionPeripherals.h>
#include <zxkit/peripherals/storage/Interface1.h>

#include <QSettings>

using namespace Qt::StringLiterals;

#ifdef Q_OS_ANDROID
#  include <QCoreApplication>
#endif

namespace {

zxkit::AyStereoMode ayStereoMode(const QString &separation)
{
    if (separation == u"ACB"_s)
        return zxkit::AyStereoMode::ACB;
    if (separation == u"ABC"_s)
        return zxkit::AyStereoMode::ABC;
    return zxkit::AyStereoMode::Mono;
}

} // namespace

ZxSettings::ZxSettings(ZxEmulator &emulator)
    : QObject(&emulator)
    , m_zxEmulator(emulator)
{
    // The built-in machines, the ones the emulator's registry has
    const zxkit::MachineRegistry machines;
    m_machineIds = machines.availableIds();
    for (std::size_t i = 0; i < m_machineIds.size(); ++i) {
        auto prof = machines.create(m_machineIds[i]);
        m_machineNames << QString::fromUtf8(prof->name().data(), qsizetype(prof->name().size()));
        if (m_machineIds[i] == "48k")
            m_currentMachineIndex = int(i);
    }

    {
        QSettings s;
        s.beginGroup(u"General"_s);
        m_hasStartButton = s.value("hasStartButton", false).toBool();
        m_emulationSpeed = s.value("emulationSpeed", 100).toInt();
        m_autoLoad = s.value("autoLoad", true).toBool();
        m_detectLoaders = s.value("detectLoaders", true).toBool();
        m_loaderAcceleration = s.value("loaderAcceleration", 1).toInt();
        m_savedMachineId = s.value("machine", u"48k"_s).toString().toStdString();
        s.endGroup();

        s.beginGroup(u"Sound"_s);
        m_soundEnabled = s.value("soundEnabled", true).toBool();
        m_loadingSound = s.value("loadingSound", true).toBool();
        m_ayStereo = s.value("ayStereo", u"ACB"_s).toString();
        m_speakerType = s.value("speakerType", u"TV speaker"_s).toString();
        s.endGroup();

        s.beginGroup(u"Peripherals"_s);
        m_kempstonJoystick = s.value("kempstonJoystick", true).toBool();
        m_kempstonMouse = s.value("kempstonMouse", false).toBool();
        m_interface1 = s.value("interface1", false).toBool();
        m_interface2 = s.value("interface2", true).toBool();
        m_joystickPrompt = s.value("joystickPrompt", true).toBool();
        m_recreatedSpectrum = s.value("recreatedSpectrum", false).toBool();
        s.endGroup();

        s.beginGroup(u"Gamepad"_s);
        QVariantMap map = s.value(u"actionsMap"_s).toMap();
        for (auto it = map.begin(); it != map.end(); ++it)
            m_gamepadActions[it.key().toInt()] = it.value().toInt();
    }
    setScreenOrientation(screenOrientation());

    connect(&NextSdCard::instance(), &NextSdCard::ready, this, [this] {
        if (m_pendingMachineIndex >= 0)
            applyMachine(std::exchange(m_pendingMachineIndex, -1), m_pendingMachinePersist);
    });
}

QStringList ZxSettings::machinesModel() const
{
    return m_machineNames;
}

QString ZxSettings::currentMachine()
{
    if (m_currentMachineIndex >= 0 && m_currentMachineIndex < m_machineNames.size())
        return m_machineNames[m_currentMachineIndex];
    return QString();
}

int ZxSettings::currentMachineIndex() const
{
    return m_currentMachineIndex;
}

void ZxSettings::applyToEmulator()
{
    m_zxEmulator.emulator().writeLock()->audioMixer().setStereoMode(ayStereoMode(m_ayStereo));
    m_zxEmulator.setSpeed(m_emulationSpeed);
    // One by one: a peripheral that can't start (e.g. a missing ROM) must not stop the others
    if (m_kempstonJoystick)
        m_zxEmulator.withEmulator([](zxkit::Emulator &emulator) { emulator.attachPeripheral(std::make_unique<zxkit::KempstonJoystickPeripheral>()); });
    if (m_kempstonMouse)
        m_zxEmulator.withEmulator([](zxkit::Emulator &emulator) { emulator.attachPeripheral(std::make_unique<zxkit::KempstonMousePeripheral>()); });
    if (m_interface1)
        m_zxEmulator.withEmulator([](zxkit::Emulator &emulator) { emulator.attachPeripheral(std::make_unique<zxkit::Interface1Peripheral>()); });
    if (m_interface2)
        m_zxEmulator.withEmulator([](zxkit::Emulator &emulator) { emulator.attachPeripheral(std::make_unique<zxkit::Interface2Peripheral>()); });

    // The machine picked last time. The Next needs its SD card image: without it
    // stay on the 48K, picking the Next again offers to download it.
    const auto it = std::ranges::find(m_machineIds, m_savedMachineId);
    if (it == m_machineIds.end() || (*it == "next" && !NextSdCard::instance().available()))
        return;
    applyMachine(int(it - m_machineIds.begin()), false);
}

void ZxSettings::setCurrentMachineIndex(int idx)
{
    applyMachine(idx, true);
}

void ZxSettings::applyMachine(int idx, bool persist)
{
    if (idx < 0 || size_t(idx) >= m_machineIds.size() || idx == m_currentMachineIndex)
        return;

    const std::string &machineId = m_machineIds[size_t(idx)];
    if (machineId == "next") {
        // The Next boots NextZXOS (and loads its ROMs) from an SD card image
        auto &sdCard = NextSdCard::instance();
        if (!sdCard.available()) {
            m_pendingMachineIndex = idx;
            m_pendingMachinePersist = persist;
            sdCard.requestDownload();
            emit currentMachineChanged();
            return;
        }
        if (!m_zxEmulator.withEmulator([&sdCard](zxkit::Emulator &emulator) { emulator.insertSdCard(sdCard.imagePath().toStdString()); }))
            return;
    }

    try {
        // On failure the emulator keeps the previous machine
        m_zxEmulator.emulator().writeLock()->selectMachine(machineId);
        m_currentMachineIndex = idx;
        if (persist) {
            QSettings s;
            s.beginGroup(u"General"_s);
            s.setValue("machine", QString::fromStdString(machineId));
        }
    } catch (const zxkit::Exception &e) {
        m_zxEmulator.showMessage(tr("Can't select the machine: %1").arg(QString::fromUtf8(e.rawMessage())), ZxEmulator::Error);
    } catch (const std::exception &e) {
        m_zxEmulator.showMessage(tr("Can't select the machine: %1").arg(QString::fromUtf8(e.what())), ZxEmulator::Error);
    }
    m_zxEmulator.engine().refreshFrame();
    emit currentMachineChanged();
    emit settingsCurrentChanged();
    m_zxEmulator.resetLoadedFile();
}

bool ZxSettings::selectMachine(const QString &machineId)
{
    const auto it = std::ranges::find(m_machineIds, machineId.toStdString());
    if (it == m_machineIds.end())
        return false;
    applyMachine(int(it - m_machineIds.begin()), false);
    return true;
}

QStringList ZxSettings::machineIds() const
{
    QStringList ids;
    for (const auto &id : m_machineIds)
        ids << QString::fromStdString(id);
    return ids;
}

int ZxSettings::emulationSpeed() const
{
    return m_emulationSpeed;
}

void ZxSettings::setEmulationSpeed(int speed)
{
    if (m_emulationSpeed == speed)
        return;
    m_emulationSpeed = speed;
    QSettings s;
    s.beginGroup(u"General"_s);
    s.setValue("emulationSpeed", speed);
    m_zxEmulator.setSpeed(speed);
    emit settingsCurrentChanged();
}

bool ZxSettings::autoLoad() const
{
    return m_autoLoad;
}

void ZxSettings::setAutoLoad(bool autoLoad)
{
    if (m_autoLoad == autoLoad)
        return;
    m_autoLoad = autoLoad;
    QSettings s;
    s.beginGroup(u"General"_s);
    s.setValue("autoLoad", autoLoad);
    emit settingsCurrentChanged();
}

bool ZxSettings::detectLoaders() const
{
    return m_detectLoaders;
}

void ZxSettings::setDetectLoaders(bool detectLoaders)
{
    if (m_detectLoaders == detectLoaders)
        return;
    m_detectLoaders = detectLoaders;
    QSettings s;
    s.beginGroup(u"General"_s);
    s.setValue("detectLoaders", detectLoaders);
    emit settingsCurrentChanged();
}

bool ZxSettings::soundEnabled() const
{
    return m_soundEnabled;
}

void ZxSettings::setSoundEnabled(bool soundEnabled)
{
    if (m_soundEnabled == soundEnabled)
        return;
    m_soundEnabled = soundEnabled;
    QSettings s;
    s.beginGroup(u"Sound"_s);
    s.setValue("soundEnabled", soundEnabled);
    emit settingsCurrentChanged();
}

bool ZxSettings::loadingSound() const
{
    return m_loadingSound;
}

void ZxSettings::setLoadingSound(bool loadingSound)
{
    if (m_loadingSound == loadingSound)
        return;
    m_loadingSound = loadingSound;
    QSettings s;
    s.beginGroup(u"Sound"_s);
    s.setValue("loadingSound", loadingSound);
    emit settingsCurrentChanged();
}

QString ZxSettings::AYStereoSeparation() const
{
    return m_ayStereo;
}

void ZxSettings::setAYStereoSeparation(const QString &AYStereoSeparation)
{
    if (m_ayStereo == AYStereoSeparation)
        return;
    m_ayStereo = AYStereoSeparation;
    QSettings s;
    s.beginGroup(u"Sound"_s);
    s.setValue("ayStereo", AYStereoSeparation);

    m_zxEmulator.emulator().writeLock()->audioMixer().setStereoMode(ayStereoMode(AYStereoSeparation));
    emit settingsCurrentChanged();
}

QString ZxSettings::speakerType() const
{
    return m_speakerType;
}

void ZxSettings::setSpeakerType(const QString &speakerType)
{
    if (m_speakerType == speakerType)
        return;
    m_speakerType = speakerType;
    QSettings s;
    s.beginGroup(u"Sound"_s);
    s.setValue("speakerType", speakerType);
    emit settingsCurrentChanged();
}

bool ZxSettings::kempstonJoystick() const
{
    return m_kempstonJoystick;
}

void ZxSettings::setKempstonJoystick(bool kempstonJoystick)
{
    if (m_kempstonJoystick == kempstonJoystick)
        return;
    m_kempstonJoystick = kempstonJoystick;
    QSettings s;
    s.beginGroup(u"Peripherals"_s);
    s.setValue("kempstonJoystick", kempstonJoystick);
    m_zxEmulator.withEmulator([kempstonJoystick](zxkit::Emulator &emulator) {
        if (kempstonJoystick)
            emulator.attachPeripheral(std::make_unique<zxkit::KempstonJoystickPeripheral>());
        else
            emulator.detachPeripheral("kempston");
    });
    emit settingsCurrentChanged();
}

bool ZxSettings::kempstonMouse() const
{
    return m_kempstonMouse;
}

void ZxSettings::setKempstonMouse(bool kempstonMouse)
{
    if (m_kempstonMouse == kempstonMouse)
        return;
    m_kempstonMouse = kempstonMouse;
    QSettings s;
    s.beginGroup(u"Peripherals"_s);
    s.setValue("kempstonMouse", kempstonMouse);
    m_zxEmulator.withEmulator([kempstonMouse](zxkit::Emulator &emulator) {
        if (kempstonMouse)
            emulator.attachPeripheral(std::make_unique<zxkit::KempstonMousePeripheral>());
        else
            emulator.detachPeripheral("kempston_mouse");
    });
    emit settingsCurrentChanged();
}

bool ZxSettings::interface1() const
{
    return m_interface1;
}

void ZxSettings::setInterface1(bool interface1)
{
    if (m_interface1 == interface1)
        return;
    m_interface1 = interface1;
    QSettings s;
    s.beginGroup(u"Peripherals"_s);
    s.setValue("interface1", interface1);
    m_zxEmulator.withEmulator([interface1](zxkit::Emulator &emulator) {
        if (interface1)
            emulator.attachPeripheral(std::make_unique<zxkit::Interface1Peripheral>());
        else
            emulator.detachPeripheral("interface1");
    });
    emit settingsCurrentChanged();
}

bool ZxSettings::interface2() const
{
    return m_interface2;
}

void ZxSettings::setInterface2(bool interface2)
{
    if (m_interface2 == interface2)
        return;
    m_interface2 = interface2;
    QSettings s;
    s.beginGroup(u"Peripherals"_s);
    s.setValue("interface2", interface2);
    m_zxEmulator.withEmulator([interface2](zxkit::Emulator &emulator) {
        if (interface2)
            emulator.attachPeripheral(std::make_unique<zxkit::Interface2Peripheral>());
        else
            emulator.detachPeripheral("interface2");
    });
    emit settingsCurrentChanged();
}

bool ZxSettings::full48kOSK() const
{
    QSettings s;
    s.beginGroup(u"Peripherals"_s);
    return s.value("full48kOSK", true).toBool();
}

void ZxSettings::setFull48kOSK(bool full48kOSK)
{
    QSettings s;
    s.beginGroup(u"Peripherals"_s);
    s.setValue("full48kOSK", full48kOSK);
    emit settingsCurrentChanged();
}

bool ZxSettings::restrictToSpectacol() const
{
    QSettings s;
    s.beginGroup(u"General"_s);
    return s.value("restrictToSpectacol", false).toBool();
}

void ZxSettings::setRestrictToSpectacol(bool restrictBrowse)
{
    if (restrictToSpectacol() == restrictBrowse)
        return;

    QSettings s;
    s.beginGroup(u"General"_s);
    s.setValue("restrictToSpectacol", restrictBrowse);
    emit settingsCurrentChanged();
}

bool ZxSettings::hasStartButton() const
{
    return m_hasStartButton;
}

void ZxSettings::setHasStartButton(bool hasStartButton)
{
    if (m_hasStartButton == hasStartButton)
        return;

    QSettings s;
    s.beginGroup(u"General"_s);
    s.setValue("hasStartButton", hasStartButton);

    m_hasStartButton = hasStartButton;
    emit hasStartButtonChanged(hasStartButton);
}

bool ZxSettings::showOrientationChooser() const
{
#ifndef Q_OS_ANDROID
    return false;
#else
    static const auto uiMode = QJniObject::getStaticObjectField("android/content/Context", "UI_MODE_SERVICE", "Ljava/lang/String;");
    auto service = QJniObject{QNativeInterface::QAndroidApplication::context()}
                       .callObjectMethod("getSystemService",
                                         "(Ljava/lang/String;)Ljava/lang/Object;",
                                         uiMode.object());
    static const auto typeTv = QJniObject::getStaticField<jint>("android.content.res.Configuration", "UI_MODE_TYPE_TELEVISION");
    return service.callMethod<jint>("getCurrentModeType") != typeTv;
#endif
}


int ZxSettings::screenOrientation() const
{
#ifndef Q_OS_ANDROID
    return -1;
#else
    QSettings s;
    s.beginGroup(u"Screen"_s);
    return s.value("orientation", Landscape).toInt();
#endif
}

void ZxSettings::setScreenOrientation(int orientation)
{
#ifndef Q_OS_ANDROID
    Q_UNUSED(orientation);
    return;
#else
    static const int fullSensor = QJniObject::getStaticField<jint>("android/content/pm/ActivityInfo", "SCREEN_ORIENTATION_FULL_SENSOR");
    static const int sensorLandscape = QJniObject::getStaticField<jint>("android/content/pm/ActivityInfo", "SCREEN_ORIENTATION_SENSOR_LANDSCAPE");
    static const int sensorPortrait = QJniObject::getStaticField<jint>("android/content/pm/ActivityInfo", "SCREEN_ORIENTATION_SENSOR_PORTRAIT");
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
    QNativeInterface::QAndroidApplication::runOnAndroidMainThread([request]{
        QJniObject{QNativeInterface::QAndroidApplication::context()}.callMethod<void>("setRequestedOrientation", "(I)V", request);
    });
    QSettings s;
    s.beginGroup(u"Screen"_s);
    s.setValue("orientation", orientation);
    emit screenOrientationChanged(orientation);
#endif
}

ZxSettings::FillMode ZxSettings::fillMode() const
{
    QSettings s;
    s.beginGroup(u"Screen"_s);
    return (FillMode) s.value("fillMode", PreserveAspectFit).toInt();
}

void ZxSettings::setFillMode(ZxSettings::FillMode fill)
{
    if (fillMode() == fill)
        return;

    QSettings s;
    s.beginGroup(u"Screen"_s);
    s.setValue("fillMode", fill);

    emit fillModeChanged(fill);
}

bool ZxSettings::smoothScaling() const
{
    QSettings s;
    s.beginGroup(u"Screen"_s);
    return s.value("smoothScaling", false).toBool();
}

void ZxSettings::setSmoothScaling(bool smoothScaling)
{
    if (this->smoothScaling() == smoothScaling)
        return;

    QSettings s;
    s.beginGroup(u"Screen"_s);
    s.setValue("smoothScaling", smoothScaling);

    emit smoothScalingChanged(smoothScaling);
}

bool ZxSettings::swipe4menu() const
{
    QSettings s;
    s.beginGroup(u"General"_s);
    return s.value("swipe4menu", true).toBool();
}

void ZxSettings::setSwipe4menu(bool swipe)
{
    if (swipe == swipe4menu())
        return;

    QSettings s;
    s.beginGroup(u"General"_s);
    s.setValue("swipe4menu", swipe);
    emit swipe4menuChanged(swipe);
}

bool ZxSettings::autoSaveOnExit() const
{
    QSettings s;
    s.beginGroup(u"General"_s);
    return s.value("autoSaveOnExit", false).toBool();
}

void ZxSettings::setAutoSaveOnExit(bool autoSave)
{
    if (autoSave == autoSaveOnExit())
        return;

    QSettings s;
    s.beginGroup(u"General"_s);
    s.setValue("autoSaveOnExit", autoSave);
    emit autoSaveOnExitChanged(autoSave);
}

bool ZxSettings::leftMargin() const
{
    QSettings s;
    s.beginGroup(u"Screen"_s);
    return s.value("leftMargin", false).toBool();
}

void ZxSettings::setLeftMargin(bool border)
{
    if (leftMargin() == border)
        return;

    QSettings s;
    s.beginGroup(u"Screen"_s);
    s.setValue("leftMargin", border);

    emit leftMarginChanged(border);
}

bool ZxSettings::joystickPrompt() const
{
    return m_joystickPrompt;
}

void ZxSettings::setJoystickPrompt(bool joystickPrompt)
{
    if (m_joystickPrompt == joystickPrompt)
        return;
    m_joystickPrompt = joystickPrompt;
    QSettings s;
    s.beginGroup(u"Peripherals"_s);
    s.setValue("joystickPrompt", joystickPrompt);
    emit settingsCurrentChanged();
}

int ZxSettings::loaderAcceleration() const
{
    return m_loaderAcceleration;
}

void ZxSettings::setLoaderAcceleration(int acceleration)
{
    if (acceleration == m_loaderAcceleration)
        return;
    m_loaderAcceleration = acceleration;
    QSettings s;
    s.beginGroup(u"General"_s);
    s.setValue("loaderAcceleration", acceleration);
    emit settingsCurrentChanged();
}

qreal ZxSettings::deadZone() const
{
    QSettings s;
    s.beginGroup(u"Peripherals"_s);
    return s.value("deadZone", 0.4).toDouble();
}

void ZxSettings::setDeadZone(qreal dz)
{
    if (deadZone() == dz)
        return;

    QSettings s;
    s.beginGroup(u"Peripherals"_s);
    s.setValue("deadZone", dz);

    emit deadZoneChanged(dz);
}

int ZxSettings::messageLevel() const
{
    QSettings s;
    s.beginGroup(u"General"_s);
    return s.value("messageLevel", 0).toInt();
}

void ZxSettings::setMessageLevel(int level)
{
    if (level == messageLevel())
        return;

    QSettings s;
    s.beginGroup(u"General"_s);
    s.setValue("messageLevel", level);
    emit messageLevelChanged(level);
}

bool ZxSettings::recreatedSpectrum() const
{
    return m_recreatedSpectrum;
}

void ZxSettings::setRecreatedSpectrum(bool recreatedSpectrum)
{
    if (m_recreatedSpectrum == recreatedSpectrum)
        return;
    m_recreatedSpectrum = recreatedSpectrum;
    QSettings s;
    s.beginGroup(u"Peripherals"_s);
    s.setValue("recreatedSpectrum", recreatedSpectrum);
    emit settingsCurrentChanged();
}

void ZxSettings::setGamepadAction(int gamepadButton, int action)
{
    std::unique_lock<std::mutex> lock(m_gamepadActionsMutex);
    m_gamepadActions[gamepadButton] = action;
    QSettings s;
    s.beginGroup(u"Gamepad"_s);
    QVariantMap map;
    for (const auto &it : m_gamepadActions)
        map[QString::number(it.first)] = it.second;
    s.setValue(u"actionsMap"_s, map);
}
