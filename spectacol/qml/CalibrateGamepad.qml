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

// @scope main.qml

import QtQuick
import QtGamepadLegacy
import Spectacol
import "private"

StandardGamepad {
    id: confGamepad

    backgroundColor: Qt.rgba(0, 0, 0, 0.75);

    property alias deviceId: gamepad.deviceId
    signal configureDone()

    FancyText {
        id: configureText
        font.pixelSize: TextSizes.scale12
        visible: false
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        horizontalAlignment: Text.AlignHCenter
    }

    FancyText {
        id: cancelButtonText
        font.pixelSize: TextSizes.scale12
        visible: false
        anchors.top: configureText.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        text: qsTr("Press <b>Start</b> button to skip this configuation");
    }

    Gamepad {
        id: gamepad
        onButtonAChanged: confGamepad.buttonAPressed = value
        onButtonBChanged: confGamepad.buttonBPressed = value
        onButtonXChanged: confGamepad.buttonXPressed = value
        onButtonYChanged: confGamepad.buttonYPressed = value
        onButtonL1Changed: confGamepad.buttonL1Pressed = value
        onButtonR1Changed: confGamepad.buttonR1Pressed = value
        onButtonL2Changed: confGamepad.buttonL2Pressed = value
        onButtonR2Changed: confGamepad.buttonR2Pressed = value
        onButtonUpChanged: confGamepad.buttonUpPressed = value
        onButtonLeftChanged: confGamepad.buttonLeftPressed = value
        onButtonDownChanged: confGamepad.buttonDownPressed = value
        onButtonRightChanged: confGamepad.buttonRightPressed = value
        onButtonStartChanged: confGamepad.buttonStartPressed = value
        onButtonSelectChanged: confGamepad.buttonSelectPressed = value
        onAxisLeftXChanged: confGamepad.axisLeftX = value
        onAxisLeftYChanged: confGamepad.axisLeftY = value
        onAxisRightXChanged: confGamepad.axisRightX = value
        onAxisRightYChanged: confGamepad.axisRightY = value
    }

    property var __buttons: [GamepadManager.ButtonStart,
        GamepadManager.ButtonSelect,
        GamepadManager.ButtonA,
        GamepadManager.ButtonB,
        GamepadManager.ButtonX,
        GamepadManager.ButtonY,
        GamepadManager.ButtonL1,
        GamepadManager.ButtonR1,
        GamepadManager.ButtonL2,
        GamepadManager.ButtonR2,
        GamepadManager.ButtonUp,
        GamepadManager.ButtonDown,
        GamepadManager.ButtonRight,
        GamepadManager.ButtonLeft];

    property var __axes: [GamepadManager.AxisLeftX,
        GamepadManager.AxisLeftY,
        GamepadManager.AxisRightX,
        GamepadManager.AxisRightY];

    function pressButton(button)
    {
        switch(button) {
        case GamepadManager.ButtonA:
            confGamepad.buttonAPressed = true;
            return 'A';

        case GamepadManager.ButtonB:
            confGamepad.buttonBPressed = true;
            return 'B';

        case GamepadManager.ButtonX:
            confGamepad.buttonXPressed = true;
            return 'X';

        case GamepadManager.ButtonY:
            confGamepad.buttonYPressed = true;
            return 'Y';

        case GamepadManager.ButtonL1:
            confGamepad.buttonL1Pressed = true;
            return 'L1';

        case GamepadManager.ButtonR1:
            confGamepad.buttonR1Pressed = true;
            return 'R1';

        case GamepadManager.ButtonL2:
            confGamepad.buttonL2Pressed = true;
            return 'L2';

        case GamepadManager.ButtonR2:
            confGamepad.buttonR2Pressed = true;
            return 'R2';

        case GamepadManager.ButtonSelect:
            confGamepad.buttonSelectPressed = true;
            return 'Select';

        case GamepadManager.ButtonStart:
            confGamepad.buttonStartPressed = true;
            return 'Start';

        case GamepadManager.ButtonUp:
            confGamepad.buttonUpPressed = true;
            return 'Up';

        case GamepadManager.ButtonDown:
            confGamepad.buttonDownPressed = true;
            return 'Down';

        case GamepadManager.ButtonRight:
            confGamepad.buttonRightPressed = true;
            return 'Right';

        case GamepadManager.ButtonLeft:
            confGamepad.buttonLeftPressed = true;
            return 'Left';
        }
    }

    function pressAxis(axis)
    {
        switch (axis) {
        case GamepadManager.AxisLeftX:
            confGamepad.axisLeftX = 1;
            return 'left joystick horizontal';
        case GamepadManager.AxisLeftY:
            confGamepad.axisLeftY = 1;
            return 'left joystick vertical';
        case GamepadManager.AxisRightX:
            confGamepad.axisRightX = 1;
            return 'right joystick horizontal';
        case GamepadManager.AxisRightY:
            confGamepad.axisRightY = 1;
            return 'right joystick vertical';
        }
    }

    function configureNext()
    {
        FuseEmulator.processInputEvents = false;
        if (__buttons.length > 0) {
            var but = __buttons.shift();
            GamepadManager.configureButton(deviceId, but);
            configureText.text = qsTr("Press button <b>") + pressButton(but) + ("</b> on your gamepad");
            configureText.visible = true;
            cancelButtonText.visible = but !== GamepadManager.ButtonStart;
        } else if (__axes.length > 0) {
            var axis = __axes.shift();
            GamepadManager.configureAxis(deviceId, axis);
            configureText.text = qsTr("Press <b>") + pressAxis(axis) + ("</b> axis on your gamepad");
            configureText.visible = true;
            cancelButtonText.visible = true;
        } else {
            configureText.visible = false;
            cancelButtonText.visible = false;
            configureDone();
            FuseEmulator.processInputEvents = true;
        }
    }

    Timer {
        id: configTimer
        interval: 500; running: false; repeat: false
        onTriggered: configureNext();
    }

    Connections {
        target: GamepadManager
        onButtonConfigured: {
            if (deviceId === confGamepad.deviceId)
                configTimer.restart();
        }
        onAxisConfigured: {
            if (deviceId === confGamepad.deviceId)
                configTimer.restart();
        }
        onConfigurationCanceled: {
            if (deviceId === confGamepad.deviceId)
                configTimer.restart();
        }
    }

    Component.onCompleted: {
        deviceId = FuseEmulator.gamepadId;
        GamepadManager.resetConfiguration(deviceId);
        GamepadManager.setCancelConfigureButton(deviceId, GamepadManager.ButtonStart);
        configTimer.restart();
    }
}
