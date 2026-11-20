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
import QtGamepadLegacy
import Spectacol

// @scope main.qml

Sheet {
    crumb: qsTr("Options")
    hints: [
        { pad: "A", key: "Enter", text: qsTr("Change") },
        { pad: "B", key: "Esc", text: qsTr("Back"), action: () => mainScreen.closePage() }
    ]

    GamepadNavigation {
        gamepad: Gamepad { deviceId: ZxEmulator.gamepadId }
        buttonAKey: Qt.Key_Return
        buttonBKey: Qt.Key_Escape
    }
    title: qsTr("Peripherals")

    SettingsList {
        SettingRow {
            label: qsTr("Overlapped Virtual Keyboard")
            checked: ZxEmulator.settings.overlappedKeboard
            onToggled: (on) => ZxEmulator.settings.overlappedKeboard = on
        }
        SettingRow {
            label: qsTr("Kempston joystick")
            checked: ZxEmulator.settings.kempstonJoystick
            onToggled: (on) => ZxEmulator.settings.kempstonJoystick = on
        }
        SettingRow {
            label: qsTr("Kempston mouse")
            checked: ZxEmulator.settings.kempstonMouse
            onToggled: (on) => ZxEmulator.settings.kempstonMouse = on
        }
        SettingRow {
            label: qsTr("Interface 1")
            enabled: ZxEmulator.settings.interface1Available
            description: enabled ? qsTr("Microdrive cartridges")
                                 : qsTr("Unavailable on this machine")
            checked: ZxEmulator.settings.interface1
            onToggled: (on) => ZxEmulator.settings.interface1 = on
        }
        SettingRow {
            label: qsTr("Interface 2")
            description: qsTr("Two Sinclair joysticks and a cartridge slot")
            checked: ZxEmulator.settings.interface2
            onToggled: (on) => ZxEmulator.settings.interface2 = on
        }

        SettingRow {
            visible: ZxEmulator.settings.currentMachine === "ZX Spectrum Next"
            label: qsTr("Next UART devices")
            description: qsTr("Wi-Fi, Pi service and external scripts")
            kind: "action"
            onClicked: pageLoader.source="NextUartPage.qml"
        }
        SectionTitle { text: qsTr("Keyboard and gamepad") }
        SettingRow {
            label: qsTr("On-screen keyboard")
            kind: "choice"
            model: [qsTr("Short"), qsTr("48K"), qsTr("Spectrum+")]
            currentIndex: ZxEmulator.settings.keyboardLayout
            onActivated: (index) => ZxEmulator.settings.keyboardLayout = index
            description: (currentIndex === ZxSettings.ShortKeyboard ? qsTr("The 48K's keys, with only their letters.")
                          : currentIndex === ZxSettings.Full48Keyboard ? qsTr("The 48K's keys, with their keywords and symbols.")
                          : qsTr("The Spectrum+ keys: EDIT, DELETE, the cursor keys and more."))
                         + " " + qsTr("The ZX Spectrum Next always uses the Spectrum+ keys.")
        }
        SettingRow {
            label: qsTr("Recreated ZX Spectrum keyboard")
            description: qsTr("Its game mode, where each key sends two codes")
            checked: ZxEmulator.settings.recreatedSpectrum
            onToggled: (on) => ZxEmulator.settings.recreatedSpectrum = on
        }
        SettingRow {
            label: qsTr("Ask for the joystick")
            description: qsTr("When a snapshot has no single supported joystick type")
            checked: ZxEmulator.settings.joystickPrompt
            onToggled: (on) => ZxEmulator.settings.joystickPrompt = on
        }
        SettingRow {
            label: qsTr("Stick dead zone")
            kind: "stepper"
            from: 0
            to: 0.95
            stepSize: 0.05
            value: ZxEmulator.settings.deadZone
            valueText: (v) => qsTr("%1%").arg(Math.round(v * 100))
            onMoved: (v) => ZxEmulator.settings.deadZone = v
        }
    }
}
