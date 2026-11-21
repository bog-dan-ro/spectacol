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

// @scope main.qml

import QtQuick
import QtQuick.Layouts
import QtGamepadLegacy
import Spectacol

Rectangle {
    id: calibration
    color: Theme.raised

    property alias deviceId: gamepad.deviceId
    signal configureDone()

    ColumnLayout {
        id: calibrationLayout
        anchors.centerIn: parent
        width: Math.max(0, calibration.width - Theme.sp(8))
        height: Math.min(implicitHeight, Math.max(0, calibration.height - Theme.sp(8)))
        spacing: Theme.sp(2)

        FancyText {
            id: configureText
            Layout.fillWidth: true
            visible: text.length > 0
            font.pixelSize: Theme.title
            font.weight: Font.DemiBold
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
        }
        FancyText {
            Layout.fillWidth: true
            visible: configureText.visible && diagram.targetButton !== GamepadManager.ButtonStart
            color: Theme.textMuted
            font.pixelSize: Theme.caption
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            text: qsTr("Press Start to skip this step")
        }
        StandardGamepad {
            id: diagram
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: calibrationLayout.width / 2
        }
    }

    Gamepad {
        id: gamepad
        onButtonAChanged: (value) => diagram.buttonAPressed = value
        onButtonBChanged: (value) => diagram.buttonBPressed = value
        onButtonXChanged: (value) => diagram.buttonXPressed = value
        onButtonYChanged: (value) => diagram.buttonYPressed = value
        onButtonL1Changed: (value) => diagram.buttonL1Pressed = value
        onButtonR1Changed: (value) => diagram.buttonR1Pressed = value
        onButtonL2Changed: (value) => diagram.buttonL2Pressed = value
        onButtonR2Changed: (value) => diagram.buttonR2Pressed = value
        onButtonUpChanged: (value) => diagram.buttonUpPressed = value
        onButtonLeftChanged: (value) => diagram.buttonLeftPressed = value
        onButtonDownChanged: (value) => diagram.buttonDownPressed = value
        onButtonRightChanged: (value) => diagram.buttonRightPressed = value
        onButtonStartChanged: (value) => diagram.buttonStartPressed = value
        onButtonSelectChanged: (value) => diagram.buttonSelectPressed = value
        onAxisLeftXChanged: (value) => diagram.axisLeftX = value
        onAxisLeftYChanged: (value) => diagram.axisLeftY = value
        onAxisRightXChanged: (value) => diagram.axisRightX = value
        onAxisRightYChanged: (value) => diagram.axisRightY = value
    }

    readonly property var buttons: [
        { code: GamepadManager.ButtonStart, label: qsTr("Start", "gamepad button") },
        { code: GamepadManager.ButtonSelect, label: qsTr("Select", "gamepad button") },
        { code: GamepadManager.ButtonA, label: "A" },
        { code: GamepadManager.ButtonB, label: "B" },
        { code: GamepadManager.ButtonX, label: "X" },
        { code: GamepadManager.ButtonY, label: "Y" },
        { code: GamepadManager.ButtonL1, label: "L1" },
        { code: GamepadManager.ButtonR1, label: "R1" },
        { code: GamepadManager.ButtonL2, label: "L2" },
        { code: GamepadManager.ButtonR2, label: "R2" },
        { code: GamepadManager.ButtonUp, label: qsTr("D-pad Up") },
        { code: GamepadManager.ButtonDown, label: qsTr("D-pad Down") },
        { code: GamepadManager.ButtonRight, label: qsTr("D-pad Right") },
        { code: GamepadManager.ButtonLeft, label: qsTr("D-pad Left") }
    ]
    readonly property var axes: [
        { code: GamepadManager.AxisLeftX, prompt: qsTr("Move the left stick horizontally") },
        { code: GamepadManager.AxisLeftY, prompt: qsTr("Move the left stick vertically") },
        { code: GamepadManager.AxisRightX, prompt: qsTr("Move the right stick horizontally") },
        { code: GamepadManager.AxisRightY, prompt: qsTr("Move the right stick vertically") }
    ]
    property int stepIndex: 0

    // ZxEmulator.processInputEvents nests, block it only once for the whole sequence
    property bool inputBlocked: false
    function blockInput(block)
    {
        if (inputBlocked === block)
            return;
        inputBlocked = block;
        ZxEmulator.processInputEvents = !block;
    }

    function configureNext()
    {
        blockInput(true);
        if (stepIndex < buttons.length) {
            const button = buttons[stepIndex++];
            diagram.targetButton = button.code;
            configureText.text = qsTr("Press %1").arg(button.label);
            GamepadManager.configureButton(deviceId, button.code);
            return;
        }

        diagram.targetButton = GamepadManager.ButtonInvalid;
        if (stepIndex - buttons.length < axes.length) {
            const axis = axes[stepIndex++ - buttons.length];
            diagram.targetAxis = axis.code;
            configureText.text = axis.prompt;
            GamepadManager.configureAxis(deviceId, axis.code);
        } else {
            diagram.targetAxis = GamepadManager.AxisInvalid;
            configureText.text = "";
            configureDone();
            blockInput(false);
        }
    }

    Timer {
        id: configTimer
        interval: 500; running: false; repeat: false
        onTriggered: configureNext();
    }

    Connections {
        target: GamepadManager
        // The parameter must keep its name: it shadows the root's deviceId alias.
        function onButtonConfigured(deviceId, button) {
            if (deviceId === calibration.deviceId)
                configTimer.restart();
        }
        function onAxisConfigured(deviceId, axis) {
            if (deviceId === calibration.deviceId)
                configTimer.restart();
        }
        function onConfigurationCanceled(deviceId) {
            if (deviceId === calibration.deviceId)
                configTimer.restart();
        }
    }

    Component.onCompleted: {
        deviceId = ZxEmulator.gamepadId;
        GamepadManager.resetConfiguration(deviceId);
        GamepadManager.setCancelConfigureButton(deviceId, GamepadManager.ButtonStart);
        configTimer.restart();
    }

    // The page can be closed half way (Back, Escape, F1, a tap outside),
    // don't leave the keyboard and the gamepad disabled.
    Component.onDestruction: blockInput(false)
}
