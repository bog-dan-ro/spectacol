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

import QtQuick
import QtGamepadLegacy
import Spectacol

// @scope main.qml
Sheet {
    id: page
    crumb: qsTr("Options")
    title: qsTr("Screen")
    readonly property var settings: ZxEmulator.settings
    hints: [
        { pad: "A", key: "Enter", text: qsTr("Change") },
        { pad: "B", key: "Esc", text: qsTr("Back"), action: () => pageLoader.source = "" }
    ]

    GamepadNavigation {
        gamepad: Gamepad { deviceId: ZxEmulator.gamepadId }
        buttonAKey: Qt.Key_Return
        buttonBKey: Qt.Key_Escape
        buttonXKey: Qt.Key_Backtab
        buttonYKey: Qt.Key_Tab
    }

    SettingsList {
        SettingRow {
            objectName: "screenFilter"
            label: qsTr("Screen filter")
            description: qsTr("Use Left and Right to preview each filter")
            kind: "choice"
            model: ZxEmulator.filtersModel
            choiceMaximumWidth: Math.max(Theme.sp(18), page.panelWidth * 0.45)
            currentIndex: ZxEmulator.selectedFilterIndex
            onActivated: index => ZxEmulator.selectedFilterIndex = index
        }
        SettingRow {
            objectName: "screenFillMode"
            label: qsTr("Fill mode")
            description: qsTr("Fit keeps the aspect ratio; Integer uses whole-pixel scaling")
            kind: "choice"
            model: [qsTr("Fit"), qsTr("Integer"), qsTr("Stretch")]
            currentIndex: page.settings.fillMode
            onActivated: index => page.settings.fillMode = index
        }
        SettingRow {
            objectName: "screenOrientation"
            visible: page.settings.showOrientationChooser
            label: qsTr("Screen orientation")
            kind: "choice"
            model: [qsTr("Any"), qsTr("Landscape"), qsTr("Portrait")]
            currentIndex: page.settings.screenOrientation
            onActivated: index => page.settings.screenOrientation = index
        }
        SettingRow {
            objectName: "smoothScaling"
            label: qsTr("Smooth scaling")
            checked: page.settings.smoothScaling
            onToggled: checked => page.settings.smoothScaling = checked
        }
        SettingRow {
            objectName: "leftMargin"
            visible: Theme.smallScreen
            label: qsTr("Reserve space on the left")
            description: qsTr("Leave room for on-screen controls in landscape")
            checked: page.settings.leftMargin
            onToggled: checked => page.settings.leftMargin = checked
        }
    }
}
