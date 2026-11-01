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
import QtQuick.Layouts
import QtQuick.Controls
import QtGamepadLegacy
import Spectacol

// @scope main.qml
Sheet {
    id: page
    crumb: qsTr("Media / Tape")
    title: qsTr("Export tape")
    hints: [
        { pad: "A", key: "Enter", text: qsTr("Export"), action: () => page.writeTape() },
        { pad: "B", key: "Esc", text: qsTr("Back"), action: () => pageLoader.source = "" }
    ]
    Component.onCompleted: ZxEmulator.tape.refreshData()

    function writeTape() {
        if (ZxEmulator.tape.write(ZxEmulator.tapeFilePath(fileName.text)))
            pageLoader.source = "";
    }

    GamepadKeyNavigation {
        gamepad: Gamepad { deviceId: ZxEmulator.gamepadId }
        buttonAKey: Qt.Key_Return
        buttonBKey: Qt.Key_Escape
    }
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.sp(6)
        spacing: Theme.sp(4)
        Text {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            text: ZxEmulator.tape.hasTape
                  ? qsTr("Exports the inserted tape, including blocks written by BASIC SAVE, as a .tap file.")
                  : qsTr("The tape is empty. Close this page, enter SAVE \"name\" in BASIC and press Enter at the tape prompt. Then return here to export it.")
            font.pixelSize: Theme.body
            color: Theme.text
        }
        FancyTextField {
            id: fileName
            Layout.fillWidth: true
            focus: true
            selectByMouse: true
            placeholderText: qsTr("Tape name.tap")
            text: ZxEmulator.tape.suggestedFileName
            font.pixelSize: Theme.body
            onAccepted: page.writeTape()
        }
        Text {
            Layout.fillWidth: true
            text: ZxEmulator.tapeFilePath(fileName.text)
            wrapMode: Text.WrapAnywhere
            font.pixelSize: Theme.caption
            color: Theme.textMuted
        }
        Button {
            text: qsTr("Export .tap")
            enabled: ZxEmulator.tape.hasTape
            onClicked: page.writeTape()
            Keys.onReturnPressed: page.writeTape()
        }
        Text {
            Layout.fillWidth: true
            text: qsTr("Input recording (.rzx) records a play session. Use tape export to share BASIC programs with another emulator or a real Spectrum.")
            wrapMode: Text.Wrap
            font.pixelSize: Theme.caption
            color: Theme.textMuted
        }
        Item { Layout.fillHeight: true }
    }
}
