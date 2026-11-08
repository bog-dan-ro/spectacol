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

#include <QObject>

#include <QGamepadManager>
#include "controllerinput.h"
#include <QQmlEngine>
#include <QVariantMap>
#include <zxkit/video/VideoFrame.h>

#include <mutex>
#include <string>
#include <unordered_map>

class ZxEmulator;

class ZxSettings : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("use ZxEmulator.settings instead")

    Q_PROPERTY(QStringList machinesModel READ machinesModel CONSTANT)
    Q_PROPERTY(QString currentMachine READ currentMachine NOTIFY currentMachineChanged)
    Q_PROPERTY(int currentMachineIndex READ currentMachineIndex WRITE setCurrentMachineIndex NOTIFY
                       currentMachineChanged)
    Q_PROPERTY(bool hasStartButton READ hasStartButton NOTIFY hasStartButtonChanged)

    // General Options
    Q_PROPERTY(int emulationSpeed READ emulationSpeed WRITE setEmulationSpeed NOTIFY
                       settingsCurrentChanged)
    Q_PROPERTY(bool autoLoad READ autoLoad WRITE setAutoLoad NOTIFY settingsCurrentChanged)
    Q_PROPERTY(bool detectLoaders READ detectLoaders WRITE setDetectLoaders NOTIFY
                       settingsCurrentChanged)
    Q_PROPERTY(bool restrictToSpectacol READ restrictToSpectacol WRITE setRestrictToSpectacol NOTIFY
                       settingsCurrentChanged)
    Q_PROPERTY(bool swipe4menu READ swipe4menu WRITE setSwipe4menu NOTIFY swipe4menuChanged)
    // The UI: larger for a TV, without animations, and the overlay (gamepad, touch) or desktop layout
    Q_PROPERTY(bool largeUi READ largeUi WRITE setLargeUi NOTIFY uiChanged)
    Q_PROPERTY(bool reduceMotion READ reduceMotion WRITE setReduceMotion NOTIFY uiChanged)
    Q_PROPERTY(UiMode uiMode READ uiMode WRITE setUiMode NOTIFY uiChanged)
    Q_PROPERTY(bool autoSaveOnExit READ autoSaveOnExit WRITE setAutoSaveOnExit NOTIFY
                       autoSaveOnExitChanged)
    Q_PROPERTY(int loaderAcceleration READ loaderAcceleration WRITE setLoaderAcceleration NOTIFY
                       settingsCurrentChanged)
    Q_PROPERTY(int messageLevel READ messageLevel WRITE setMessageLevel NOTIFY messageLevelChanged)

    // Screen Options
    Q_PROPERTY(bool showOrientationChooser READ showOrientationChooser CONSTANT)
    Q_PROPERTY(int screenOrientation READ screenOrientation WRITE setScreenOrientation NOTIFY
                       screenOrientationChanged)
    Q_PROPERTY(FillMode fillMode READ fillMode WRITE setFillMode NOTIFY fillModeChanged)
    Q_PROPERTY(bool smoothScaling READ smoothScaling WRITE setSmoothScaling NOTIFY
                       smoothScalingChanged)
    Q_PROPERTY(bool leftMargin READ leftMargin WRITE setLeftMargin NOTIFY leftMarginChanged)
    Q_PROPERTY(int borderLeft READ borderLeft NOTIFY borderMarginsChanged)
    Q_PROPERTY(int borderTop READ borderTop NOTIFY borderMarginsChanged)
    Q_PROPERTY(int borderRight READ borderRight NOTIFY borderMarginsChanged)
    Q_PROPERTY(int borderBottom READ borderBottom NOTIFY borderMarginsChanged)
    Q_PROPERTY(QVariantMap borderLimits READ borderLimits NOTIFY borderLimitsChanged)

    // Sound Options
    Q_PROPERTY(
            bool soundEnabled READ soundEnabled WRITE setSoundEnabled NOTIFY settingsCurrentChanged)
    Q_PROPERTY(
            bool loadingSound READ loadingSound WRITE setLoadingSound NOTIFY settingsCurrentChanged)
    Q_PROPERTY(QString AYStereoSeparation READ AYStereoSeparation WRITE setAYStereoSeparation NOTIFY
                       settingsCurrentChanged)
    Q_PROPERTY(QString ayModel READ ayModel WRITE setAyModel NOTIFY settingsCurrentChanged)
    Q_PROPERTY(
            QString speakerType READ speakerType WRITE setSpeakerType NOTIFY settingsCurrentChanged)

    Q_PROPERTY(bool fastForwardToggle READ fastForwardToggle WRITE setFastForwardToggle NOTIFY
                       controllerMappingChanged FINAL)
    Q_PROPERTY(qreal keyboardOpacity READ keyboardOpacity WRITE setKeyboardOpacity NOTIFY
                       controlsAppearanceChanged FINAL)
    Q_PROPERTY(qreal joystickOpacity READ joystickOpacity WRITE setJoystickOpacity NOTIFY
                       controlsAppearanceChanged FINAL)
    Q_PROPERTY(qreal buttonsOpacity READ buttonsOpacity WRITE setButtonsOpacity NOTIFY
                       controlsAppearanceChanged FINAL)

    // Peripherals Options
    Q_PROPERTY(bool kempstonJoystick READ kempstonJoystick WRITE setKempstonJoystick NOTIFY
                       settingsCurrentChanged)
    Q_PROPERTY(bool kempstonMouse READ kempstonMouse WRITE setKempstonMouse NOTIFY
                       settingsCurrentChanged)
    Q_PROPERTY(bool interface1 READ interface1 WRITE setInterface1 NOTIFY settingsCurrentChanged)
    Q_PROPERTY(bool interface2 READ interface2 WRITE setInterface2 NOTIFY settingsCurrentChanged)
    Q_PROPERTY(bool full48kOSK READ full48kOSK WRITE setFull48kOSK NOTIFY settingsCurrentChanged)
    Q_PROPERTY(bool joystickPrompt READ joystickPrompt WRITE setJoystickPrompt NOTIFY
                       settingsCurrentChanged)
    Q_PROPERTY(bool recreatedSpectrum READ recreatedSpectrum WRITE setRecreatedSpectrum NOTIFY
                       settingsCurrentChanged)
    Q_PROPERTY(qreal deadZone READ deadZone WRITE setDeadZone NOTIFY deadZoneChanged)
    Q_PROPERTY(int directionMapping READ directionMapping WRITE setDirectionMapping NOTIFY
                       controllerMappingChanged)
    Q_PROPERTY(QStringList directionKeyNames READ directionKeyNames NOTIFY controllerMappingChanged)

