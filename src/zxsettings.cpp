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

#include "zxsettings.h"
#include "zxemulator.h"
#include "nextsdcard.h"
#include "zximage.h"

#include <zxkit/core/Exception.h>
#include <zxkit/input/Joystick.h>
#include <zxkit/input/Mouse.h>
#include <zxkit/machines/MachineProfile.h>
#include <zxkit/peripherals/expansion/ExpansionPeripherals.h>
#include <zxkit/peripherals/storage/Interface1.h>

#include <QSettings>

#include <algorithm>
#include <cmath>
#include <string_view>

using namespace Qt::StringLiterals;
using namespace std::literals;

#ifdef Q_OS_ANDROID
#include <QCoreApplication>
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

zxkit::SpeakerModel speakerModel(const QString &type)
{
    if (type == u"Beeper"_s)
        return zxkit::SpeakerModel::Beeper;
    if (type == u"Unfiltered"_s)
        return zxkit::SpeakerModel::Unfiltered;
    return zxkit::SpeakerModel::TV;
}

bool validSpeakerType(const QString &type)
{
    return type == u"TV speaker"_s || type == u"Beeper"_s || type == u"Unfiltered"_s;
}

} // namespace

ZxSettings::ZxSettings(ZxEmulator &emulator) : QObject(&emulator), m_zxEmulator(emulator)
{
    // The built-in machines, the ones the emulator's registry has
    const zxkit::MachineRegistry machines;
    m_machineIds = machines.availableIds();
    for (std::size_t i = 0; i < m_machineIds.size(); ++i) {
        auto prof = machines.create(m_machineIds[i]);
        m_machineNames << QString::fromUtf8(prof->name().data(), qsizetype(prof->name().size()));
        if (m_machineIds[i] == "48k"sv)
            m_currentMachineIndex = int(i);
    }

    {
        QSettings s;
        s.beginGroup(u"Screen"_s);
        m_borderMargins = { uint16_t(std::clamp(s.value(u"borderLeft"_s, 32).toInt(), 0, 128)),
                            uint16_t(std::clamp(s.value(u"borderTop"_s, 32).toInt(), 0, 128)),
                            uint16_t(std::clamp(s.value(u"borderRight"_s, 32).toInt(), 0, 128)),
                            uint16_t(std::clamp(s.value(u"borderBottom"_s, 32).toInt(), 0, 128)) };
        s.endGroup();
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
        m_ayModel = s.value(u"ayModel"_s, u"AY8912"_s).toString();
        if (m_ayModel != u"YM2149"_s)
            m_ayModel = u"AY8912"_s;
        m_speakerType = s.value("speakerType", u"TV speaker"_s).toString();
        if (!validSpeakerType(m_speakerType))
            m_speakerType = u"TV speaker"_s;
        s.endGroup();

        s.beginGroup(u"Peripherals"_s);
        m_kempstonJoystick = s.value("kempstonJoystick", true).toBool();
        m_kempstonMouse = s.value("kempstonMouse", false).toBool();
        m_interface1 = s.value("interface1", false).toBool();
        m_interface2 = s.value("interface2", true).toBool();
        const double deadZone = s.value(u"deadZone"_s, 0.4).toDouble();
        m_deadZone = std::isfinite(deadZone) ? std::clamp(deadZone, 0.0, 0.95) : 0.4;
        m_joystickPrompt = s.value("joystickPrompt", true).toBool();
        m_recreatedSpectrum = s.value("recreatedSpectrum", false).toBool();
        s.endGroup();

        s.beginGroup(u"Gamepad"_s);
        m_fastForwardToggle = s.value(u"fastForwardToggle"_s, false).toBool();
        m_directionMapping =
                std::clamp(s.value(u"directionMapping"_s, int(JoystickDirections)).toInt(), 0, 2);
        const auto directions = s.value(u"directionKeys"_s).toList();
        for (int i = 0; i < std::min(int(directions.size()), int(ControllerInput::DirectionCount));
             ++i)
            if (ControllerInput::isSpectrumKey(directions[i].toInt()))
                m_directionKeys[i] = directions[i].toInt();
        QVariantMap map = s.value(u"actionsMap"_s).toMap();
        for (auto it = map.begin(); it != map.end(); ++it)
            m_gamepadActions[it.key().toInt()] = it.value().toInt();
        s.endGroup();
        s.beginGroup(u"Controls"_s);
        auto opacity = [&s](const QString &key) {
            const double value = s.value(key, 1.0).toDouble();
            return std::isfinite(value) ? std::clamp(value, 0.1, 1.0) : 1.0;
        };
        m_keyboardOpacity = opacity(u"keyboardOpacity"_s);
        m_joystickOpacity = opacity(u"joystickOpacity"_s);
        m_buttonsOpacity = opacity(u"buttonsOpacity"_s);
    }
    connect(ZxImage::instance(), &ZxImage::sourceGeometryChanged, this,
            &ZxSettings::borderLimitsChanged, Qt::QueuedConnection);

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

void ZxSettings::applyTapeOptions()
{
    zxkit::TapeOptions options;
    options.autoPlay = m_detectLoaders;
    // "None", "Safe" (the edge loops are fast-forwarded), "Turbo" (and the emulation runs flat out)
    options.accelerateLoaders = m_loaderAcceleration > 0;
    options.turboWhilePlaying = m_loaderAcceleration > 1;
    options.loadingSound = m_loadingSound;
    m_zxEmulator.emulator().writeLock()->tapeDeck().setOptions(options);
}

void ZxSettings::applyToEmulator()
{
    ZxImage::instance()->setBorderMargins(m_borderMargins);
    m_zxEmulator.emulator().writeLock()->audioMixer().setStereoMode(ayStereoMode(m_ayStereo));
    m_zxEmulator.emulator().writeLock()->audioMixer().setAyModel(
            m_ayModel == u"YM2149"_s ? zxkit::AyModel::YM2149 : zxkit::AyModel::AY8912);
    m_zxEmulator.emulator().writeLock()->audioMixer().setSpeakerModel(speakerModel(m_speakerType));
    applyTapeOptions();
    m_zxEmulator.setSpeed(m_emulationSpeed);
    // One by one: a peripheral that can't start (e.g. a missing ROM) must not stop the others
    if (m_kempstonJoystick)
        m_zxEmulator.withEmulator([](zxkit::Emulator &emulator) {
            emulator.attachPeripheral(std::make_unique<zxkit::KempstonJoystickPeripheral>());
        });
    if (m_kempstonMouse)
        m_zxEmulator.withEmulator([](zxkit::Emulator &emulator) {
            emulator.attachPeripheral(std::make_unique<zxkit::KempstonMousePeripheral>());
        });
    if (m_interface1)
        m_zxEmulator.withEmulator([](zxkit::Emulator &emulator) {
            emulator.attachPeripheral(std::make_unique<zxkit::Interface1Peripheral>());
        });
    if (m_interface2)
        m_zxEmulator.withEmulator([](zxkit::Emulator &emulator) {
            emulator.attachPeripheral(std::make_unique<zxkit::Interface2Peripheral>());
        });

    // The machine picked last time. The Next needs its SD card image: without it
    // stay on the 48K, picking the Next again offers to download it.
    const auto it = std::ranges::find(m_machineIds, m_savedMachineId);
    if (it == m_machineIds.end() || (*it == "next"sv && !NextSdCard::instance().available()))
        return;
    applyMachine(int(it - m_machineIds.begin()), false);
}

QVariantMap ZxSettings::borderLimits() const
{
    const auto limits = ZxImage::instance()->borderLimits();
    return { { u"left"_s, limits.left() },
             { u"top"_s, limits.top() },
             { u"right"_s, limits.right() },
             { u"bottom"_s, limits.bottom() } };
}

void ZxSettings::setBorderMargins(int left, int top, int right, int bottom)
{
    const zxkit::BorderMargins margins{ uint16_t(std::clamp(left, 0, 128)),
                                        uint16_t(std::clamp(top, 0, 128)),
                                        uint16_t(std::clamp(right, 0, 128)),
                                        uint16_t(std::clamp(bottom, 0, 128)) };
    if (margins == m_borderMargins)
        return;
    m_borderMargins = margins;
    QSettings s;
    s.beginGroup(u"Screen"_s);
    s.setValue(u"borderLeft"_s, margins.left);
    s.setValue(u"borderTop"_s, margins.top);
    s.setValue(u"borderRight"_s, margins.right);
    s.setValue(u"borderBottom"_s, margins.bottom);
    ZxImage::instance()->setBorderMargins(margins);
    emit borderMarginsChanged();
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
    if (machineId == "next"sv) {
        // The Next boots NextZXOS (and loads its ROMs) from an SD card image
        auto &sdCard = NextSdCard::instance();
        if (!sdCard.available()) {
            m_pendingMachineIndex = idx;
            m_pendingMachinePersist = persist;
            sdCard.requestDownload();
            emit currentMachineChanged();
            return;
        }
        if (!m_zxEmulator.withEmulator([&sdCard](zxkit::Emulator &emulator) {
                emulator.insertSdCard(sdCard.imagePath().toStdString());
            }))
            return;
    }

    try {
        // On failure the emulator keeps the previous machine
        m_zxEmulator.releaseInputs();
        m_zxEmulator.emulator().writeLock()->selectMachine(machineId);
        m_currentMachineIndex = idx;
        if (persist) {
            QSettings s;
            s.beginGroup(u"General"_s);
            s.setValue("machine", QString::fromStdString(machineId));
        }
    } catch (const zxkit::Exception &e) {
        m_zxEmulator.showMessage(
                tr("Can't select the machine: %1").arg(QString::fromUtf8(e.rawMessage())),
                ZxEmulator::Error);
    } catch (const std::exception &e) {
        m_zxEmulator.showMessage(
                tr("Can't select the machine: %1").arg(QString::fromUtf8(e.what())),
                ZxEmulator::Error);
    }
    m_zxEmulator.engine().refreshFrame();
    emit currentMachineChanged();
    emit settingsCurrentChanged();
    m_zxEmulator.resetLoadedFile();
}

void ZxSettings::syncCurrentMachine(std::string_view machineId)
{
    const auto it = std::ranges::find(m_machineIds, machineId);
    if (it == m_machineIds.end())
        return;
    const int index = int(it - m_machineIds.begin());
    if (index != m_currentMachineIndex) {
        m_currentMachineIndex = index;
        emit currentMachineChanged();
    }
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
    applyTapeOptions();
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
    applyTapeOptions();
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

    m_zxEmulator.emulator().writeLock()->audioMixer().setStereoMode(
            ayStereoMode(AYStereoSeparation));
    emit settingsCurrentChanged();
}

QString ZxSettings::speakerType() const
{
    return m_speakerType;
}

void ZxSettings::setAyModel(const QString &model)
{
    if ((model != u"AY8912"_s && model != u"YM2149"_s) || m_ayModel == model)
        return;
    m_ayModel = model;
    QSettings s;
    s.beginGroup(u"Sound"_s);
    s.setValue(u"ayModel"_s, model);
    m_zxEmulator.emulator().writeLock()->audioMixer().setAyModel(
            model == u"YM2149"_s ? zxkit::AyModel::YM2149 : zxkit::AyModel::AY8912);
    emit settingsCurrentChanged();
}

void ZxSettings::setSpeakerType(const QString &speakerType)
{
    if (!validSpeakerType(speakerType) || m_speakerType == speakerType)
        return;
    m_speakerType = speakerType;
    QSettings s;
    s.beginGroup(u"Sound"_s);
    s.setValue("speakerType", speakerType);
    m_zxEmulator.emulator().writeLock()->audioMixer().setSpeakerModel(speakerModel(speakerType));
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
            emulator.detachPeripheral("kempston"sv);
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
            emulator.detachPeripheral("kempston_mouse"sv);
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
            emulator.detachPeripheral("interface1"sv);
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
            emulator.detachPeripheral("interface2"sv);
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
    static const auto uiMode = QJniObject::getStaticObjectField(
            "android/content/Context", "UI_MODE_SERVICE", "Ljava/lang/String;");
    auto service = QJniObject{ QNativeInterface::QAndroidApplication::context() }.callObjectMethod(
            "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;", uiMode.object());
    static const auto typeTv = QJniObject::getStaticField<jint>("android.content.res.Configuration",
                                                                "UI_MODE_TYPE_TELEVISION");
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
    return s.value("orientation", Sensors).toInt();
#endif
}

void ZxSettings::setScreenOrientation(int orientation)
{
#ifndef Q_OS_ANDROID
    Q_UNUSED(orientation);
    return;
#else
    static const int fullSensor = QJniObject::getStaticField<jint>(
            "android/content/pm/ActivityInfo", "SCREEN_ORIENTATION_FULL_SENSOR");
    static const int sensorLandscape = QJniObject::getStaticField<jint>(
            "android/content/pm/ActivityInfo", "SCREEN_ORIENTATION_SENSOR_LANDSCAPE");
    static const int sensorPortrait = QJniObject::getStaticField<jint>(
            "android/content/pm/ActivityInfo", "SCREEN_ORIENTATION_SENSOR_PORTRAIT");
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
    QNativeInterface::QAndroidApplication::runOnAndroidMainThread([request] {
        QJniObject{ QNativeInterface::QAndroidApplication::context() }.callMethod<void>(
                "setRequestedOrientation", "(I)V", request);
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
    return (FillMode)s.value("fillMode", PreserveAspectFit).toInt();
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

bool ZxSettings::largeUi() const
{
    return QSettings{}.value(u"Ui/large"_s, false).toBool();
}

void ZxSettings::setLargeUi(bool large)
{
    if (large == largeUi())
        return;
    QSettings{}.setValue(u"Ui/large"_s, large);
    emit uiChanged();
}

bool ZxSettings::reduceMotion() const
{
    return QSettings{}.value(u"Ui/reduceMotion"_s, false).toBool();
}

void ZxSettings::setReduceMotion(bool reduce)
{
    if (reduce == reduceMotion())
        return;
    QSettings{}.setValue(u"Ui/reduceMotion"_s, reduce);
    emit uiChanged();
}

ZxSettings::UiMode ZxSettings::uiMode() const
{
    return UiMode(std::clamp(QSettings{}.value(u"Ui/mode"_s, int(AutoUi)).toInt(), int(AutoUi),
                             int(DesktopUi)));
}

void ZxSettings::setUiMode(UiMode mode)
{
    if (mode == uiMode())
        return;
    QSettings{}.setValue(u"Ui/mode"_s, int(mode));
    emit uiChanged();
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
    applyTapeOptions();
    emit settingsCurrentChanged();
}

qreal ZxSettings::deadZone() const
{
    return m_deadZone;
}

void ZxSettings::setDeadZone(qreal value)
{
    if (!std::isfinite(value))
        return;
    value = std::clamp(value, 0.0, 0.95);
    if (m_deadZone == value)
        return;
    m_deadZone = value;
    persistControllerMapping();
    emit deadZoneChanged(value);
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
    if (action != JoystickFire && !ControllerInput::isSpectrumKey(action)
        && (action < ToggleKeyboard || action > ToggleEmulationSpeed))
        return;
    std::unique_lock<std::mutex> lock(m_gamepadActionsMutex);
    const auto previous = m_gamepadActions.find(gamepadButton);
    if ((previous == m_gamepadActions.end() ? int(JoystickFire) : previous->second) == action)
        return;
    m_gamepadActions[gamepadButton] = action;
    lock.unlock();
    persistControllerMapping();
    emit controllerMappingChanged();
}

void ZxSettings::setDirectionMapping(int mode)
{
    if (mode < JoystickDirections || mode > CustomDirections || mode == m_directionMapping)
        return;
    m_directionMapping = mode;
    persistControllerMapping();
    emit controllerMappingChanged();
}

int ZxSettings::directionKey(int index) const
{
    return index >= 0 && index < ControllerInput::DirectionCount ? m_directionKeys[index] : 0;
}

QStringList ZxSettings::directionKeyNames() const
{
    QStringList result;
    for (const int key : m_directionKeys) {
        switch (key) {
        case Qt::Key_Shift:
            result.append(tr("Caps Shift"));
            break;
        case Qt::Key_Control:
            result.append(tr("Symbol Shift"));
            break;
        case Qt::Key_Enter:
        case Qt::Key_Return:
            result.append(tr("Enter"));
            break;
        case Qt::Key_Space:
            result.append(tr("Space"));
            break;
        default:
            result.append(QString(QChar(key)));
            break;
        }
    }
    return result;
}

void ZxSettings::setDirectionKey(int index, int key)
{
    if (index < 0 || index >= ControllerInput::DirectionCount
        || !ControllerInput::isSpectrumKey(key) || m_directionKeys[index] == key)
        return;
    m_directionKeys[index] = key;
    persistControllerMapping();
    emit controllerMappingChanged();
}

QVariantMap ZxSettings::controllerMapping()
{
    QVariantList keys;
    for (const int key : m_directionKeys)
        keys.append(key);
    QVariantMap actions;
    std::unique_lock lock(m_gamepadActionsMutex);
    for (const auto &[button, action] : m_gamepadActions)
        actions.insert(QString::number(button), action);
    return { { u"directionMode"_s, m_directionMapping },
             { u"directionKeys"_s, keys },
             { u"deadZone"_s, m_deadZone },
             { u"actions"_s, actions },
             { u"fastForwardToggle"_s, m_fastForwardToggle } };
}

void ZxSettings::persistControllerMapping()
{
    if (m_controllerOverride)
        return;
    const auto mapping = controllerMapping();
    QSettings settings;
    settings.setValue(u"Gamepad/directionMapping"_s, m_directionMapping);
    settings.setValue(u"Gamepad/directionKeys"_s, mapping.value(u"directionKeys"_s));
    settings.setValue(u"Gamepad/actionsMap"_s, mapping.value(u"actions"_s));
    settings.setValue(u"Gamepad/fastForwardToggle"_s, m_fastForwardToggle);
    settings.setValue(u"Peripherals/deadZone"_s, m_deadZone);
}

void ZxSettings::applyControllerMapping(const QVariantMap &mapping, bool temporary)
{
    // ControllerProfiles validates the entire mapping before any settings change.
    m_controllerOverride = temporary;
    m_directionMapping = mapping.value(u"directionMode"_s).toInt();
    const auto keys = mapping.value(u"directionKeys"_s).toList();
    for (int i = 0; i < ControllerInput::DirectionCount; ++i)
        m_directionKeys[i] = keys[i].toInt();
    m_deadZone = mapping.value(u"deadZone"_s).toDouble();
    m_fastForwardToggle = mapping.value(u"fastForwardToggle"_s).toBool();
    {
        std::unique_lock lock(m_gamepadActionsMutex);
        m_gamepadActions.clear();
        const auto actions = mapping.value(u"actions"_s).toMap();
        for (auto it = actions.cbegin(); it != actions.cend(); ++it)
            m_gamepadActions[it.key().toInt()] = it.value().toInt();
    }
    persistControllerMapping();
    emit controllerMappingChanged();
    emit deadZoneChanged(m_deadZone);
}

void ZxSettings::setFastForwardToggle(bool toggle)
{
    if (m_fastForwardToggle == toggle)
        return;
    m_fastForwardToggle = toggle;
    persistControllerMapping();
    emit controllerMappingChanged();
}

void ZxSettings::setControlOpacity(qreal &member, qreal value, const QString &key)
{
    if (!std::isfinite(value))
        return;
    value = std::clamp(value, 0.1, 1.0);
    if (member == value)
        return;
    member = value;
    QSettings().setValue(u"Controls/"_s + key, value);
    emit controlsAppearanceChanged();
}
void ZxSettings::setKeyboardOpacity(qreal value)
{
    setControlOpacity(m_keyboardOpacity, value, u"keyboardOpacity"_s);
}
void ZxSettings::setJoystickOpacity(qreal value)
{
    setControlOpacity(m_joystickOpacity, value, u"joystickOpacity"_s);
}
void ZxSettings::setButtonsOpacity(qreal value)
{
    setControlOpacity(m_buttonsOpacity, value, u"buttonsOpacity"_s);
}
