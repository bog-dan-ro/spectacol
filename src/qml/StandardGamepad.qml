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

Canvas {
    id: canvas
    property color strokeColor: "black"
    property color gamepadFillColor: "lightgray"

    property int targetButton: GamepadManager.ButtonInvalid
    property int targetAxis: GamepadManager.AxisInvalid
    readonly property var buttonBounds: ({
        [GamepadManager.ButtonStart]: Qt.rect(520, 165, 100, 30),
        [GamepadManager.ButtonSelect]: Qt.rect(380, 165, 100, 30),
        [GamepadManager.ButtonA]: Qt.rect(760, 270, 60, 60),
        [GamepadManager.ButtonB]: Qt.rect(820, 210, 60, 60),
        [GamepadManager.ButtonX]: Qt.rect(700, 210, 60, 60),
        [GamepadManager.ButtonY]: Qt.rect(760, 150, 60, 60),
        [GamepadManager.ButtonL1]: Qt.rect(150, 55, 100, 30),
        [GamepadManager.ButtonR1]: Qt.rect(750, 55, 100, 30),
        [GamepadManager.ButtonL2]: Qt.rect(150, 15, 100, 30),
        [GamepadManager.ButtonR2]: Qt.rect(750, 15, 100, 30),
        [GamepadManager.ButtonUp]: Qt.rect(165, 140, 50, 75),
        [GamepadManager.ButtonDown]: Qt.rect(165, 265, 50, 75),
        [GamepadManager.ButtonRight]: Qt.rect(215, 215, 75, 50),
        [GamepadManager.ButtonLeft]: Qt.rect(90, 215, 75, 50)
    })
    readonly property rect leftStickBounds: Qt.rect(220, 290, 180, 180)
    readonly property rect rightStickBounds: Qt.rect(600, 290, 180, 180)
    readonly property rect targetBounds: {
        if (targetAxis === GamepadManager.AxisLeftX || targetAxis === GamepadManager.AxisLeftY)
            return leftStickBounds;
        if (targetAxis === GamepadManager.AxisRightX || targetAxis === GamepadManager.AxisRightY)
            return rightStickBounds;
        return buttonBounds[targetButton] ?? Qt.rect(0, 0, 0, 0);
    }
    readonly property real diagramScale: Math.min(width / 1000, height / 500)

    property var buttonArrowsColors: {"up": "midnightblue", "down": "purple", "border": "white"}
    property var buttonStartColors: {"up": "white", "down": "gray", "border": "black"}
    property var buttonSelectColors: {"up": "white", "down": "gray", "border": "black"}
    property var buttonAColors: {"up": "green", "down": "lightgreen", "border": "black"}
    property var buttonBColors: {"up": "red", "down": "pink", "border": "black"}
    property var buttonXColors: {"up": "blue", "down": "lightblue", "border": "black"}
    property var buttonYColors: {"up": "yellow", "down": "lightyellow", "border": "black"}
    property var buttonL1Colors: {"up": "white", "down": "gray", "border": "black"}
    property var buttonR1Colors: {"up": "white", "down": "gray", "border": "black"}
    property var buttonL2Colors: {"up": "white", "down": "gray", "border": "black"}
    property var buttonR2Colors: {"up": "white", "down": "gray", "border": "black"}

    property bool buttonStartPressed: false
    property bool buttonSelectPressed: false
    property bool buttonAPressed: false
    property bool buttonBPressed: false
    property bool buttonXPressed: false
    property bool buttonYPressed: false
    property bool buttonUpPressed: false
    property bool buttonDownPressed: false
    property bool buttonLeftPressed: false
    property bool buttonRightPressed: false
    property bool buttonL1Pressed: false
    property bool buttonR1Pressed: false
    property bool buttonL2Pressed: false
    property bool buttonR2Pressed: false

    property double  axisLeftX : 0
    property double  axisLeftY : 0
    property double  axisRightX : 0
    property double  axisRightY : 0

    onButtonSelectPressedChanged: requestPaint()
    onButtonStartPressedChanged: requestPaint()
    onButtonAPressedChanged: requestPaint()
    onButtonBPressedChanged: requestPaint()
    onButtonXPressedChanged: requestPaint()
    onButtonYPressedChanged: requestPaint()
    onButtonUpPressedChanged:requestPaint()
    onButtonDownPressedChanged: requestPaint()
    onButtonLeftPressedChanged: requestPaint()
    onButtonRightPressedChanged: requestPaint()
    onButtonL1PressedChanged: requestPaint()
    onButtonR1PressedChanged: requestPaint()
    onButtonL2PressedChanged: requestPaint()
    onButtonR2PressedChanged: requestPaint()

    onAxisLeftXChanged: requestPaint()
    onAxisLeftYChanged: requestPaint()
    onAxisRightXChanged: requestPaint()
    onAxisRightYChanged: requestPaint()

    onWidthChanged: requestPaint()
    onHeightChanged: requestPaint()

    function drawButton(context, button, colors, pressed, text)
    {
        const bounds = buttonBounds[button];
        const round = bounds.width === bounds.height;
        context.save();
        context.fillStyle = pressed ? colors.down : colors.up;
        context.strokeStyle = colors.border;
        context.lineWidth = 5;

        context.beginPath();
        if (round)
            context.ellipse(bounds.x, bounds.y, bounds.width, bounds.height);
        else
            context.roundedRect(bounds.x, bounds.y, bounds.width, bounds.height, 2.5, 2.5);
        context.closePath();
        context.fill();
        context.stroke();

        if (text) {
            context.lineWidth = round ? 3 : 2;
            context.font = round ? "30px sans-serif" : "20px sans-serif";
            context.textAlign = "center";
            context.textBaseline = "middle";
            context.strokeText(text, bounds.x + bounds.width / 2, bounds.y + bounds.height / 2);
        }
        context.restore();
    }

    function drawJoystick(context, bounds, ax, ay)
    {
        const x = bounds.x;
        const y = bounds.y;
        const wh = bounds.width;
        context.save();
        context.fillStyle = "lightgray";
        context.strokeStyle = "black";
        context.lineWidth = 10;

        context.beginPath();
        context.ellipse(x, y, wh, wh);
        context.fill();


        context.fillStyle = "gray";
        context.stroke();
        context.beginPath();
        context.ellipse(x + wh / 10, y + wh / 10, wh - wh / 5, wh - wh / 5);
        context.fill();
        context.stroke();

        context.lineWidth = 8;
        context.beginPath();
        context.moveTo(x + wh / 2 , y + wh / 5);
        context.lineTo(x + wh / 2 , y + wh - wh / 5);
        context.moveTo(x + wh / 5 , y + wh / 2);
        context.lineTo(x + wh - wh / 5, y + wh / 2);
        context.stroke();

        context.lineWidth = 6;
        context.strokeStyle = "magenta";
        context.beginPath();
        if (ay) {
            context.moveTo(x+ wh / 2 , y + wh / 2);
            context.lineTo(x + wh / 2, y + (wh / 2) + ay *(1.5 * wh / 5) );
        }
        if (ax) {
            context.moveTo(x+ wh / 2 , y + wh / 2);
            context.lineTo(x + (wh / 2) + ax *(1.5 * wh / 5), y + wh / 2 );
        }
        context.stroke();
        context.restore();
    }

    onPaint: {
        var context = canvas.getContext("2d");
        context.reset();
        context.clearRect(0, 0, canvas.width, canvas.height);
        context.translate((canvas.width - 1000 * diagramScale) / 2,
                          (canvas.height - 500 * diagramScale) / 2);
        context.scale(diagramScale, diagramScale);

        context.fillStyle = gamepadFillColor;
        context.strokeStyle = strokeColor;
        context.lineWidth = 10;

        context.beginPath();
        context.roundedRect(200, 100, 600, 300, context.lineWidth / 2, context.lineWidth / 2);
        context.fill();
        context.stroke();

        context.beginPath();
        context.ellipse(50, 100, 280, 280);
        context.ellipse(650, 100, 280, 280);
        context.fill();
        context.stroke();

        context.lineWidth = 5;
        context.fillStyle = buttonArrowsColors["up"]
        context.strokeStyle = buttonArrowsColors["border"];
        context.beginPath();
        context.roundedRect(90, 215, 200, 50, context.lineWidth / 2, context.lineWidth / 2);
        context.roundedRect(165, 140, 50, 200, context.lineWidth / 2, context.lineWidth / 2);
        context.fill();
        context.stroke();

        if (buttonUpPressed)
            drawButton(context, GamepadManager.ButtonUp, buttonArrowsColors, true);
        if (buttonDownPressed)
            drawButton(context, GamepadManager.ButtonDown, buttonArrowsColors, true);
        if (buttonLeftPressed)
            drawButton(context, GamepadManager.ButtonLeft, buttonArrowsColors, true);
        if (buttonRightPressed)
            drawButton(context, GamepadManager.ButtonRight, buttonArrowsColors, true);

        drawButton(context, GamepadManager.ButtonY, buttonYColors, buttonYPressed, "Y");
        drawButton(context, GamepadManager.ButtonA, buttonAColors, buttonAPressed, "A");
        drawButton(context, GamepadManager.ButtonX, buttonXColors, buttonXPressed, "X");
        drawButton(context, GamepadManager.ButtonB, buttonBColors, buttonBPressed, "B");
        drawButton(context, GamepadManager.ButtonSelect, buttonSelectColors, buttonSelectPressed, "Select");
        drawButton(context, GamepadManager.ButtonStart, buttonStartColors, buttonStartPressed, "Start");
        drawButton(context, GamepadManager.ButtonL1, buttonL1Colors, buttonL1Pressed, "L1");
        drawButton(context, GamepadManager.ButtonL2, buttonL2Colors, buttonL2Pressed, "L2");
        drawButton(context, GamepadManager.ButtonR1, buttonR1Colors, buttonR1Pressed, "R1");
        drawButton(context, GamepadManager.ButtonR2, buttonR2Colors, buttonR2Pressed, "R2");

        drawJoystick(context, leftStickBounds, axisLeftX, axisLeftY);
        drawJoystick(context, rightStickBounds, axisRightX, axisRightY);
    }

    // The outline uses the drawing's coordinates but animates independently of the canvas.
    Item {
        width: 1000
        height: 500
        anchors.centerIn: parent
        scale: canvas.diagramScale
        visible: canvas.targetBounds.width > 0 && scale > 0
        Accessible.ignored: true

        Rectangle {
            id: outline
            readonly property int stroke: Math.max(1, Math.round(Theme.focusLine / Math.max(canvas.diagramScale, 0.1)))
            x: canvas.targetBounds.x - stroke
            y: canvas.targetBounds.y - stroke
            width: canvas.targetBounds.width + stroke * 2
            height: canvas.targetBounds.height + stroke * 2
            radius: canvas.targetBounds.width === canvas.targetBounds.height ? width / 2 : stroke * 2
            color: "transparent"
            border.color: Theme.textOnAccent
            border.width: stroke * 2

            Rectangle {
                anchors.fill: parent
                anchors.margins: Math.round(outline.stroke / 2)
                radius: outline.radius - anchors.margins
                color: "transparent"
                border.color: Theme.accent
                border.width: outline.stroke
            }

            Rectangle {
                id: pulse
                anchors.fill: parent
                anchors.margins: -outline.stroke
                radius: outline.radius + outline.stroke
                color: "transparent"
                border.color: Theme.accent
                border.width: outline.stroke
                opacity: 0.2
                visible: !Theme.reduceMotion

                SequentialAnimation {
                    running: pulse.visible
                    loops: Animation.Infinite
                    OpacityAnimator { target: pulse; from: 0.2; to: 0.85; duration: 850; easing.type: Easing.InOutSine }
                    OpacityAnimator { target: pulse; from: 0.85; to: 0.2; duration: 850; easing.type: Easing.InOutSine }
                }
            }
        }
    }
}
