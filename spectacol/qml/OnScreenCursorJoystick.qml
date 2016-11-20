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
import QtQuick.Layouts 1.1
import QtQuick.Window 2.2
import QtGamepad 1.0

Item {
    id: onScreenJoystick
    anchors.fill: parent
    anchors.topMargin: Screen.pixelDensity * 20
    property var buttonList: [button5, button56, button57, button6,
                               button7, button8, button86, button87,
                               button1, button2, button3, button4,
                               button0, button9]

    Keys.onReleased: {
        if (event.key === Qt.Key_Escape)
            parent.visible = false;
    }

    GamepadKeyNavigation {
        id: gamepad
        active: onScreenJoystick.visible
        gamepad: Gamepad { deviceId: fuse.gamepadId }
        buttonAKey: Qt.Key_Return
        buttonBKey: Qt.Key_Escape
        buttonXKey: Qt.Key_unknown
        buttonYKey: Qt.Key_unknown
        buttonGuideKey: Qt.Key_unknown
        buttonL1Key: Qt.Key_unknown
        buttonL2Key: Qt.Key_unknown
        buttonL3Key: Qt.Key_unknown
        buttonR1Key: Qt.Key_unknown
        buttonR2Key: Qt.Key_unknown
        buttonR3Key: Qt.Key_unknown
        buttonSelectKey: Qt.Key_unknown
        buttonStartKey: Qt.Key_unknown

        onActiveChanged: fuse.processInputEvents = !active
    }

    RowLayout {
        anchors.fill: parent
        ColumnLayout {
            width: Screen.pixelDensity * 24
            Layout.fillHeight: true
            GridLayout {
                columns: 2
                rows: 2
                OnScreenButton {
                    id: button1
                    focus: onScreenJoystick.visible
                    KeyNavigation.up: button6
                    KeyNavigation.left: button0
                    KeyNavigation.right: button2
                    KeyNavigation.down: button3
                    text: "1"
                    buttons: [Qt.Key_1]
                }
                OnScreenButton {
                    id: button2
                    KeyNavigation.up: button6
                    KeyNavigation.left: button1
                    KeyNavigation.down: button4
                    text: "2"
                    buttons: [Qt.Key_2]
                }
                OnScreenButton {
                    id: button3
                    KeyNavigation.up: button1
                    KeyNavigation.right: button4
                    KeyNavigation.down: button7
                    text: "3"
                    buttons: [Qt.Key_3]
                }
                OnScreenButton {
                    id: button4
                    KeyNavigation.up: button2
                    KeyNavigation.left: button3
                    KeyNavigation.down: button7
                    text: "4"
                    buttons: [Qt.Key_4]
                }
            }

            Item {
                Layout.fillHeight: true
            }
            GridLayout {
                columns: 3
                rows: 3
                OnScreenButton {
                    id: button57
                    text: ""
                    imageSource: "qrc:///images/arrow-up-left.svg"
                    buttons: [Qt.Key_5, Qt.Key_7]
                }
                OnScreenButton {
                    id: button7
                    KeyNavigation.up: button4
                    KeyNavigation.left: button5
                    KeyNavigation.right: button8
                    KeyNavigation.down: button6
                    text: "7"
                    imageSource: "qrc:///images/arrow-up.svg"
                    buttons: [Qt.Key_7]
                }
                OnScreenButton {
                    id: button87
                    text: ""
                    imageSource: "qrc:///images/arrow-up-right.svg"
                    buttons: [Qt.Key_8, Qt.Key_7]
                }
                OnScreenButton {
                    id: button5
                    KeyNavigation.up: button7
                    KeyNavigation.left: button0
                    KeyNavigation.right: button8
                    KeyNavigation.down: button6
                    text: "5"
                    imageSource: "qrc:///images/arrow-left.svg"
                    buttons: [Qt.Key_5]
                }

                OnScreenButton {}

                OnScreenButton {
                    id: button8
                    KeyNavigation.up: button7
                    KeyNavigation.left: button5
                    KeyNavigation.right: button0
                    KeyNavigation.down: button6
                    text: "8"
                    imageSource: "qrc:///images/arrow-right.svg"
                    buttons: [Qt.Key_8]
                }
                OnScreenButton {
                    id: button56
                    text: ""
                    imageSource: "qrc:///images/arrow-down-left.svg"
                    buttons: [Qt.Key_5, Qt.Key_6]
                }
                OnScreenButton {
                    id: button6
                    KeyNavigation.up: button7
                    KeyNavigation.left: button5
                    KeyNavigation.right: button8
                    KeyNavigation.down: button1
                    text: "6"
                    imageSource: "qrc:///images/arrow-down.svg"
                    buttons: [Qt.Key_6]
                }
                OnScreenButton {
                    id: button86
                    text: ""
                    imageSource: "qrc:///images/arrow-down-right.svg"
                    buttons: [Qt.Key_8, Qt.Key_6]
                }
            }
        }
        Item {
            Layout.fillHeight: true
            Layout.fillWidth: true
        }
        ColumnLayout {
            Layout.fillHeight: true
            Item {
                Layout.fillHeight: true
            }
            OnScreenButton {
                id: button9
                KeyNavigation.up: button4
                KeyNavigation.left: button8
                KeyNavigation.right: button5
                KeyNavigation.down: button0
                text: "9"
                buttons: [Qt.Key_9]
            }
            OnScreenButton {
                id: button0
                KeyNavigation.up: button9
                KeyNavigation.left: button8
                KeyNavigation.right: button5
                KeyNavigation.down: button6
                text: "0"
                imageSource: "qrc:///images/draw-fire.svg"
                buttons: [Qt.Key_0]
            }
        }
    }

    function pressButton(x, y, which) {
        var len = buttonList.length;
        for (var i = 0; i < len; ++i) {
            var but = buttonList[i];
            var rc = but.mapToItem(touchArea, 0, 0, but.width, but.height);
            if (x < rc.x || x > rc.x + rc.width ||
                y < rc.y || y > rc.y + rc.height) {
                switch(which) {
                case 1:
                    if (but.pressed1)
                        but.pressed1 = false;
                    break;
                case 2:
                    if (but.pressed2)
                        but.pressed2 = false;
                    break;
                }
                continue;
            }
            switch(which) {
            case 1:
                if (!but.pressed1)
                    but.pressed1 = true;
                break;
            case 2:
                if (!but.pressed2)
                    but.pressed2 = true;
                break;
            }
        }
    }

    function releaseButton(which) {
        var len = buttonList.length;
        for (var i = 0; i < len; ++i) {
            var but = buttonList[i];
            switch(which) {
            case 1:
                if (but.pressed1)
                    but.pressed1 = false;
                break;
            case 2:
                if (but.pressed2)
                    but.pressed2 = false;
                break;
            }
        }
    }

    MultiPointTouchArea {
        id: touchArea
        anchors.fill: parent
        touchPoints: [
            TouchPoint { id: point1
                onPressedChanged: pressed ? pressButton(x, y, 1) : releaseButton(1);
                onAreaChanged: pressButton(x, y, 1)
            },
            TouchPoint { id: point2
                onPressedChanged: pressed ? pressButton(x, y, 2) : releaseButton(2);
                onAreaChanged: pressButton(x, y, 2)
            }
        ]
    }
}
