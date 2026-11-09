/*
    Copyright (c) 2015-2026, BogDan Vatra <bogdan@kde.org>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

import QtQuick
import QtGamepadLegacy
import Spectacol

// @scope main.qml
Sheet {
    id: page
    crumb: qsTr("Options")
    title: qsTr("Borders")
    readonly property var settings: ZxEmulator.settings
    readonly property var limits: settings.borderLimits
    hints: [
        { pad: "A", key: "← →", text: qsTr("Change") },
        { pad: "B", key: "Esc", text: qsTr("Back"), action: () => pageLoader.source = "" }
    ]
    GamepadNavigation {
        gamepad: Gamepad { deviceId: ZxEmulator.gamepadId }
        buttonAKey: Qt.Key_Return
        buttonBKey: Qt.Key_Escape
    }

    SettingsList {
        SettingRow {
            label: qsTr("Top")
            description: qsTr("Visible lines above the picture")
            kind: "stepper"
            stepSize: 4
            from: 0; to: page.limits.top
            value: Math.min(page.settings.borderTop, to)
            onMoved: (v) => page.settings.setBorderMargins(page.settings.borderLeft, v, page.settings.borderRight, page.settings.borderBottom)
        }
        SettingRow {
            label: qsTr("Left")
            kind: "stepper"
            stepSize: 4
            from: 0; to: page.limits.left
            value: Math.min(page.settings.borderLeft, to)
            onMoved: (v) => page.settings.setBorderMargins(v, page.settings.borderTop, page.settings.borderRight, page.settings.borderBottom)
        }
        SettingRow {
            label: qsTr("Bottom")
            kind: "stepper"
            stepSize: 4
            from: 0; to: page.limits.bottom
            value: Math.min(page.settings.borderBottom, to)
            onMoved: (v) => page.settings.setBorderMargins(page.settings.borderLeft, page.settings.borderTop, page.settings.borderRight, v)
        }
        SettingRow {
            label: qsTr("Right")
            description: qsTr("Horizontal sizes use standard Spectrum pixels")
            kind: "stepper"
            stepSize: 4
            from: 0; to: page.limits.right
            value: Math.min(page.settings.borderRight, to)
            onMoved: (v) => page.settings.setBorderMargins(page.settings.borderLeft, page.settings.borderTop, v, page.settings.borderBottom)
        }
        SectionTitle { text: qsTr("Presets") }
        SettingRow {
            label: qsTr("Wide")
            description: qsTr("Full side borders, no top or bottom border")
            kind: "action"; actionText: qsTr("Apply")
            onClicked: page.settings.setBorderMargins(page.limits.left, 0, page.limits.right, 0)
        }
        SettingRow {
            label: qsTr("Full borders")
            kind: "action"; actionText: qsTr("Apply")
            onClicked: page.settings.setBorderMargins(page.limits.left, page.limits.top, page.limits.right, page.limits.bottom)
        }
        SettingRow {
            label: qsTr("No borders")
            kind: "action"; actionText: qsTr("Apply")
            onClicked: page.settings.setBorderMargins(0, 0, 0, 0)
        }
        SettingRow {
            label: qsTr("Default borders")
            kind: "action"; actionText: qsTr("Apply")
            onClicked: page.settings.setBorderMargins(32, 32, 32, 32)
        }
    }
}
