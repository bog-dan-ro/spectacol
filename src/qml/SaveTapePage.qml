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
import QtQuick.Dialogs
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
    readonly property bool tapSupported: ZxEmulator.tape.canSaveTap()
    readonly property string suffix: formatChoice.currentText
    function exportName() { return fileName.text.replace(/\.(tap|tzx)$/i, "") + "." + suffix; }

    function writeTape(path) {
        if (ZxEmulator.tape.write(path || ZxEmulator.tapeFilePath(page.exportName())))
            pageLoader.source = "";
    }
    FileDialog {
        id: deviceFile
        title: qsTr("Export tape to device storage")
        fileMode: FileDialog.SaveFile
        defaultSuffix: page.suffix
        currentFile: "file://" + ZxEmulator.tapeFilePath(page.exportName())
        onAccepted: page.writeTape(selectedFile.toString())
    }

    GamepadNavigation {
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
                  ? qsTr("TZX preserves custom loading signals and tape controls. TAP exports standard ROM data, including BASIC SAVE programs.")
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
            text: ZxEmulator.tapeFilePath(page.exportName())
            wrapMode: Text.WrapAnywhere
            font.pixelSize: Theme.caption
            color: Theme.textMuted
        }
        ComboBox {
            id: formatChoice
            objectName: "exportFormat"
            model: page.tapSupported ? ["tap", "tzx"] : ["tzx"]
            Accessible.name: qsTr("Tape export format")
        }
        Button {
            text: qsTr("Export .%1").arg(page.suffix)
            enabled: ZxEmulator.tape.hasTape
            onClicked: page.writeTape()
            Keys.onReturnPressed: page.writeTape()
        }
        Button {
            visible: Qt.platform.os === "android"
            text: qsTr("Export to device…")
            enabled: ZxEmulator.tape.hasTape
            onClicked: deviceFile.open()
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
