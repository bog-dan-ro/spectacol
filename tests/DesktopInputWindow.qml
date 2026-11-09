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
import QtGamepadLegacy

ApplicationWindow {
    id: window
    width: 1280
    height: 800
    visible: true
    property int escapeCount: 0
    property int acceptedCount: 0
    property bool inputPanelVisible: false
    property int searchShortcutCount: 0

    TextField {
        objectName: "editor"
        x: 20
        y: 20
        width: 300
        inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase
        Keys.onEscapePressed: window.escapeCount++
        Keys.onPressed: event => {
            if (event.key === Qt.Key_F3) {
                window.searchShortcutCount++;
                event.accepted = true;
            }
        }
        onAccepted: window.acceptedCount++
    }
    Button {
        objectName: "otherControl"
        x: 400
        y: 20
        text: "Other"
    }
    GamepadNavigation {
        textInputActive: window.inputPanelVisible
        gamepad: Gamepad { deviceId: 4242 }
        buttonAKey: Qt.Key_Return
        buttonBKey: Qt.Key_Escape
        buttonXKey: Qt.Key_F3
    }
}
