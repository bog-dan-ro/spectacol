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
import QtGamepadLegacy
import Spectacol

// Shared page navigation. GUI keys let modal dialogs
// receive input before the page; gameplay input remains blocked by the page.
QtObject {
    id: root
    property bool transportKeys: true
    property int buttonXKey: Qt.Key_F2
    property int buttonYKey: Qt.Key_F3
    property int buttonStartKey: Qt.Key_Menu
    property int leftShoulderKey: Qt.Key_F11
    property int rightShoulderKey: Qt.Key_F5
    property int leftTriggerKey: Qt.Key_F12
    property int rightTriggerKey: Qt.Key_F10
    property bool ready: false
    Component.onCompleted: ready = true
    property int direction: 0
    property bool repeating: false
    function tap(key: int) {
        if (key !== Qt.Key_unknown && ready && Qt.application.state === Qt.ApplicationActive)
            ZxEmulator.sendNavigationKey(key);
    }
    function updateDirection() {
        if (!repeat)
            return;
        if (Qt.application.state !== Qt.ApplicationActive) {
            direction = 0;
            repeat.stop();
            return;
        }
        const x = pad.buttonLeft ? -1 : pad.buttonRight ? 1 : Math.abs(pad.axisLeftX) >= 0.5 ? Math.sign(pad.axisLeftX) : 0;
        const y = pad.buttonUp ? -1 : pad.buttonDown ? 1 : Math.abs(pad.axisLeftY) >= 0.5 ? Math.sign(pad.axisLeftY) : 0;
        const next = y < 0 ? Qt.Key_Up : y > 0 ? Qt.Key_Down : x < 0 ? Qt.Key_Left : x > 0 ? Qt.Key_Right : 0;
        if (direction === next)
            return;
        repeat.stop();
        repeating = false;
        direction = next;
        if (direction) {
            tap(direction);
            repeat.start();
        }
    }
    property Gamepad pad: Gamepad {
        id: pad
        deviceId: ZxEmulator.gamepadId
        readonly property bool leftTrigger: buttonL2 >= 0.5
        readonly property bool rightTrigger: buttonR2 >= 0.5
        onButtonUpChanged: root.updateDirection()
        onButtonDownChanged: root.updateDirection()
        onButtonLeftChanged: root.updateDirection()
        onButtonRightChanged: root.updateDirection()
        onAxisLeftXChanged: root.updateDirection()
        onAxisLeftYChanged: root.updateDirection()
        onButtonAChanged: if (buttonA)
            root.tap(Qt.Key_Select)
        onButtonBChanged: if (buttonB)
            root.tap(Qt.Key_Escape)
        onButtonXChanged: if (buttonX)
            root.tap(DesktopInput.visible ? Qt.Key_Backspace : root.buttonXKey)
        onButtonYChanged: if (buttonY)
            root.tap(root.buttonYKey)
        onButtonStartChanged: if (buttonStart)
            root.tap(root.buttonStartKey)
        onButtonSelectChanged: if (buttonSelect)
            root.tap(Qt.Key_Escape)
        onButtonL1Changed: if (buttonL1 && root.transportKeys)
            root.tap(root.leftShoulderKey)
        onButtonR1Changed: if (buttonR1 && root.transportKeys)
            root.tap(root.rightShoulderKey)
        onLeftTriggerChanged: if (leftTrigger && root.transportKeys)
            root.tap(root.leftTriggerKey)
        onRightTriggerChanged: if (rightTrigger && root.transportKeys)
            root.tap(root.rightTriggerKey)
        onButtonL3Changed: if (buttonL3)
            root.tap(Qt.Key_PageUp)
        onButtonR3Changed: if (buttonR3)
            root.tap(Qt.Key_PageDown)
        onDeviceIdChanged: {
            root.direction = 0;
            if (root.repeat)
                root.repeat.stop();
        }
    }
    property Connections applicationConnection: Connections {
        target: Qt.application
        function onStateChanged() {
            if (Qt.application.state !== Qt.ApplicationActive) {
                root.direction = 0;
                if (root.repeat)
                    root.repeat.stop();
            }
        }
    }
    property Timer repeat: Timer {
        id: repeat
        interval: root.repeating ? 90 : 400
        repeat: false
        onTriggered: {
            if (!root.direction)
                return;
            root.tap(root.direction);
            root.repeating = true;
            restart();
        }
    }
}
