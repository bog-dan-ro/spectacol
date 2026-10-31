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
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import QtQuick.Window
import QtGamepadLegacy
import Spectacol
import "private"

ApplicationWindow {
    id: mainScreen
    visible: true

    width: 320 * 3
    height: 256 * 3

    title: qsTr("Spectacol")

    property bool portrait: width < height

    MessageDialog {
        id: quitDialog
        title: "Spectacol"
        text: "Quit ?"
        buttons: MessageDialog.Yes | MessageDialog.No
        onAccepted: ZxEmulator.quit()
    }

    Drawer {
        id: menuBar
        focus: false
        dragMargin: ZxEmulator.settings.swipe4menu ? Qt.styleHints.startDragDistance : 0

        width: 17 * (TextSizes.smallScreen ? TextSizes.scale16 : TextSizes.scale20)
        height: mainScreen.height
        MenuView {
            id: menuView
            anchors.fill: parent
            rootMenu: ZxMenu {}
            transform: Translate {
                x: (menuBar.position - 1.0) * menuView.width
            }
        }
        property bool paused: false
        onPausedChanged: ZxEmulator.paused = paused
        // position changes on every frame of the animation and of a drag,
        // rebuild the menu only when the drawer starts to open
        property bool wasClosed: true
        onPositionChanged: {
            if (wasClosed && position > 0) {
                wasClosed = false;
                menuView.reset();
            }
            if (position === 1) {
                onScreenCursorJoystick.visible = false;
                onScreen48Keyboard.visible = false;
                pageLoader.source = "";
                menuView.useGamepad = true;
                if (!paused)
                    paused = true;
            }

            if (position === 0) {
                wasClosed = true;
                menuView.useGamepad = false;
                if (paused)
                    paused = false;
            }
        }
    }

    Rectangle {
        id: mainRectangle
        anchors.fill: parent
        color: "black"
        focus: true

        Keys.onPressed: (event)=> {
            switch (event.key) {
            case Qt.Key_F1:
            case Qt.Key_Search:
            case Qt.Key_Guide:
            case Qt.Key_Menu:
                if (menuBar.position) {
                    menuBar.close();
                } else {
                    if (pageLoader.status !== Loader.Null) {
                        pageLoader.source = "";
                    } else {
                        menuBar.open();
                    }
                }
                event.accepted = true;
                break;

            case Qt.Key_F4:
                pageLoader.source = "FileBrowserPage.qml";
                event.accepted = true;
                break;

            case Qt.Key_F7:
                toggleOnScreenControls(ZxEmulator.Keyboard48K, true);
                break;

            case Qt.Key_F10:
                quitDialog.open();
                event.accepted = true;
                break;

            case Qt.Key_F11:
                zxScreen.fullScreen = !zxScreen.fullScreen;
                event.accepted = true;
                break;

            case Qt.Key_Down:
                if (menuBar.position) {
                    menuView.incrementCurrentIndex();
                    event.accepted = true;
                }
                break;

            case Qt.Key_Up:
                if (menuBar.position) {
                    menuView.decrementCurrentIndex();
                    event.accepted = true;
                }
                break;

            case Qt.Key_Return:
            case Qt.Key_Enter:
                if (menuBar.position)
                    event.accepted = true;
                break;
            case Qt.Key_Escape:
                if (menuBar.position) {
                    menuView.popMenu();
                    event.accepted = true;
                } else {
                    if (pageLoader.status !== Loader.Null) {
                        pageLoader.source = "";
                        event.accepted = true;
                    }
                }
                break;
            case Qt.Key_Back:
                if (menuBar.position) {
                    menuView.popMenu();
                    event.accepted = true;
                } else {
                    if (pageLoader.status !== Loader.Null) {
                        pageLoader.source = "";
                        event.accepted = true;
                    } else {
                        if (ZxEmulator.settings.hasStartButton || ZxEmulator.touchscreen)
                            quitDialog.open();
                        else
                            menuBar.open();
                        event.accepted = true;
                    }
                }
                break;
            }
            if (!event.accepted && ZxEmulator.processInputEvents) {
                ZxEmulator.keyPress(event.key, event.modifiers, event.isAutoRepeat, false);
                event.accepted = true;
            }
        }

        Keys.onReleased: (event)=> {
            switch (event.key) {
            case Qt.Key_Return:
            case Qt.Key_Enter:
                if (menuBar.position) {
                    menuView.openSelectedMenu();
                    event.accepted = true;
                }
                break;
            }
            if (!event.accepted && ZxEmulator.processInputEvents) {
                ZxEmulator.keyRelease(event.key, event.modifiers, event.isAutoRepeat, false);
                event.accepted = true;
            }
        }

        Item {
            anchors.fill: parent
            anchors.leftMargin: (ZxEmulator.settings.leftMargin && mainScreen.height < mainScreen.width) ? 10 * Screen.pixelDensity : 0

            ZxScreen {
                id: zxScreen
                anchors.fill: parent
                mouseGrabSuspended: pageLoader.status !== Loader.Null || menuBar.position > 0 || !ZxEmulator.processInputEvents

                onFullScreenChanged: mainScreen.visibility = fullScreen ? Window.FullScreen : Window.AutomaticVisibility;

                Row {
                    anchors.margins: Screen.pixelDensity * 2
                    anchors.bottom: parent.bottom
                    anchors.right: parent.right
                    spacing: Screen.pixelDensity * 5

                    Image {
                        id: cassetteIcon
                        visible: false
                        height: Screen.pixelDensity * 10
                        width: Screen.pixelDensity * 13
                        sourceSize: Qt.size(width, height)
                        NumberAnimation on opacity {
                            id: cassetteIconHideAnimation
                            running: false
                            to: 0
                            duration: 2000
                            onStopped: {cassetteIcon.visible = false; cassetteIcon.opacity = 1}
                        }

                        source: "qrc:/images/cassette-tape.svg"
                    }

                    Image {
                        id: diskIcon
                        visible: false
                        height: Screen.pixelDensity * 13
                        width: Screen.pixelDensity * 13
                        sourceSize: Qt.size(width, height)
                        NumberAnimation on opacity {
                            id: diskIconHideAnimation
                            running: false
                            to: 0
                            duration: 2000
                            onStopped: {diskIcon.visible = false; diskIcon.opacity = 1}
                        }

                        source: "qrc:/images/floppy.svg"
                    }
                }
            }
        }

        MouseArea {
            anchors.fill: parent
            // let the clicks reach the ZX screen (Kempston mouse) when no page is open
            enabled: pageLoader.status !== Loader.Null
            onClicked: pageLoader.source = "";
        }

        Grid {
            id: screenButtonsGrid
            anchors.fill: parent
            anchors.topMargin: mainScreen.portrait ? zxScreen.implicitHeight : 0;
            visible: ZxEmulator.touchscreen && ZxEmulator.showControlsIcons
            horizontalItemAlignment: Grid.AlignHCenter
            verticalItemAlignment: Grid.AlignVCenter
            columns: mainScreen.portrait ? 6 : 1
            rows: mainScreen.portrait ? 1 : 6
            columnSpacing: Screen.pixelDensity
            rowSpacing: Screen.pixelDensity

            property real buttonsOpacity: (onScreenCursorJoystick.visible || onScreen48Keyboard.visible) ? 0.25 : 1.0
            Button {
                text: "="
                onClicked: {
                    if (menuBar.position)
                        menuBar.close();
                    else
                        menuBar.open();
                }
            }
            Image {
                opacity: parent.buttonsOpacity
                height: Screen.pixelDensity * 5
                width: Screen.pixelDensity * 7
                sourceSize: Qt.size(width, height)
                source: "qrc:/images/keyboard-icon.svg"
                MouseArea {
                    anchors.fill: parent
                    onClicked: toggleOnScreenControls(ZxEmulator.Keyboard48K, false);
                }
            }

            RoundButton {
                id: stickyCSSSButton
                radius: 7
                checkable: true
                checked: false
                text: "Sticky CS/SS"
            }

            Image {
                opacity: parent.buttonsOpacity
                height: Screen.pixelDensity * 5
                width: Screen.pixelDensity * 7
                sourceSize: Qt.size(width, height)
                source: "qrc:/images/controller-icon.svg"
                MouseArea {
                    anchors.fill: parent
                    onClicked: toggleOnScreenControls(ZxEmulator.CursorJoystick, false)
                }
            }
            Image {
                opacity: parent.buttonsOpacity
                height: Screen.pixelDensity * 7
                width: Screen.pixelDensity * 7
                sourceSize: Qt.size(width, height)
                source: "qrc:/images/fastforward.svg"
                MouseArea {
                    anchors.fill: parent
                    onPressed: ZxEmulator.speedup()
                    onReleased: ZxEmulator.slowdown()
                    onCanceled: ZxEmulator.slowdown()
                }
            }
            Image {
                opacity: parent.buttonsOpacity
                height: Screen.pixelDensity * 7
                width: Screen.pixelDensity * 7
                sourceSize: Qt.size(width, height)
                source: ZxEmulator.paused ? "qrc:/images/play.svg" : "qrc:/images/pause.svg"
                MouseArea {
                    anchors.fill: parent
                    onClicked: ZxEmulator.togglePaused()
                }
            }
        }

        Loader {
            id: onScreenCursorJoystick
            anchors.fill: parent
            visible: false
            focus: visible
            enabled: visible
            z: parent.z + 10
            onVisibleChanged: if (!visible) mainRectangle.forceActiveFocus(Qt.TabFocusReason);
        }

        OnScreen48Keyboard {
            id: onScreen48Keyboard
            visible: false
            enabled: visible
            anchors.fill: parent
            z: parent.z + 11
            onVisibleChanged: if (!visible) mainRectangle.forceActiveFocus(Qt.TabFocusReason);
        }

        Loader {
            z: parent.z + 20
            id: pageLoader
            anchors.fill: parent
            visible: status === Loader.Ready
            // Every page pauses the emulation while it is open, unless it says otherwise
            readonly property bool pausing: status === Loader.Ready && item !== null && item.pausesEmulation !== false
            onPausingChanged: ZxEmulator.paused = pausing
            focus: visible
            enabled: visible
            onVisibleChanged: {
                if (visible) {
                    // otherwise their gamepad navigation stays active under the page,
                    // and they don't get the focus back when the page closes
                    onScreenCursorJoystick.visible = false;
                    onScreen48Keyboard.visible = false;
                } else {
                    mainRectangle.forceActiveFocus(Qt.TabFocusReason);
                }
            }
        }

        MessagePage {
            id: messagePage
            z: 100
        }
    }

    function toggleOnScreenControls(type, gamepad) {
        if (pageLoader.visible)
            return;
        switch (type) {
        case ZxEmulator.CursorJoystick:
            if (!onScreenCursorJoystick.visible) {
                if (onScreen48Keyboard.visible)
                    onScreen48Keyboard.visible = false;
                // switch only while hidden: the touch joystick has no gamepad navigation
                onScreenCursorJoystick.source = gamepad ? "OnScreenCursorJoystick.qml" : "OnScreenJoystick.qml";
            }
            onScreenCursorJoystick.visible = !onScreenCursorJoystick.visible;
            break;

        case ZxEmulator.Keyboard48K:
            if (!onScreen48Keyboard.visible && onScreenCursorJoystick.visible)
                onScreenCursorJoystick.visible = false;
            onScreen48Keyboard.gamepadMode = gamepad;
            onScreen48Keyboard.visible = !onScreen48Keyboard.visible;
            break;
        }
    }

    Connections {
        target: NextSdCard
        function onDownloadRequested() {
            menuBar.close();
            pageLoader.source = "NextSdCardPage.qml";
        }
    }

    Connections {
        target: ZxEmulator

        function onError (level, message) { messagePage.showMessage(level, message); }

        function onShowDebugger() { pageLoader.source = "DebuggerPage.qml"; }
        function onHideDebugger() { pageLoader.source = ""; }

        function onConfigureJoystick() { pageLoader.source = "Joysticks.qml"; }

        function onShowMenu() { menuBar.open(); }
        function onHideMenu() { menuBar.close(); }
        function onToggleOnScreenControls(type) { toggleOnScreenControls(type, true); }
        function onShowWelcome() {
            ZxEmulator.settings.swipe4menu = !TextSizes.smallScreen;
            ZxEmulator.settings.leftMargin = TextSizes.smallScreen;
            pageLoader.source = "AboutPage.qml";
        }

        function onUiIconUpdate(item, state) {
            switch (item) {
            case ZxEmulator.Disk:
                if (cassetteIcon.visible)
                    cassetteIconHideAnimation.start();
                if (state === ZxEmulator.Active) {
                    diskIconHideAnimation.stop();
                    diskIcon.visible = true
                } else if (diskIcon.visible) {
                    diskIconHideAnimation.start();
                }
                break;
            case ZxEmulator.Microdrive:
            case ZxEmulator.Tape:
                if (diskIcon.visible)
                    diskIconHideAnimation.start();
                if (state === ZxEmulator.Active) {
                    cassetteIconHideAnimation.stop();
                    cassetteIcon.visible = true
                } else if (cassetteIcon.visible) {
                    cassetteIconHideAnimation.start();
                }
                break;
            case ZxEmulator.Mouse:
                break;
            case ZxEmulator.Paused:
                break;
            }
        }

    }

    Component.onCompleted: {
        // Check gamepads status
        if (ZxEmulator.gamepadId === -1) {
            var len = GamepadManager.connectedGamepads.length;
            if (len > 0) {
                for (var i = 0; i < len; ++i) {
                    var id = GamepadManager.connectedGamepads[i];
                    if (!GamepadManager.isConfigurationNeeded(id)) {
                        ZxEmulator.gamepadId = id;
                        break;
                    }
                }

                if (ZxEmulator.gamepadId === -1) {
                    ZxEmulator.gamepadId = GamepadManager.connectedGamepads[0];
                    pageLoader.source = "CalibrateGamepad.qml";
                }
            } else {
                if (!ZxEmulator.touchscreen)
                    messagePage.showMessage(ZxEmulator.Info, qsTr("No gamepad was found, please connect a gamepad"));
            }
        }
    }
}
