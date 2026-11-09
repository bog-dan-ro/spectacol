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
    id: page
    crumb: qsTr("Options")
    hints: [
        { pad: "A", key: "Enter", text: qsTr("Change") },
        { pad: "B", key: "Esc", text: qsTr("Back"), action: () => pageLoader.source = "" }
    ]

    GamepadNavigation {
        gamepad: Gamepad { deviceId: ZxEmulator.gamepadId }
        buttonAKey: Qt.Key_Return
        buttonBKey: Qt.Key_Escape
    }
    title: qsTr("Sound")

    readonly property var stereoModes: ["None", "ACB", "ABC"]
    readonly property var speakerTypes: ["TV speaker", "Beeper", "Unfiltered"]
    readonly property var chipModels: ["AY8912", "YM2149"]

    SettingsList {
        SettingRow {
            label: qsTr("Sound")
            checked: ZxEmulator.settings.soundEnabled
            onToggled: (on) => ZxEmulator.settings.soundEnabled = on
        }
        SettingRow {
            label: qsTr("Loading sound")
            description: qsTr("Hear the tape while it loads")
            checked: ZxEmulator.settings.loadingSound
            onToggled: (on) => ZxEmulator.settings.loadingSound = on
        }
        SettingRow {
            label: qsTr("AY stereo")
            description: qsTr("Where the AY's three channels go: left, centre, right")
            kind: "choice"
            model: [qsTr("Mono"), "ACB", "ABC"]
            currentIndex: Math.max(0, page.stereoModes.indexOf(ZxEmulator.settings.AYStereoSeparation))
            onActivated: (index) => ZxEmulator.settings.AYStereoSeparation = page.stereoModes[index]
        }
        SettingRow {
            label: qsTr("Sound chip")
            description: qsTr("AY and YM chips have different volume curves and envelope steps")
            kind: "choice"
            model: ["AY-3-8912", "YM2149"]
            currentIndex: Math.max(0, page.chipModels.indexOf(ZxEmulator.settings.ayModel))
            onActivated: (index) => ZxEmulator.settings.ayModel = page.chipModels[index]
        }
        SettingRow {
            label: qsTr("Speaker")
            description: qsTr("TV shapes ULA and AY audio; Beeper shapes ULA audio; Unfiltered keeps the original response")
            kind: "choice"
            model: [qsTr("TV speaker"), qsTr("Beeper"), qsTr("Unfiltered")]
            currentIndex: Math.max(0, page.speakerTypes.indexOf(ZxEmulator.settings.speakerType))
            onActivated: (index) => ZxEmulator.settings.speakerType = page.speakerTypes[index]
        }
    }
}
