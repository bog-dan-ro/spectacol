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
import QtQuick.Controls
import QtQuick.Layouts
import Spectacol

Rectangle {
    objectName: "desktopVirtualKeyboard"
    color: Theme.raised
    implicitHeight: Theme.rowHeight + keyboard.height

    RowLayout {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: Theme.rowHeight
        spacing: Theme.sp(2)
        FancyTextField {
            Layout.fillWidth: true
            readOnly: true
            focusPolicy: Qt.NoFocus
            text: DesktopInput.text
            Accessible.name: qsTr("Text being edited")
        }
        ToolButton {
            text: "\u232b"
            focusPolicy: Qt.NoFocus
            Accessible.name: qsTr("Delete character")
            onClicked: DesktopInput.sendKey(Qt.Key_Backspace)
        }
        ToolButton {
            text: qsTr("Close (B)")
            focusPolicy: Qt.NoFocus
            onClicked: DesktopInput.hide()
        }
    }

    OnScreen48Keyboard {
        id: keyboard
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: width * 0.4
        textInputMode: true
        gamepadMode: Theme.gamepadHints
        onTextKeyPressed: (key, text) => {
            if (text.length)
                DesktopInput.commitText(text);
            else
                DesktopInput.sendKey(key);
        }
    }
    Connections {
        target: DesktopInput
        function onNavigationKey(key, pressed) {
            keyboard.navigateTextInput(key, pressed);
        }
    }
}