public:
    enum DirectionMapping {
        JoystickDirections = ControllerInput::Joystick,
        QaopDirections = ControllerInput::Qaop,
        CustomDirections = ControllerInput::Custom
    };
    Q_ENUM(DirectionMapping)
    bool fastForwardToggle() const
    {
        return m_fastForwardToggle;
    }
    void setFastForwardToggle(bool toggle);
    qreal keyboardOpacity() const
    {
        return m_keyboardOpacity;
    }
    qreal joystickOpacity() const
    {
        return m_joystickOpacity;
    }
    qreal buttonsOpacity() const
    {
        return m_buttonsOpacity;
    }
    void setKeyboardOpacity(qreal value);
    void setJoystickOpacity(qreal value);
    void setButtonsOpacity(qreal value);
    QVariantMap controllerMapping();
    void applyControllerMapping(const QVariantMap &mapping, bool temporary);
    bool controllerOverride() const
    {
        return m_controllerOverride;
    }
    int directionMapping() const
    {
        return m_directionMapping;
    }
    void setDirectionMapping(int mode);
    QStringList directionKeyNames() const;
    const std::array<int, ControllerInput::DirectionCount> &directionKeys() const
    {
        return m_directionKeys;
    }
    Q_INVOKABLE int directionKey(int index) const;
    Q_INVOKABLE void setDirectionKey(int index, int key);
    int borderLeft() const
    {
        return m_borderMargins.left;
    }
    int borderTop() const
    {
        return m_borderMargins.top;
    }
    int borderRight() const
    {
        return m_borderMargins.right;
    }
    int borderBottom() const
    {
        return m_borderMargins.bottom;
    }
    QVariantMap borderLimits() const;
    Q_INVOKABLE void setBorderMargins(int left, int top, int right, int bottom);
    enum UiMode {
        AutoUi, // the desktop layout without a gamepad or a touch screen, else the overlay
        OverlayUi,
        DesktopUi
    };
    Q_ENUM(UiMode)

    enum FillMode { PreserveAspectFit = 0, PreserveAspect, Stretch };
    Q_ENUM(FillMode)

    enum Actions {
        JoystickFire = int(ControllerAction::Fire),
        ToggleKeyboard = int(ControllerAction::Keyboard),
        ToggleCursorJoystick,
        TogglePause,
        QuickSaveSnapshot,
        QuickLoadSnapshot,
        ToggleEmulationSpeed
    };
    Q_ENUM(Actions)

