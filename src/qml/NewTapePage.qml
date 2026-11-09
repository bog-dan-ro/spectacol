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
import QtQuick.Layouts
import QtQuick.Controls
import QtGamepadLegacy
import Spectacol

// @scope main.qml
Sheet {
    id: page
    crumb: qsTr("Media / Tape")
    title: qsTr("New blank tape")
    hints: [
        { pad: "A", key: "Enter", text: qsTr("Create"), action: () => page.createTape() },
        { pad: "B", key: "Esc", text: qsTr("Cancel"), action: () => pageLoader.source = "" }
    ]
    Component.onCompleted: ZxEmulator.tape.refreshData()
    function createTape() {
        ZxEmulator.tape.clear();
        pageLoader.source = "";
    }
    GamepadNavigation {
        gamepad: Gamepad { deviceId: ZxEmulator.gamepadId }
        buttonAKey: Qt.Key_Return
        buttonBKey: Qt.Key_Escape
    }
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.sp(6)
        spacing: Theme.sp(5)
        Text {
            Layout.fillWidth: true
            visible: ZxEmulator.tape.hasTape
            text: qsTr("This replaces the inserted tape. Export its blocks first if you want to keep them.")
            wrapMode: Text.Wrap
            font.pixelSize: Theme.body
            color: Theme.warning
        }
        Text {
            Layout.fillWidth: true
            text: qsTr("Your program stays in memory.\n\n1. Create the blank tape.\n2. In BASIC, enter SAVE \"name\", then press Enter at the tape prompt.\n3. Choose Media → Tape → Save to export a .tap file.")
            wrapMode: Text.Wrap
            font.pixelSize: Theme.body
            color: Theme.text
        }
        Button {
            focus: true
            text: qsTr("Create blank tape")
            onClicked: page.createTape()
            Keys.onReturnPressed: page.createTape()
        }
        Item { Layout.fillHeight: true }
    }
}
