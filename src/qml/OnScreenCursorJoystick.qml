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
import QtQuick.Layouts
import QtQuick.Window
import QtGamepadLegacy
import Spectacol

Item {
    id: onScreenJoystick
    opacity: ZxEmulator.settings.joystickOpacity
    anchors.fill: parent
    anchors.topMargin: Screen.pixelDensity * 20
    property var buttonList: [button5, button56, button57, button6,
                               button7, button8, button86, button87,
                               button1, button2, button3, button4,
                               button0, button9]

    // The left side has 5 rows of buttons, shrink them on short (landscape phone) screens.
    readonly property real buttonSize: Math.max(0, Math.min(Screen.pixelDensity * 12,
                                                            (height - Screen.pixelDensity * 4) / 5))

    // The keys held by the pressed buttons. Several buttons share keys (e.g. 7 and
    // the 5+7 diagonal), so the emulator only gets the difference: sliding from one button
    // to another must not release a key that the new button still holds.
    readonly property var wantedKeys: {
        const keys = [];
        for (const but of buttonList) {
            if (!but.held || !but.buttons)
                continue;
            for (const key of but.buttons) {
                if (!keys.includes(key))
                    keys.push(key);
            }
        }
        return keys;
    }
    property var sentKeys: []
    onWantedKeysChanged: {
        for (const key of sentKeys) {
            if (!wantedKeys.includes(key))
                ZxEmulator.keyRelease(key);
        }
        for (const key of wantedKeys) {
            if (!sentKeys.includes(key))
                ZxEmulator.keyPress(key);
        }
        sentKeys = wantedKeys;
    }

    onVisibleChanged: {
        if (!visible) {
            for (const but of buttonList)
                but.releaseAll();
        }
    }

    // Balance ZxEmulator.processInputEvents, also when the Loader replaces this item.
    property bool inputBlocked: false
    function blockInput(block) {
        if (inputBlocked === block)
            return;
        inputBlocked = block;
        ZxEmulator.processInputEvents = !block;
    }

    Component.onDestruction: {
        for (const key of sentKeys)
            ZxEmulator.keyRelease(key);
        blockInput(false);
    }

    Keys.onReleased: (event) => {
        if (event.key === Qt.Key_Escape)
            parent.visible = false;
    }

    GamepadKeyNavigation {
        id: gamepad
        active: onScreenJoystick.visible
        gamepad: Gamepad { deviceId: ZxEmulator.gamepadId }
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

        onActiveChanged: blockInput(active)
    }

    RowLayout {
        anchors.fill: parent
        ColumnLayout {
            Layout.fillHeight: true
            GridLayout {
                columns: 2
                rows: 2
                OnScreenButton {
                    id: button1
                    size: onScreenJoystick.buttonSize
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
                    size: onScreenJoystick.buttonSize
                    KeyNavigation.up: button6
                    KeyNavigation.left: button1
                    KeyNavigation.down: button4
                    text: "2"
                    buttons: [Qt.Key_2]
                }
                OnScreenButton {
                    id: button3
                    size: onScreenJoystick.buttonSize
                    KeyNavigation.up: button1
                    KeyNavigation.right: button4
                    KeyNavigation.down: button7
                    text: "3"
                    buttons: [Qt.Key_3]
                }
                OnScreenButton {
                    id: button4
                    size: onScreenJoystick.buttonSize
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
                    size: onScreenJoystick.buttonSize
                    text: ""
                    imageSource: "qrc:///images/arrow-up-left.svg"
                    buttons: [Qt.Key_5, Qt.Key_7]
                }
                OnScreenButton {
                    id: button7
                    size: onScreenJoystick.buttonSize
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
                    size: onScreenJoystick.buttonSize
                    text: ""
                    imageSource: "qrc:///images/arrow-up-right.svg"
                    buttons: [Qt.Key_8, Qt.Key_7]
                }
                OnScreenButton {
                    id: button5
                    size: onScreenJoystick.buttonSize
                    KeyNavigation.up: button7
                    KeyNavigation.left: button0
                    KeyNavigation.right: button8
                    KeyNavigation.down: button6
                    text: "5"
                    imageSource: "qrc:///images/arrow-left.svg"
                    buttons: [Qt.Key_5]
                }

                OnScreenButton { size: onScreenJoystick.buttonSize }

                OnScreenButton {
                    id: button8
                    size: onScreenJoystick.buttonSize
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
                    size: onScreenJoystick.buttonSize
                    text: ""
                    imageSource: "qrc:///images/arrow-down-left.svg"
                    buttons: [Qt.Key_5, Qt.Key_6]
                }
                OnScreenButton {
                    id: button6
                    size: onScreenJoystick.buttonSize
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
                    size: onScreenJoystick.buttonSize
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
                size: onScreenJoystick.buttonSize
                KeyNavigation.up: button4
                KeyNavigation.left: button8
                KeyNavigation.right: button5
                KeyNavigation.down: button0
                text: "9"
                buttons: [Qt.Key_9]
            }
            OnScreenButton {
                id: button0
                size: onScreenJoystick.buttonSize
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
        for (const but of buttonList) {
            const rc = but.mapToItem(touchArea, 0, 0, but.width, but.height);
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
        for (const but of buttonList) {
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
