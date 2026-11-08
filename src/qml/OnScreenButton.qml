/*
    Copyright (c) 2016, BogDan Vatra <bogdan@kde.org>

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
import QtQuick.Window
import Spectacol
import "private"

Rectangle {
    id: button
    property variant buttons
    property alias imageSource: image.source
    property alias text: buttonText.text
    property real size: Screen.pixelDensity * 12
    property bool pressed1: false
    property bool pressed2: false
    property bool keyHeld: false // pressed with Return (gamepad A)
    // The keys are sent by the joystick: several buttons share keys (e.g. 7 and
    // the 5+7 diagonal) and the emulator must not see a key released while another
    // button still holds it.
    readonly property bool held: pressed1 || pressed2 || keyHeld

    function releaseAll() {
        pressed1 = false;
        pressed2 = false;
        keyHeld = false;
    }

    implicitWidth: size
    implicitHeight: size
    radius: Screen.pixelDensity * 2
    color: "transparent"
    border.color: "gray"

    SequentialAnimation on color {
        id: animation
        running: button.focus || button.held
        alwaysRunToEnd: true
        loops: Animation.Infinite
        ColorAnimation { from: "transparent"; to: "#009688"; duration: 500 }
        ColorAnimation { from: "#009688"; to: "transparent"; duration: 500 }
    }
    Image {
        id: image
        anchors.centerIn: parent
        opacity: 0.5
        fillMode: Image.Stretch
        sourceSize.width: button.size - Screen.pixelDensity * 2
        sourceSize.height: button.size - Screen.pixelDensity * 2
    }
    FancyText {
        id: buttonText
        font.pixelSize: TextSizes.scale16
        anchors.centerIn: parent
    }
    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Return && buttons) {
            keyHeld = true;
            event.accepted = true;
        }
    }
    Keys.onReleased: (event) => {
        if (event.key === Qt.Key_Return && buttons) {
            keyHeld = false;
            event.accepted = true;
        }
    }
}
