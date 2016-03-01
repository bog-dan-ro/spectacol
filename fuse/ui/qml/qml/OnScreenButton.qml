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

import QtQuick 2.0
import QtQuick.Window 2.2
import Fuse 1.0

Rectangle {
    property variant buttons
    property alias imageSource: image.source
    property alias imageWidth: image.sourceSize.width
    property alias imageHeight: image.sourceSize.height
    property alias text: buttonText.text
    property alias animationRunning: animation.running

    width: imageWidth + Screen.pixelDensity * 2
    height: imageHeight + Screen.pixelDensity * 2
    radius: Screen.pixelDensity * 2
    color: "transparent"
    SequentialAnimation on color {
        id: animation
        running: false
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
        sourceSize.width: Screen.pixelDensity * 10
        sourceSize.height: Screen.pixelDensity * 10
    }
    FancyText {
        id: buttonText
        anchors.centerIn: parent
    }
    MouseArea {
        anchors.fill: parent
        onPressed: {
            animation.running = true;
            if (buttons) {
                for (var i = 0; i < buttons.length ; ++i)
                    fuse.keyPress(buttons[i]);
            }
        }
        onReleased: {
            animation.running = parent.focus;
            if (buttons) {
                for (var i = 0; i < buttons.length ; ++i)
                    fuse.keyRelease(buttons[i]);
            }
        }
    }
    Keys.onPressed: {
        if (event.key === Qt.Key_Return && buttons) {
            for (var i = 0; i < buttons.length ; ++i)
                fuse.keyPress(buttons[i]);
            event.accepted = true;
        }
    }
    Keys.onReleased: {
        if (event.key === Qt.Key_Return && buttons) {
            for (var i = 0; i < buttons.length ; ++i)
                fuse.keyRelease(buttons[i]);
            event.accepted = true;
        }
    }
}
