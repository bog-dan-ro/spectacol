/*
    Copyright (c) 2015, BogDan Vatra <bogdan@kde.org>

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

import QtQuick 2.6
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.1
import QtQuick.Window 2.2
import QtQuick.Controls 2.0

Rectangle {
    id: saveFilePage
    color: Qt.rgba(0, 0, 0, 0.75);

    property alias title: title.text
    property alias fileName: fileNameEdit.text
    property alias folder: pathEdit.text

    signal accpeted
    signal canceled

    GridLayout {
        columns: 2
        anchors.centerIn: parent

        FancyText {
            id: title
            Layout.alignment: Qt.AlignRight
        }

        FancyTextField {
            id: fileNameEdit
            implicitWidth: saveFilePage.width / 2
            Layout.fillWidth: true
            placeholderText: qsTr("Enter file name")
        }

        FancyText {
            text: qsTr("Path:")
            Layout.alignment: Qt.AlignRight
        }

        RowLayout {
            Layout.fillWidth: true
            FancyTextField {
                id: pathEdit
                Layout.fillWidth: true
                text: folder
                placeholderText: folder === "" ? qsTr("Enter file path") : ""
            }
            Button {
                FileDialog {
                    id: browseFolder
                    selectFolder: true
                    selectExisting: false
                    onAccepted: pathEdit.text = fileUrl
                    folder: pathEdit.text
                }
                text: qsTr("Browse")
                onClicked: browseFolder.open()
            }
        }

        Button {
            Layout.alignment: Qt.AlignRight
            text: qsTr("Save")
            onClicked: saveFilePage.accpeted()
        }
        Button {
            text: qsTr("Cancel")
            onClicked: saveFilePage.canceled()
        }
    }
}
