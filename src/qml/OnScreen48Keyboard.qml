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
import QtQuick.Window
import QtGamepadLegacy
import Spectacol
import "private"

// @scope main.qml

Item {
    id: onScreenKeyboard
    enabled: visible

    property bool textInputMode: false
    readonly property bool portrait: Window.window ? Window.window.width < Window.window.height : width < height
    signal textKeyPressed(int key, string text)
    readonly property bool cobraMode: !textInputMode && ZxEmulator.settings.currentMachine.startsWith("CoBra 1")
    readonly property int cobraFlag: 0x20000000
    readonly property int cobraControlKey: Qt.Key_Control | cobraFlag
    readonly property bool nextMode: !textInputMode && ZxEmulator.settings.currentMachine === "ZX Spectrum Next"
    readonly property int nextFlag: 0x10000000
    readonly property int columns: cobraMode ? 12 : nextMode ? 14 : 10
    readonly property bool cobraCtrlPressed: input.heldKeys.indexOf(cobraControlKey) >= 0
    readonly property bool capsPressed: input.heldKeys.indexOf(Qt.Key_Control) >= 0
    readonly property bool symbolPressed: input.heldKeys.indexOf(Qt.Key_Shift) >= 0
    property bool gamepadMode: false
    property bool stickyCSSS: textInputMode || gamepadMode || stickyCSSSButton.checked
    onCobraModeChanged: releasePressedKeys()
    onNextModeChanged: releasePressedKeys()
    onGamepadModeChanged: releasePressedKeys()
    Component.onDestruction: releasePressedKeys()

    KeyboardInput {
        id: input
        sticky: onScreenKeyboard.stickyCSSS
        extraModifier: onScreenKeyboard.cobraControlKey
        onKeyChanged: (key, pressed) => {
            if (onScreenKeyboard.textInputMode) {
                if (pressed && !input.isModifier(key))
                    onScreenKeyboard.typeTextKey(key);
                return;
            }
            if (key & nextFlag)
                ZxEmulator.setNextKeyState(key & ~nextFlag, pressed);
            else if (key & cobraFlag)
                ZxEmulator.setCobraKeyState(key & ~cobraFlag, pressed);
            else if (pressed)
                ZxEmulator.keyPress(key);
            else
                ZxEmulator.keyRelease(key);
        }
    }

    Connections {
        target: ZxEmulator
        function onPausedChanged() {
            if (ZxEmulator.paused)
                onScreenKeyboard.releasePressedKeys();
        }
    }
    Connections {
        target: onScreenKeyboard.Window.window
        function onActiveChanged() {
            const window = onScreenKeyboard.Window.window;
            if (!window || !window.active)
                onScreenKeyboard.releasePressedKeys();
        }
    }

    anchors {
        bottomMargin: textInputMode ? 0 : Screen.pixelDensity * (onScreenKeyboard.portrait ? 10 : 4)
        leftMargin: textInputMode || onScreenKeyboard.portrait ? 0 : Screen.pixelDensity * 2
        rightMargin: textInputMode || onScreenKeyboard.portrait ? 0 : Screen.pixelDensity * 2
    }

    function releasePressedKeys(): void {
        input.clear();
    }

    readonly property var textSymbols: ({
        "1":"!", "2":"@", "3":"#", "4":"$", "5":"%", "6":"&", "7":"'", "8":"(", "9":")", "0":"_",
        "Q":"<=", "W":"<>", "E":">=", "R":"<", "T":">", "Y":"[", "U":"]", "I":"`", "O":";", "P":"\"",
        "A":"~", "S":"|", "D":"\\", "F":"{", "G":"}", "H":"^", "J":"-", "K":"+", "L":"=",
        "Z":":", "X":"\u00a3", "C":"?", "V":"/", "B":"*", "N":",", "M":"."
    })
    function textLabel(key: int): string {
        if (key === Qt.Key_Shift) return qsTr("Caps Shift");
        if (key === Qt.Key_Control) return qsTr("Symbols");
        if (key === Qt.Key_Space) return qsTr("Space");
        if (key === Qt.Key_Enter) return qsTr("Enter");
        const caps = input.heldKeys.includes(Qt.Key_Shift);
        if (caps && key === Qt.Key_0) return "\u232b";
        const letter = String.fromCharCode(key);
        if (input.heldKeys.includes(Qt.Key_Control)) return textSymbols[letter] || letter;
        return caps ? letter : letter.toLowerCase();
    }
    function typeTextKey(key: int): void {
        const caps = input.heldKeys.includes(Qt.Key_Shift);
        if (key === Qt.Key_Enter)
            textKeyPressed(Qt.Key_Return, "");
        else if (key === Qt.Key_Space)
            textKeyPressed(0, " ");
        else if (caps && key === Qt.Key_0)
            textKeyPressed(Qt.Key_Backspace, "");
        else if (caps && key >= Qt.Key_5 && key <= Qt.Key_8) {
            const arrows = [Qt.Key_Left, Qt.Key_Down, Qt.Key_Up, Qt.Key_Right];
            textKeyPressed(arrows[key - Qt.Key_5], "");
        } else
            textKeyPressed(0, textLabel(key));
    }
    function navigateTextInput(key: int, pressed: bool): void {
        if (key === Qt.Key_Return) {
            if (pressed)
                input.press("navigation", grid.model.get(grid.currentIndex).code);
            else
                input.release("navigation", false);
        } else if (pressed) {
            switch (key) {
            case Qt.Key_Left: grid.moveCurrentIndexLeft(); break;
            case Qt.Key_Right: grid.moveCurrentIndexRight(); break;
            case Qt.Key_Up: grid.moveCurrentIndexUp(); break;
            case Qt.Key_Down: grid.moveCurrentIndexDown(); break;
            }
        }
    }

    onVisibleChanged: {
        if (textInputMode) {
            if (!visible) releasePressedKeys();
            return;
        }
        ZxEmulator.processInputEvents = !visible;
        if (visible) {
            grid.currentIndex = columns * 2;
            grid.forceActiveFocus(Qt.OtherFocusReason);
        } else {
            releasePressedKeys();
        }
    }
    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Escape) {
            event.accepted = true;
        } else if (event.key === Qt.Key_Return) {
            event.accepted = true;
            if (!event.isAutoRepeat && !ZxEmulator.paused)
                input.press("navigation", grid.model.get(grid.currentIndex).code);
        }
    }
    Keys.onReleased: (event) => {
        if (event.key === Qt.Key_Escape) {
            event.accepted = true;
            onScreenKeyboard.visible = false;
        } else if (event.key === Qt.Key_Return) {
            event.accepted = true;
            if (!event.isAutoRepeat)
                input.release("navigation", false);
        }
    }

    Timer {
        // A short CS+SS chord switches the ROM into extended mode. Other
        // owners of a modifier keep it held when these latches expire.
        running: !onScreenKeyboard.textInputMode && input.latchedKeys.indexOf(Qt.Key_Control) >= 0 &&
                 input.latchedKeys.indexOf(Qt.Key_Shift) >= 0
        interval: 200
        onTriggered: {
            input.releaseLatch(Qt.Key_Control);
            input.releaseLatch(Qt.Key_Shift);
        }
    }

    GamepadKeyNavigation {
        id: gamepad
        gamepad: Gamepad { deviceId: ZxEmulator.gamepadId }
        active: onScreenKeyboard.visible && !onScreenKeyboard.textInputMode
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

    ListModel { id: cobraKeyboard }
    ListModel { id: nextKeyboard }

    Component.onCompleted: {
        if (textInputMode) {
            grid.currentIndex = 20;
            return;
        }
        const extraCodes = [Qt.Key_F1, Qt.Key_F2, Qt.Key_F3, Qt.Key_F4,
                            Qt.Key_Escape, Qt.Key_Enter, Qt.Key_Control, Qt.Key_ScrollLock];
        const extraLabels = ["F1", "F2", "F3", "F4", "ESC", "LINE FEED", "CTRL", "NO SCROLL"];
        const nextCodes = [14,9,11,12, 10,8,13,15, 7,6,3,5, 4,1,2,0];
        const nextLabels = ["EDIT","CAPS LOCK","TRUE VIDEO","INV VIDEO", "GRAPH","EXTEND","BREAK","DELETE",
                            ";", "\"", "\u2191", ",", ".", "\u2190", "\u2193", "\u2192"];
        for (let row = 0; row < 4; ++row) {
            for (let column = 0; column < 10; ++column) {
                const original = zx48Keyboard.get(row * 10 + column);
                nextKeyboard.append({code: original.code, letter: original.letter,
                    keyword: original.keyword || "", redsymbol: original.redsymbol || "",
                    extgreen: original.extgreen || "", extred: original.extred || "",
                    buttonColor: original.buttonColor || ""});
                cobraKeyboard.append({code: original.code, letter: original.letter,
                    keyword: original.keyword || "", redsymbol: original.redsymbol || "",
                    extgreen: original.extgreen || "", extred: original.extred || "",
                    buttonColor: original.buttonColor || ""});
            }
            for (let column = 0; column < 4; ++column) {
                const extra = row * 4 + column;
                nextKeyboard.append({code: nextCodes[extra] | nextFlag, letter: nextLabels[extra],
                    keyword: "", redsymbol: "", extgreen: "", extred: "", buttonColor: ""});
            }
            for (let column = 0; column < 2; ++column) {
                const extra = row * 2 + column;
                cobraKeyboard.append({code: extraCodes[extra] | cobraFlag, letter: extraLabels[extra],
                    keyword: "", redsymbol: "", extgreen: "", extred: "", buttonColor: ""});
            }
        }
    }

    Component {
        id: highlightComponent
        Rectangle {
            id: rect
            visible: gamepadMode
            width: grid.cellWidth; height: grid.cellHeight
            x: grid.currentItem ? grid.currentItem.x : 0
            y: grid.currentItem ? grid.currentItem.y : 0
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
        highlight: highlightComponent
        focus: onScreenKeyboard.visible && !onScreenKeyboard.textInputMode
        anchors {
            horizontalCenter: parent.horizontalCenter
            bottom: parent.bottom
        }
        cellWidth: (onScreenKeyboard.portrait || gamepadMode || textInputMode) ? Math.max(1, Math.floor(parent.width / columns)) : Math.min(parent.width / columns, Screen.pixelDensity * 9)
        cellHeight: onScreenKeyboard.portrait && !textInputMode ? Screen.pixelDensity * 9 : cellWidth
        width: Math.min(cellWidth * columns + ((onScreenKeyboard.portrait || gamepadMode || textInputMode) ? 0 : 8 * Screen.pixelDensity), parent.width)
        height: cellHeight * 4 + ((onScreenKeyboard.portrait || gamepadMode || textInputMode) ? 0 : 2 * Screen.pixelDensity)
        model: cobraMode ? cobraKeyboard : nextMode ? nextKeyboard : zx48Keyboard
        currentIndex: 24
        onCurrentIndexChanged: input.release("navigation", false)
        onActiveFocusChanged: if (!activeFocus) input.release("navigation", true)
        interactive: false
        keyNavigationEnabled: gamepadMode

        // One area owns the whole keyboard, so every point has a stable ID.
        // Sliding off a key keeps its original hold until release/cancellation.
        MultiPointTouchArea {
            anchors.fill: parent
            minimumTouchPoints: 1
            maximumTouchPoints: 10
            z: 2
            onPressed: (points) => {
                if (ZxEmulator.paused && !onScreenKeyboard.textInputMode)
                    return;
                for (const point of points) {
                    // indexAt takes content coordinates; the view's origin can
                    // shift when the layout changes size or machine.
                    const index = grid.indexAt(point.x + grid.contentX, point.y + grid.contentY);
                    const item = grid.itemAtIndex(index);
                    if (item && item.contains(item.mapFromItem(grid, point.x, point.y)))
                        input.press("touch:" + point.pointId, grid.model.get(index).code);
                }
            }
            onReleased: (points) => {
                for (const point of points)
                    input.release("touch:" + point.pointId, false);
            }
            onCanceled: (points) => {
                for (const point of points)
                    input.release("touch:" + point.pointId, true);
            }
        }

        delegate: Rectangle {
            id: rec
            width: grid.cellWidth - (Screen.pixelDensity / TextSizes.scaleImage)
            height: grid.cellHeight - (Screen.pixelDensity / TextSizes.scaleImage)
            radius: ((onScreenKeyboard.portrait ? 1 : 2) * Screen.pixelDensity) / TextSizes.scaleImage
            color: (code === cobraControlKey && cobraCtrlPressed) ||
                   (code === Qt.Key_Control && capsPressed) ||
                   (code === Qt.Key_Shift && symbolPressed) ? "blue"
                                                           : Qt.rgba(0.5, 0.5, 0.5, 0.75);
            Text {
                id: letterItem
                anchors.fill: parent
                anchors.leftMargin: parent.radius
                anchors.rightMargin: parent.radius
                anchors.topMargin: !onScreenKeyboard.textInputMode && ZxEmulator.settings.full48kOSK && keyword ? parent.height / 4 : parent.radius
                font.family: 'Monospace'
                font.bold: true
                font.pixelSize: grid.cellWidth / 4.5
                horizontalAlignment: !onScreenKeyboard.textInputMode && ZxEmulator.settings.full48kOSK && keyword ? Text.AlignLeft : Text.AlignHCenter
                verticalAlignment: !onScreenKeyboard.textInputMode && ZxEmulator.settings.full48kOSK && keyword ? Text.AlignTop : Text.AlignVCenter
                style: Text.Outline
                styleColor: "black"
                color: !keyword && buttonColor ? buttonColor : "white"
                width: parent.width
                wrapMode: Text.WordWrap
                text: onScreenKeyboard.textInputMode ? onScreenKeyboard.textLabel(code) : letter
            }

            Text {
                visible: !onScreenKeyboard.textInputMode && ZxEmulator.settings.full48kOSK
                anchors.left: parent.left
                anchors.margins: parent.radius
                style: Text.Outline
                styleColor: buttonColor === "black" ? "white" : "black"
                color: buttonColor ? buttonColor : "green"
                font.family: 'Monospace'
                font.bold: true
                font.pixelSize: grid.cellWidth / 6
                text: extgreen ? extgreen : ""
            }

            Text {
                visible: !onScreenKeyboard.textInputMode && ZxEmulator.settings.full48kOSK
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
                visible: !onScreenKeyboard.textInputMode && ZxEmulator.settings.full48kOSK
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.topMargin: parent.height / 6 * 3
                style: Text.Outline
                styleColor: "black"
                color: "white"
                font.family: 'Monospace'
                font.bold: true
                font.pixelSize: keyword && keyword.length > 1 ? grid.cellWidth / 6 : grid.cellHeight / 4
                text: keyword ? keyword : ""
            }

            Text {
                visible: !onScreenKeyboard.textInputMode && ZxEmulator.settings.full48kOSK
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

        }
    }
}
