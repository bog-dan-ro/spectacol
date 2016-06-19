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
import "private"

Item {
    id: onScreenKeyboard
    enabled: visible

    property bool gamepadMode: false
    anchors.topMargin: Screen.pixelDensity * 20
    anchors.leftMargin: Screen.pixelDensity * 2
    anchors.rightMargin: Screen.pixelDensity * 2

    onVisibleChanged: {
        fuse.processInputEvents = !visible;
        if (!visible) {
            if (_pressedKey) {
                fuse.keyRelease(_pressedKey);
                _pressedKey = 0;
            }

            if (capsPressed) {
                fuse.keyRelease(Qt.Key_Control);
                capsPressed = false;
            }

            if (symbolPressed) {
                fuse.keyRelease(Qt.Key_Shift);
                symbolPressed = false;
            }
        }
    }
    Keys.onPressed: {
        if (event.key === Qt.Key_Escape) {
            event.accept = true;
        } else if (event.key === Qt.Key_Return) {
            event.accept = true;
            pressCurrentKey();
        }
    }

    Keys.onReleased: {
        if (event.key === Qt.Key_Escape) {
            event.accept = true;
            onScreenKeyboard.visible = false;
        } else if (event.key === Qt.Key_Return) {
            event.accept = true;
            releaseCurrentKey();
        }
    }

    property bool capsPressed: false
    property bool symbolPressed: false
    property int _pressedKey: 0

    function pressCurrentKey()
    {
        _pressedKey = 0;
        var key = zx48Keyboard.get(grid.currentIndex);
        switch(key.code)
        {
        case Qt.Key_Control:
            capsPressed = !capsPressed;
            if (capsPressed)
                fuse.keyPress(Qt.Key_Control);
            else
                fuse.keyRelease(Qt.Key_Control);
            break;
        case Qt.Key_Shift:
            symbolPressed = !symbolPressed;
            if (symbolPressed)
                fuse.keyPress(Qt.Key_Shift);
            else
                fuse.keyRelease(Qt.Key_Shift);
            break;
        default:
            fuse.keyPress(key.code);
            if (gamepadMode)
                _pressedKey = key.code;
            break;
        }
    }

    function releaseCurrentKey()
    {
        var key = zx48Keyboard.get(grid.currentIndex);
        switch(key.code)
        {
        case Qt.Key_Control:
        case Qt.Key_Shift:
            break;
        default:
            if (gamepadMode)
                fuse.keyRelease(_pressedKey);
            else
                fuse.keyRelease(key.code);
            _pressedKey = 0;
            break;
        }
    }

    GamepadKeyNavigation {
        id: gamepad
        gamepad: Gamepad { deviceId: fuse.gamepadId }
        active: onScreenKeyboard.visible
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
    }

    ListModel {
        id: zx48Keyboard
        // numeric row
        ListElement{ code:Qt.Key_1; letter:"1"; keyword:"EDIT"; redsymbol:"!"; extgreen:"BLUE"; extred: "DEF FN"; graphics:""; buttonColor:"blue"}
        ListElement{ code:Qt.Key_2; letter:"2"; keyword:"CAPS LOOK"; redsymbol:"@"; extgreen:"RED"; extred: "FN"; graphics:""; buttonColor:"red"}
        ListElement{ code:Qt.Key_3; letter:"3"; keyword:"TRUE VIDEO"; redsymbol:"#"; extgreen:"MAGENTA"; extred: "LINE"; graphics:""; buttonColor:"magenta"}
        ListElement{ code:Qt.Key_4; letter:"4"; keyword:"INV. VIDEO"; redsymbol:"$"; extgreen:"GREEN"; extred: "OPEN #"; graphics:""; buttonColor:"green"}
        ListElement{ code:Qt.Key_5; letter:"5"; keyword:"\u2190"; redsymbol:"%"; extgreen:"CYAN"; extred: "CLOSE #"; graphics:""; buttonColor:"cyan"}
        ListElement{ code:Qt.Key_6; letter:"6"; keyword:"\u2193"; redsymbol:"&"; extgreen:"YELLOW"; extred: "MOVE"; graphics:""; buttonColor:"yellow"}
        ListElement{ code:Qt.Key_7; letter:"7"; keyword:"\u2191"; redsymbol:"'"; extgreen:"WHITE"; extred: "ERASE"; graphics:""; buttonColor:"white"}
        ListElement{ code:Qt.Key_8; letter:"8"; keyword:"\u2192"; redsymbol:"("; extgreen:""; extred: "POINT"; graphics:""}
        ListElement{ code:Qt.Key_9; letter:"9"; keyword:"GRAPHICS"; redsymbol:")"; extgreen:""; extred: "CAT"; graphics:""}
        ListElement{ code:Qt.Key_0; letter:"0"; keyword:"DELETE"; redsymbol:"_"; extgreen:"BLACK"; extred: "FORMAT"; graphics:""; buttonColor:"black"}

        // q row
        ListElement{ code:Qt.Key_Q; letter:"Q"; keyword:"PLOT"; redsymbol:"<="; extgreen:"SIN"; extred: "ASN"}
        ListElement{ code:Qt.Key_W; letter:"W"; keyword:"DRAW"; redsymbol:"<>"; extgreen:"COS"; extred: "ACS"}
        ListElement{ code:Qt.Key_E; letter:"E"; keyword:"REM"; redsymbol:">="; extgreen:"TAN"; extred: "ATN"}
        ListElement{ code:Qt.Key_R; letter:"R"; keyword:"RUN"; redsymbol:"<"; extgreen:"INT"; extred: "VERIFY"}
        ListElement{ code:Qt.Key_T; letter:"T"; keyword:"RAND"; redsymbol:">"; extgreen:"RND"; extred: "MERGE"}
        ListElement{ code:Qt.Key_Y; letter:"Y"; keyword:"RETURN"; redsymbol:"AND"; extgreen:"STR$"; extred: "["}
        ListElement{ code:Qt.Key_U; letter:"U"; keyword:"IF"; redsymbol:"OR"; extgreen:"CHR$"; extred: "]"}
        ListElement{ code:Qt.Key_I; letter:"I"; keyword:"INPUT"; redsymbol:"AT"; extgreen:"CODE"; extred: "IN"}
        ListElement{ code:Qt.Key_O; letter:"O"; keyword:"POKE"; redsymbol:";"; extgreen:"PEEK"; extred: "OUT"}
        ListElement{ code:Qt.Key_P; letter:"P"; keyword:"PRINT"; redsymbol:"\""; extgreen:"TAB"; extred: "\u00A9"}

        // a row
        ListElement{ code:Qt.Key_A; letter:"A"; keyword:"NEW"; redsymbol:"STOP"; extgreen:"READ"; extred: "~"}
        ListElement{ code:Qt.Key_S; letter:"S"; keyword:"SAVE"; redsymbol:"NOT"; extgreen:"RESTORE"; extred: "|"}
        ListElement{ code:Qt.Key_D; letter:"D"; keyword:"DIM"; redsymbol:"STEP"; extgreen:"DATA"; extred: "\\"}
        ListElement{ code:Qt.Key_F; letter:"F"; keyword:"FOR"; redsymbol:"TO"; extgreen:"SGN"; extred: "{"}
        ListElement{ code:Qt.Key_G; letter:"G"; keyword:"GOTO"; redsymbol:"THEN"; extgreen:"ABS"; extred: "}"}
        ListElement{ code:Qt.Key_H; letter:"H"; keyword:"GOSUB"; redsymbol:"\u2191"; extgreen:"SQR"; extred: "CIRCLE"}
        ListElement{ code:Qt.Key_J; letter:"J"; keyword:"LOAD"; redsymbol:"-"; extgreen:"VAL"; extred: "VAL$"}
        ListElement{ code:Qt.Key_K; letter:"K"; keyword:"LIST"; redsymbol:"+"; extgreen:"LEN"; extred: "SCREEN$"}
        ListElement{ code:Qt.Key_L; letter:"L"; keyword:"LET"; redsymbol:"="; extgreen:"USR"; extred: "ATTR"}
        ListElement{ code:Qt.Key_Enter; letter:"ENTER"}

        // z row
        ListElement{ code:Qt.Key_Shift; letter:"CAPS SHIFT"}
        ListElement{ code:Qt.Key_Z; letter:"Z"; keyword:"COPY"; redsymbol:":"; extgreen:"LN"; extred: "BEEP"}
        ListElement{ code:Qt.Key_X; letter:"X"; keyword:"CLEAR"; redsymbol:"\u00A3"; extgreen:"EXP"; extred: "INK"}
        ListElement{ code:Qt.Key_C; letter:"C"; keyword:"CONT"; redsymbol:"?"; extgreen:"L PRINT"; extred: "PAPER"}
        ListElement{ code:Qt.Key_V; letter:"V"; keyword:"CLS"; redsymbol:"/"; extgreen:"L LIST"; extred: "FLASH"}
        ListElement{ code:Qt.Key_B; letter:"B"; keyword:"BORDER"; redsymbol:"*"; extgreen:"BIN"; extred: "BRIGHT"}
        ListElement{ code:Qt.Key_N; letter:"N"; keyword:"NEXT"; redsymbol:","; extgreen:"INKEY$"; extred: "OVER"}
        ListElement{ code:Qt.Key_M; letter:"M"; keyword:"PAUSE"; redsymbol:"."; extgreen:"PI"; extred: "INVERSE"}
        ListElement{ code:Qt.Key_Control; letter:"SYMBOL SHIFT"; buttonColor:"red"}
        ListElement{ code:Qt.Key_Space; letter:"BREAK SPACE"}
    }

    Component {
        id: highlight
        Rectangle {
            id: rect
            visible: fuse.gamepadId != -1 && gamepadMode
            width: grid.cellWidth; height: grid.cellHeight
            x: grid.currentItem.x
            y: grid.currentItem.y
            Behavior on x { SpringAnimation { spring: 3; damping: 0.2 } }
            Behavior on y { SpringAnimation { spring: 3; damping: 0.2 } }
            SequentialAnimation on color {
                id: animation
                running: rect.visible
                loops: Animation.Infinite
                ColorAnimation { from: "blue"; to: "magenta"; duration: 500 }
                ColorAnimation { from: "magenta"; to: "blue"; duration: 500 }
            }

        }
    }

    GridView {
        id: grid
        highlight: highlight
        focus: onScreenKeyboard.visible
        anchors.fill: parent
        anchors.topMargin: mainScreen.height > mainScreen.width ? mainScreen.height / 2 : 0;
        cellWidth: (width - (mainScreen.height > mainScreen.width ? 0 : (4 * Screen.pixelDensity) / TextSizes.scaleImage)) / 10
        cellHeight: cellWidth
        model: zx48Keyboard
        currentIndex: 24
        onCurrentIndexChanged: {
            if (_pressedKey)
                fuse.keyRelease(_pressedKey);
        }

        interactive: fuse.gamepadId != -1 && gamepadMode

        delegate: Rectangle {
            id: rec
            property bool pressed: true
            width: grid.cellWidth - (2 * Screen.pixelDensity) / TextSizes.scaleImage
            height: width
            radius: ((mainScreen.height > mainScreen.width ? 1 : 2) * Screen.pixelDensity) / TextSizes.scaleImage
            color: (code == Qt.Key_Control && capsPressed) ||
                   (code == Qt.Key_Shift && symbolPressed) ? "blue"
                                                           : Qt.rgba(0.5, 0.5, 0.5, 0.75);
            Text {
                id: letterItem
                anchors.fill: parent
                anchors.leftMargin: parent.radius
                anchors.rightMargin: parent.radius
                anchors.topMargin: fuseSettings.full48kOSK && keyword ? parent.height / 4 : parent.radius
                font.family: 'Monospace'
                font.bold: true
                font.pixelSize: parent.height / 4
                horizontalAlignment: fuseSettings.full48kOSK && keyword ? Text.AlignLeft : Text.AlignHCenter
                verticalAlignment: fuseSettings.full48kOSK && keyword ? Text.AlignTop : Text.AlignVCenter
                style: Text.Outline
                styleColor: "black"
                color: !keyword && buttonColor ? buttonColor : "white"
                width: parent.width
                wrapMode: Text.WordWrap
                text: letter
            }

            Text {
                visible: fuseSettings.full48kOSK
                anchors.left: parent.left
                anchors.margins: parent.radius
                style: Text.Outline
                styleColor: buttonColor && buttonColor == "black" ? "white" : "black"
                color: buttonColor ? buttonColor : "green"
                font.family: 'Monospace'
                font.bold: true
                font.pixelSize: parent.height / 6
                text: extgreen ? extgreen : ""
            }

            Text {
                visible: fuseSettings.full48kOSK
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.rightMargin: parent.radius
                anchors.topMargin: parent.height / 6
                style: Text.Outline
                styleColor: "black"
                color: "red"
                font.family: 'Monospace'
                font.bold: true
                font.pixelSize: parent.height / 6
                text: redsymbol ? redsymbol : ""
            }

            Text {
                visible: fuseSettings.full48kOSK
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.topMargin: parent.height / 6 * 3
                style: Text.Outline
                styleColor: "black"
                color: "white"
                font.family: 'Monospace'
                font.bold: true
                font.pixelSize: keyword && keyword.length > 1 ? parent.height / 6 : parent.height / 4
                text: keyword ? keyword : ""
            }

            Text {
                visible: fuseSettings.full48kOSK
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.leftMargin: parent.radius
                anchors.bottomMargin: parent.radius
                style: Text.Outline
                styleColor: "black"
                color: "red"
                font.family: 'Monospace'
                font.bold: true
                font.pixelSize: parent.height / 6
                text: extred ? extred : ""
            }

            MultiPointTouchArea {
                anchors.fill: parent
                touchPoints: TouchPoint {
                    onPressedChanged: pressed ? fuse.keyPress(code) : fuse.keyRelease(code)
                }
            }
        }
    }
}
