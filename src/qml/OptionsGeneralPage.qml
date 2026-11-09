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
    title: qsTr("General")
    hints: [
        { pad: "A", key: "Enter", text: qsTr("Change") },
        { pad: "B", key: "Esc", text: qsTr("Back"), action: () => pageLoader.source = "" }
    ]

    GamepadNavigation {
        gamepad: Gamepad { deviceId: ZxEmulator.gamepadId }
        buttonAKey: Qt.Key_Return
        buttonBKey: Qt.Key_Escape
    }

    SettingsList {
        SettingRow {
            label: qsTr("Emulation speed")
            kind: "stepper"
            from: 10
            to: 1000
            stepSize: 10
            value: ZxEmulator.settings.emulationSpeed
            valueText: (v) => qsTr("%1%").arg(v)
            onMoved: (v) => ZxEmulator.settings.emulationSpeed = v
        }
        SettingRow {
            label: qsTr("Save a snapshot when quitting")
            checked: ZxEmulator.settings.autoSaveOnExit
            onToggled: (on) => ZxEmulator.settings.autoSaveOnExit = on
        }
        SettingRow {
            label: qsTr("Browse only the Spectacol folder")
            checked: ZxEmulator.settings.restrictToSpectacol
            onToggled: (on) => ZxEmulator.settings.restrictToSpectacol = on
        }
        SettingRow {
            visible: ZxEmulator.touchscreen
            label: qsTr("Swipe to open the menu")
            checked: ZxEmulator.settings.swipe4menu
            onToggled: (on) => ZxEmulator.settings.swipe4menu = on
        }
        SettingRow {
            label: qsTr("Start tapes when opened")
            description: qsTr("Resets the machine and types LOAD \"\"")
            checked: ZxEmulator.settings.autoLoad
            onToggled: (on) => ZxEmulator.settings.autoLoad = on
        }
        SettingRow {
            label: qsTr("Detect loaders")
            description: qsTr("Plays and stops the tape when a game loads")
            checked: ZxEmulator.settings.detectLoaders
            onToggled: (on) => ZxEmulator.settings.detectLoaders = on
        }
        SettingRow {
            label: qsTr("Loader acceleration")
            description: qsTr("Turbo also runs flat out while the tape plays")
            kind: "choice"
            model: [qsTr("Off"), qsTr("Safe"), qsTr("Turbo")]
            currentIndex: ZxEmulator.settings.loaderAcceleration
            onActivated: (index) => ZxEmulator.settings.loaderAcceleration = index
        }
        SettingRow {
            label: qsTr("Messages")
            kind: "choice"
            model: [qsTr("All"), qsTr("Warnings"), qsTr("Errors")]
            currentIndex: ZxEmulator.settings.messageLevel
            onActivated: (index) => ZxEmulator.settings.messageLevel = index
        }

        SectionTitle { text: qsTr("Interface") }
        SettingRow {
            label: qsTr("Layout")
            description: qsTr("Desktop: a menu bar and docked panels. Overlay: for gamepads, touch and TVs")
            kind: "choice"
            model: [qsTr("Auto"), qsTr("Overlay"), qsTr("Desktop")]
            currentIndex: ZxEmulator.settings.uiMode
            onActivated: (index) => ZxEmulator.settings.uiMode = index
        }
        SettingRow {
            label: qsTr("Large text")
            description: qsTr("For a TV across the room")
            checked: ZxEmulator.settings.largeUi
            onToggled: (on) => ZxEmulator.settings.largeUi = on
        }
        SettingRow {
            label: qsTr("Reduce motion")
            checked: ZxEmulator.settings.reduceMotion
            onToggled: (on) => ZxEmulator.settings.reduceMotion = on
        }
    }
}
