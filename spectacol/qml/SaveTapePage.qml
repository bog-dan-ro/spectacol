/*
    Copyright (c) 2015-2025, BogDan Vatra <bogdan@kde.org>

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
import QtQuick.Window
import QtQuick.Controls
import Spectacol

Rectangle {
    id: saveFilePage
    color: Theme.surface


    function writeTape()
    {
        ZxEmulator.tape.write(ZxEmulator.savesPath() + fileNameEdit.text +
                        (fileNameEdit.text.charAt(fileNameEdit.text.length - 4) === '.' ? "" : ".tzx"))
        pageLoader.source = ""
    }

    ColumnLayout {
        anchors.centerIn: parent

        RowLayout {
            FancyText {
                id: title
                text: ZxEmulator.savesPath()
            }
            FancyTextField {
                id: fileNameEdit
                focus: true
                text: ZxEmulator.tape.programName
                onAccepted: writeTape()
            }
            FancyText {
                id: ext
                text: ".tzx"
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