public:
    explicit ZxSettings(ZxEmulator &emulator);

    /// Applies the saved settings (machine, peripherals, sound, speed) to the emulator.
    void applyToEmulator();

    QStringList machinesModel() const;
    QString currentMachine();
    int currentMachineIndex() const;
    void setCurrentMachineIndex(int idx);
    /// Selects a machine by its ZxKit id (e.g. "next"); false if there is no such machine.
    bool selectMachine(const QString &machineId);
    QStringList machineIds() const;
    void syncCurrentMachine(std::string_view machineId);

    int emulationSpeed() const;
    void setEmulationSpeed(int speed);

    bool autoLoad() const;
    void setAutoLoad(bool autoLoad);

    bool detectLoaders() const;
    void setDetectLoaders(bool detectLoaders);

    bool soundEnabled() const;
    void setSoundEnabled(bool soundEnabled);

    bool loadingSound() const;
    void setLoadingSound(bool loadingSound);

    QString AYStereoSeparation() const;
    void setAYStereoSeparation(const QString &AYStereoSeparation);

    QString ayModel() const
    {
        return m_ayModel;
    }
    void setAyModel(const QString &model);

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

    bool hasStartButton() const;
    void setHasStartButton(bool hasStartButton);

    bool showOrientationChooser() const;

    int screenOrientation() const;
    void setScreenOrientation(int orientation);

    FillMode fillMode() const;
    void setFillMode(FillMode fill);

    bool smoothScaling() const;
    void setSmoothScaling(bool smoothScaling);

    bool swipe4menu() const;
    void setSwipe4menu(bool swipe4menu);

    bool largeUi() const;
    void setLargeUi(bool large);
    bool reduceMotion() const;
    void setReduceMotion(bool reduce);
    UiMode uiMode() const;
    void setUiMode(UiMode mode);

    bool autoSaveOnExit() const;
    void setAutoSaveOnExit(bool autoSave);

    bool leftMargin() const;
    void setLeftMargin(bool leftMargin);

    bool joystickPrompt() const;
    void setJoystickPrompt(bool joystickPrompt);

    int loaderAcceleration() const;
    void setLoaderAcceleration(int loaderAcceleration);

    qreal deadZone() const;
    void setDeadZone(qreal dz);

    int messageLevel() const;
    void setMessageLevel(int level);

    bool recreatedSpectrum() const;
    void setRecreatedSpectrum(bool recreatedSpectrum);

public slots:
    inline int gamepadAction(int gamepadButton)
    {
        std::unique_lock<std::mutex> lock(m_gamepadActionsMutex);
        auto it = m_gamepadActions.find(gamepadButton);
        if (it != m_gamepadActions.end())
            return it->second;
        return JoystickFire;
    }

    void setGamepadAction(int gamepadButton, int action);

private:
    void persistControllerMapping();
    void setControlOpacity(qreal &member, qreal value, const QString &key);
    bool m_fastForwardToggle = false;
    bool m_controllerOverride = false;
    qreal m_deadZone = 0.4;
    qreal m_keyboardOpacity = 1;
    qreal m_joystickOpacity = 1;
    qreal m_buttonsOpacity = 1;
    int m_directionMapping = JoystickDirections;
    std::array<int, ControllerInput::DirectionCount> m_directionKeys = ControllerInput::qaopKeys;
    /// `persist`: remember the machine for the next start (the user picked it)
    void applyMachine(int idx, bool persist);
    void applyTapeOptions();

    enum ScreenOrientation { Sensors = 0, Landscape, Portrait };

signals:
    void borderLimitsChanged();
    void borderMarginsChanged();
    void currentMachineChanged();
    void settingsCurrentChanged();
    void hasStartButtonChanged(bool hasStartButton);
    void screenOrientationChanged(int currentOrientation);
    void fillModeChanged(ZxSettings::FillMode fillMode);
    void smoothScalingChanged(bool smoothScaling);
    void swipe4menuChanged(bool swipe4menu);
    void uiChanged();
    void autoSaveOnExitChanged(bool autoSaveOnExit);
    void leftMarginChanged(bool leftMargin);
    void deadZoneChanged(qreal deadZone);
    void controllerMappingChanged();
    void controlsAppearanceChanged();

    void messageLevelChanged(int messageLevel);

private:
    ZxEmulator &m_zxEmulator;
    zxkit::BorderMargins m_borderMargins;
    bool m_hasStartButton = false;
    int m_currentMachineIndex = 1; // Default to 48k
    std::string m_savedMachineId = "48k";
    int m_pendingMachineIndex = -1; // the Next, waiting for its SD card image
    bool m_pendingMachinePersist = false;
    int m_emulationSpeed = 100;
    bool m_autoLoad = true;
    bool m_detectLoaders = true;
    bool m_soundEnabled = true;
    bool m_loadingSound = true;
    QString m_ayStereo;
    QString m_ayModel;
    QString m_speakerType;
    bool m_kempstonJoystick = true;
    bool m_kempstonMouse = false;
    bool m_interface1 = false;
    bool m_interface2 = true;
    bool m_joystickPrompt = true;
    int m_loaderAcceleration = 1;
    bool m_recreatedSpectrum = false;
    std::vector<std::string> m_machineIds;
    QStringList m_machineNames;
    std::mutex m_gamepadActionsMutex;

    // Default button actions map
    std::unordered_map<int, int> m_gamepadActions = {
        { QGamepadManager::ButtonX, ToggleKeyboard },
        { QGamepadManager::ButtonY, ToggleCursorJoystick },
        { QGamepadManager::ButtonL2, QuickSaveSnapshot },
        { QGamepadManager::ButtonR2, QuickLoadSnapshot }
    };
};
