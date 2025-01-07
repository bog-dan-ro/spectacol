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

import QtQuick

Canvas {
    id: canvas
    anchors.fill: parent
    property color backgroundColor: Qt.transparent
    property color strokeColor: "black"
    property color gamepadFillColor: "lightgray"

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

    function drawRoundButton(context, x, y, wh, color, borderColor, text)
    {
        context.save();
        context.fillStyle = color;
        context.strokeStyle = borderColor;
        context.lineWidth = 5;

        context.beginPath();
        context.ellipse(x - wh /2, y - wh /2, wh, wh);
        context.closePath();
        context.fill();
        context.stroke();

        context.lineWidth = 3;
        context.font = "1 30px sans-serif";
        context.textAlign = 'center';
        context.textBaseline = "middle";
        context.strokeText(text, x, y);
        context.restore();
    }

    function drawRectangleButton(context, x, y, w, h, color, borderColor, text)
    {
        context.save();
        context.fillStyle = color;
        context.strokeStyle = borderColor;
        context.lineWidth = 5;

        context.beginPath();
        context.roundedRect(x - w /2, y - h /2, w, h, context.lineWidth / 2, context.lineWidth / 2);
        context.closePath();
        context.fill();
        context.stroke();

        if (text !=='') {
            context.lineWidth = 2;
            context.font = "1 20px sans-serif";
            context.textAlign = 'center';
            context.textBaseline = "middle";
            context.strokeText(text, x, y);
        }
        context.restore();
    }

    function drawJoystick(context, x, y, wh, ax, ay)
    {
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
        context.scale(canvas.width/1000.0, canvas.height/500.0);
        context.fillStyle = backgroundColor;
        context.fillRect(0, 0, 1000, 500);

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
            drawRectangleButton(context, 190, 177, 50, 75, buttonArrowsColors["down"], buttonArrowsColors["border"], '');
        if (buttonDownPressed)
            drawRectangleButton(context, 190, 303, 50, 75, buttonArrowsColors["down"], buttonArrowsColors["border"], '');
        if (buttonLeftPressed)
            drawRectangleButton(context, 127, 240, 75, 50, buttonArrowsColors["down"], buttonArrowsColors["border"], '');
        if (buttonRightPressed)
            drawRectangleButton(context, 253, 240, 75, 50, buttonArrowsColors["down"], buttonArrowsColors["border"], '');

        drawRoundButton(context, 790, 180, 60, buttonYPressed ? buttonYColors["down"] : buttonYColors["up"], buttonYColors["border"], "Y");
        drawRoundButton(context, 790, 300, 60, buttonAPressed ? buttonAColors["down"] : buttonAColors["up"], buttonAColors["border"], "A");

        drawRoundButton(context, 730, 240, 60, buttonXPressed ? buttonXColors["down"] : buttonXColors["up"], buttonXColors["border"], "X");
        drawRoundButton(context, 850, 240, 60, buttonBPressed ? buttonBColors["down"] : buttonBColors["up"], buttonBColors["border"], "B");

        drawRectangleButton(context, 430, 180, 100, 30, buttonSelectPressed ? buttonSelectColors["down"] : buttonSelectColors["up"], buttonSelectColors["border"], "Select");
        drawRectangleButton(context, 570, 180, 100, 30, buttonStartPressed ? buttonStartColors["down"] : buttonStartColors["up"], buttonStartColors["border"], "Start");

        drawRectangleButton(context, 200, 70, 100, 30, buttonL1Pressed ? buttonL1Colors["down"] : buttonL1Colors["up"], buttonL1Colors["border"], "L1");
        drawRectangleButton(context, 200, 30, 100, 30, buttonL2Pressed ? buttonL2Colors["down"] : buttonL2Colors["up"], buttonL2Colors["border"], "L2");
        drawRectangleButton(context, 800, 70, 100, 30, buttonR1Pressed ? buttonR1Colors["down"] : buttonR1Colors["up"], buttonR1Colors["border"], "R1");
        drawRectangleButton(context, 800, 30, 100, 30, buttonR2Pressed ? buttonR2Colors["down"] : buttonR2Colors["up"], buttonR2Colors["border"], "R2");

        drawJoystick(context, 220, 290, 180, axisLeftX, axisLeftY);
        drawJoystick(context, 600, 290, 180, axisRightX, axisRightY);
    }
}
