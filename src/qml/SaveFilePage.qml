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
import QtQuick.Dialogs
import QtQuick.Layouts
import QtQuick.Controls
import QtGamepadLegacy
import Spectacol

// @scope main.qml
Sheet {
    id: page
    crumb: qsTr("Media")
    property alias fileName: fileNameEdit.text
    property alias folder: pathEdit.text
    property string destinationPath
    property list<string> formats: ["szx"]
    readonly property string selectedFormat: formatChoice.currentText
    signal accepted(string path)
    function save() { accepted(destinationPath); }
    signal canceled
    onCloseRequested: canceled()
    hints: [
        { pad: "A", key: "Enter", text: qsTr("Save"), action: page.save },
        { pad: "B", key: "Esc", text: qsTr("Cancel"), action: () => page.canceled() }
    ]
    GamepadNavigation {
        gamepad: Gamepad { deviceId: ZxEmulator.gamepadId }
        buttonAKey: Qt.Key_Return
        buttonBKey: Qt.Key_Escape
    }
    FolderDialog {
        id: browseFolder
        currentFolder: "file://" + pathEdit.text
        onAccepted: pathEdit.text = decodeURIComponent(selectedFolder.toString().replace(/^file:\/\//, ""))
    }
    FileDialog {
        id: deviceFile
        title: page.title
        fileMode: FileDialog.SaveFile
        defaultSuffix: page.selectedFormat
        currentFile: "file://" + page.destinationPath
        onAccepted: page.accepted(selectedFile.toString())
    }
    // Scrolls: a phone in landscape, or one with its keyboard up, is shorter than the form
    Flickable {
        id: scroller
        anchors.fill: parent
        contentHeight: form.implicitHeight + 2 * Theme.sp(Theme.touchScreenUi ? 3 : 6)
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        ScrollIndicator.vertical: ScrollIndicator {}
    ColumnLayout {
        id: form
        x: Theme.sp(Theme.touchScreenUi ? 3 : 6)
        y: x
        width: scroller.width - 2 * x
        spacing: Theme.sp(Theme.touchScreenUi ? 2 : 4)
        Text {
            text: qsTr("File name (.%1)").arg(page.selectedFormat)
            font.pixelSize: Theme.body
            color: Theme.text
        }
        FancyTextField {
            id: fileNameEdit
            Layout.fillWidth: true
            focus: true
            selectByMouse: true
            onAccepted: page.save()
        }
        Text {
            text: Qt.platform.os === "android" ? qsTr("Spectacol folder") : qsTr("Folder")
            font.pixelSize: Theme.body
            color: Theme.text
        }
        RowLayout {
            Layout.fillWidth: true
            FancyTextField {
                id: pathEdit
                Layout.fillWidth: true
                selectByMouse: true
                onAccepted: page.save()
            }
            Button {
                text: Qt.platform.os === "android" ? qsTr("Export to device…") : qsTr("Browse")
                onClicked: Qt.platform.os === "android" ? deviceFile.open() : browseFolder.open()
            }
        }
        Text {
            Layout.fillWidth: true
            visible: text !== ""
            text: page.destinationPath
            wrapMode: Text.WrapAnywhere
            font.pixelSize: Theme.caption
            color: Theme.textMuted
        }
        RowLayout {
            Layout.fillWidth: true
            Text {
                text: qsTr("Format")
                font.pixelSize: Theme.body
                color: Theme.text
            }
            ComboBox {
                id: formatChoice
                objectName: "exportFormat"
                model: page.formats
                onActivated: {
                    page.fileName = page.fileName.replace(/\.(szx|sna|z80|nex)$/i, "") + "." + currentText;
                }
            }
        }
        Text {
            Layout.fillWidth: true
            visible: page.selectedFormat === "nex"
            text: qsTr("NEX exports a Next program and its RAM. It does not preserve all CPU registers or device state.")
            wrapMode: Text.Wrap
            font.pixelSize: Theme.caption
            color: Theme.textMuted
        }
        RowLayout {
            Button {
                text: qsTr("Save")
                onClicked: page.save()
                Keys.onReturnPressed: page.save()
            }
            Button {
                text: qsTr("Cancel")
                onClicked: page.canceled()
                Keys.onReturnPressed: page.canceled()
            }
        }
    }
    }
}
