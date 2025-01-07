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

import QtQuick
import QtQuick.Window
import QtGamepadLegacy
import QtQuick.Controls
import Spectacol

// @scope main.qml

Flickable {
    contentHeight: pane.height
    Component.onCompleted: FuseEmulator.paused = true
    Component.onDestruction: FuseEmulator.paused = false
    anchors.margins: Screen.pixelDensity

    GamepadKeyNavigation {
        gamepad: Gamepad { deviceId: FuseEmulator.gamepadId }
        buttonBKey: Qt.Key_Escape
        buttonAKey: Qt.Key_Space
    }

    FancyPane {
        id: pane
        Column {
            spacing: 2.5 * Screen.pixelDensity
            anchors.horizontalCenter: parent.horizontalCenter
            Label {
                text: qsTr("Peripherals Options")
            }

            CheckBox {
                id: kempstonJoystick
                KeyNavigation.up: deadZone
                KeyNavigation.down: kempstonMouse
                focus: true
                text: qsTr("Kempston joystick")
                checked: FuseEmulator.settings.kempstonJoystick
                onCheckedChanged: FuseEmulator.settings.kempstonJoystick = checked
            }

            CheckBox {
                id: kempstonMouse
                KeyNavigation.up: kempstonJoystick
                KeyNavigation.down: interface1
                text: qsTr("Kempston mouse")
                checked: FuseEmulator.settings.kempstonMouse
                onCheckedChanged: FuseEmulator.settings.kempstonMouse = checked
            }

            CheckBox {
                id: interface1
                KeyNavigation.up: kempstonMouse
                KeyNavigation.down: interface2
                text: qsTr("Interface 1")
                checked: FuseEmulator.settings.interface1
                onCheckedChanged: FuseEmulator.settings.interface1 = checked
            }

            CheckBox {
                id: interface2
                KeyNavigation.up: interface1
                KeyNavigation.down: full48kOSK
                text: qsTr("Interface 2")
                checked: FuseEmulator.settings.interface2
                onCheckedChanged: FuseEmulator.settings.interface2 = checked
            }

            CheckBox {
                id: full48kOSK
                KeyNavigation.up: interface2
                KeyNavigation.down: recreatedSpectrum
                text: qsTr("Full On Screen 48k Keyboard")
                checked: FuseEmulator.settings.full48kOSK
                onCheckedChanged: FuseEmulator.settings.full48kOSK = checked
            }

            CheckBox {
                id: recreatedSpectrum
                KeyNavigation.up: full48kOSK
                KeyNavigation.down: joystickPrompt
                text: qsTr("Recreated spectrum keyboard")
                checked: FuseEmulator.settings.recreatedSpectrum
                onCheckedChanged: FuseEmulator.settings.recreatedSpectrum = checked
            }

            CheckBox {
                id: joystickPrompt
                KeyNavigation.up: recreatedSpectrum
                KeyNavigation.down: deadZone
                text: qsTr("Snapshot joystick prompt")
                checked: FuseEmulator.settings.joystickPrompt
                onCheckedChanged: FuseEmulator.settings.joystickPrompt = checked
            }
            Row {
                Label {
                    text: "Joystick deadzone"
                }
                Slider {
                    id: deadZone
                    KeyNavigation.up: joystickPrompt
                    KeyNavigation.down: kempstonJoystick
                    from: 0
                    to: 0.5
                    stepSize: 0.05
                    value: FuseEmulator.settings.deadZone
                    onValueChanged: FuseEmulator.settings.deadZone = value
                }
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Close (B)")
                onClicked: pageLoader.source = ""
            }
        }
    }
}
