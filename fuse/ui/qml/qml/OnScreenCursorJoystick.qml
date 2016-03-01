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
    property alias gamepadMode: gamepad.active
    Keys.onReleased: {
        if (event.key === Qt.Key_X) {
            onScreenJoystick.gamepadMode = false;
            onScreenJoystick.visible = false;
        }
    }

    GamepadKeyNavigation {
        id: gamepad
        gamepad: Gamepad { deviceId: fuse.gamepadId }
        buttonAKey: Qt.Key_Return
        buttonBKey: Qt.Key_unknown
        buttonXKey: Qt.Key_X
        buttonYKey: Qt.Key_unknown

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
                    focus: gamepadMode
                    animationRunning: gamepadMode && focus
                    KeyNavigation.up: button6
                    KeyNavigation.left: button0
                    KeyNavigation.right: button2
                    KeyNavigation.down: button3
                    border.color: "gray"
                    text: "1"
                    buttons: [Qt.Key_1]
                }
                OnScreenButton {
                    id: button2
                    KeyNavigation.up: button6
                    KeyNavigation.left: button1
                    KeyNavigation.down: button4
                    animationRunning: gamepadMode && focus
                    border.color: "gray"
                    text: "2"
                    buttons: [Qt.Key_2]
                }
                OnScreenButton {
                    id: button3
                    KeyNavigation.up: button1
                    KeyNavigation.right: button4
                    KeyNavigation.down: button7
                    animationRunning: gamepadMode && focus
                    border.color: "gray"
                    text: "3"
                    buttons: [Qt.Key_3]
                }
                OnScreenButton {
                    id: button4
                    KeyNavigation.up: button2
                    KeyNavigation.left: button3
                    KeyNavigation.down: button7
                    animationRunning: gamepadMode && focus
                    border.color: "gray"
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
                    border.color: "gray"
                    animationRunning: gamepadMode && focus
                    text: "7"
                    imageSource: "qrc:///images/arrow-up.svg"
                    buttons: [Qt.Key_7]
                }
                OnScreenButton {
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
                    border.color: "gray"
                    animationRunning: gamepadMode && focus
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
                    border.color: "gray"
                    animationRunning: gamepadMode && focus
                    text: "8"
                    imageSource: "qrc:///images/arrow-right.svg"
                    buttons: [Qt.Key_8]
                }
                OnScreenButton {
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
                    border.color: "gray"
                    animationRunning: gamepadMode && focus
                    text: "6"
                    imageSource: "qrc:///images/arrow-down.svg"
                    buttons: [Qt.Key_6]
                }
                OnScreenButton {
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
                id: button0
                KeyNavigation.up: button4
                KeyNavigation.left: button8
                KeyNavigation.right: button5
                KeyNavigation.down: button6
                border.color: "gray"
                animationRunning: gamepadMode && focus
                text: "0"
                imageSource: "qrc:///images/draw-fire.svg"
                buttons: [Qt.Key_0]
            }
        }
    }
}
