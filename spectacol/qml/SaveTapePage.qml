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

import QtQuick 2.12
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.12
import QtQuick.Window 2.12
import QtQuick.Controls 2.12

Rectangle {
    id: saveFilePage
    color: Qt.rgba(0, 0, 0, 0.75);

    Component.onCompleted: fuse.paused = true
    Component.onDestruction: fuse.paused = false

    function writeTape()
    {
        fuse.tape.write(fuse.savesPath() + fileNameEdit.text +
                        (fileNameEdit.text.charAt(fileNameEdit.text.length - 4) === '.' ? "" : ".tzx"))
        onClicked: pageLoader.source = ""
    }

    ColumnLayout {
        anchors.centerIn: parent

        RowLayout {
            FancyText {
                id: title
                text: fuse.savesPath()
            }
            FancyTextField {
                id: fileNameEdit
                focus: true
                text: fuse.tape.programName
                onAccepted: writeTape()
            }
            FancyText {
                id: ext
                text: ".txz"
            }
        }

        RowLayout {
            Button {
                Layout.alignment: Qt.AlignRight
                text: qsTr("Save")
                onClicked: writeTape()
            }
            Button {
                text: qsTr("Cancel")
                onClicked: pageLoader.source = ""
            }
        }
    }
}
