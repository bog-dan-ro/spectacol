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

pragma ComponentBehavior: Bound

import QtQuick
import QtGamepadLegacy
import Spectacol

Sheet {
    id: page
    crumb: qsTr("Options / Controls")
    title: qsTr("On-screen controls and fast-forward")
    hints: [
        { pad: "↔", key: "Left / Right", text: qsTr("Change") },
        { pad: "B", key: "Esc", text: qsTr("Back"), action: () => page.closeRequested() }
    ]
    GamepadKeyNavigation {
        gamepad: Gamepad { deviceId: ZxEmulator.gamepadId }
        buttonAKey: Qt.Key_Return
        buttonBKey: Qt.Key_Escape
    }
    SettingsList {
        SettingRow {
            label: qsTr("Fast-forward")
            description: ZxEmulator.controllerProfiles.gameProfile.length
                ? qsTr("Profile: %1 — save mode changes in Controller profiles").arg(ZxEmulator.controllerProfiles.gameProfile)
                : qsTr("For the touch shortcut and controller buttons assigned to fast-forward")
            kind: "choice"
            model: [qsTr("Hold"), qsTr("Press to toggle")]
            currentIndex: ZxEmulator.settings.fastForwardToggle ? 1 : 0
            onActivated: index => ZxEmulator.settings.fastForwardToggle = index === 1
        }
        SettingRow {
            label: qsTr("Keyboard opacity")
            kind: "stepper"
            from: 10; to: 100; stepSize: 10
            value: ZxEmulator.settings.keyboardOpacity * 100
            valueText: value => qsTr("%1%").arg(Math.round(value))
            onMoved: value => ZxEmulator.settings.keyboardOpacity = value / 100
        }
        SettingRow {
            label: qsTr("Joystick opacity")
            kind: "stepper"
            from: 10; to: 100; stepSize: 10
            value: ZxEmulator.settings.joystickOpacity * 100
            valueText: value => qsTr("%1%").arg(Math.round(value))
            onMoved: value => ZxEmulator.settings.joystickOpacity = value / 100
        }
        SettingRow {
            label: qsTr("Action buttons opacity")
            kind: "stepper"
            from: 10; to: 100; stepSize: 10
            value: ZxEmulator.settings.buttonsOpacity * 100
            valueText: value => qsTr("%1%").arg(Math.round(value))
            onMoved: value => ZxEmulator.settings.buttonsOpacity = value / 100
        }
    }
}
