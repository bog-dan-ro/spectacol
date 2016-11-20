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

#ifndef FUSESETTINGS_H
#define FUSESETTINGS_H

#include "fuseobject.h"

#include <QGamepadManager>

#include <mutex>
#include <unordered_map>

class FuseSettings : public FuseObject
{
    Q_OBJECT
    Q_ENUMS(FillMode Actions)

    Q_PROPERTY(QStringList machinesModel READ machinesModel CONSTANT)
    Q_PROPERTY(QString currentMachine READ currentMachine NOTIFY currentMachineChanged)
    Q_PROPERTY(int currentMachineIndex READ currentMachineIndex WRITE setCurrentMachineIndex NOTIFY currentMachineChanged)
    Q_PROPERTY(bool hasStartButton READ hasStartButton NOTIFY hasStartButtonChanged)

    // General Options
    Q_PROPERTY(int emulationSpeed READ emulationSpeed WRITE setEmulationSpeed NOTIFY settingsCurrentChanged)
    Q_PROPERTY(bool autoLoad READ autoLoad WRITE setAutoLoad NOTIFY settingsCurrentChanged)
    Q_PROPERTY(bool detectLoaders READ detectLoaders WRITE setDetectLoaders NOTIFY settingsCurrentChanged)
    Q_PROPERTY(bool restrictToSpectacol READ restrictToSpectacol WRITE setRestrictToSpectacol NOTIFY settingsCurrentChanged)
    Q_PROPERTY(bool swipe4menu READ swipe4menu WRITE setSwipe4menu NOTIFY swipe4menuChanged)
    Q_PROPERTY(bool autoSaveOnExit READ autoSaveOnExit WRITE setAutoSaveOnExit NOTIFY autoSaveOnExitChanged)
    Q_PROPERTY(int loaderAcceleration READ loaderAcceleration WRITE setLoaderAcceleration NOTIFY settingsCurrentChanged)
    Q_PROPERTY(int messageLevel READ messageLevel WRITE setMessageLevel NOTIFY messageLevelChanged)

    // Screen Options
    Q_PROPERTY(bool showOrientationChooser READ showOrientationChooser CONSTANT)
    Q_PROPERTY(int screenOrientation READ screenOrientation WRITE setScreenOrientation NOTIFY screenOrientationChanged)
    Q_PROPERTY(FillMode fillMode READ fillMode WRITE setFillMode NOTIFY fillModeChanged)
    Q_PROPERTY(bool leftMargin READ leftMargin WRITE setLeftMargin NOTIFY leftMarginChanged)

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
    Q_PROPERTY(bool joystickPrompt READ joystickPrompt WRITE setJoystickPrompt NOTIFY settingsCurrentChanged)
    Q_PROPERTY(qreal deadZone READ deadZone WRITE setDeadZone NOTIFY deadZoneChanged)

public:
    enum FillMode {
        PreserveAspectFit = 0,
        PreserveAspect,
        Stretch
    };

    enum Actions {
        JoystickFire = 0,
        ToggleKeyboard = Qt::Key_unknown + 1,
        ToggleCursorJoystick,
        TogglePause,
        QuickSaveSnapshot,
        QuickLoadSnapshot,
        ToggleEmulationSpeed
    };

public:
    explicit FuseSettings(QObject *parent = 0);

    QStringList machinesModel() const;
    QString currentMachine();
    int currentMachineIndex() const;
    void setCurrentMachineIndex(int idx);

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

    bool hasStartButton() const;
    void setHasStartButton(bool hasStartButton);

    bool showOrientationChooser() const;

    int screenOrientation() const;
    void setScreenOrientation(int orientation);

    FillMode fillMode() const;
    void setFillMode(FillMode fill);

    bool swipe4menu() const;
    void setSwipe4menu(bool swipe4menu);

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

public slots:
    inline int gamepadAction(int gamepadButton) {
        std::unique_lock<std::mutex> lock(m_gamepadActionsMutex);
        auto it = m_gamepadActions.find(gamepadButton);
        if (it != m_gamepadActions.end())
            return it->second;
        return JoystickFire;
    }

    void setGamepadAction(int gamepadButton, int action);

private:
    enum ScreenOrientation {
        Sensors = 0,
        Landscape,
        Portrait
    };

signals:
    void currentMachineChanged();
    void settingsCurrentChanged();
    void hasStartButtonChanged(bool hasStartButton);
    void screenOrientationChanged(int currentOrientation);
    void fillModeChanged(FillMode fillMode);
    void swipe4menuChanged(bool swipe4menu);
    void autoSaveOnExitChanged(bool autoSaveOnExit);
    void leftMarginChanged(bool leftMargin);
    void deadZoneChanged(qreal deadZone);

    void messageLevelChanged(int messageLevel);

private:
    bool m_hasStartButton = false;
    std::mutex m_gamepadActionsMutex;

    // Default button actions map
    std::unordered_map<int, int> m_gamepadActions = {
        {QGamepadManager::ButtonX, ToggleKeyboard},
        {QGamepadManager::ButtonY, ToggleCursorJoystick},
        {QGamepadManager::ButtonL2, QuickSaveSnapshot},
        {QGamepadManager::ButtonR2, QuickLoadSnapshot}
    };
};

#endif // FUSESETTINGS_H
