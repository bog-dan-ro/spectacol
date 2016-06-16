/*
    Copyright (c) 2015, BogDan Vatra <bogdan@kde.org>

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

import QtQuick 2.6
import QtQuick.Dialogs 1.2
import QtQuick.Window 2.0
import QtQuick.Controls 2.0
import QtGamepad 1.0
import Fuse 1.0
import "private"

ApplicationWindow {
    id: mainScreen
    visible: true

    width: 320 * 3
    height: 256 * 3

    title: qsTr("Spectacol")

    property int dpiMultiplier: 1
    property bool portrait: false

    MessageDialog {
        id: quitDialog
        icon: StandardIcon.Question
        title: "Spectacol"
        text: "Quit ?"
        standardButtons: StandardButton.Yes | StandardButton.No
        onYes: fuse.quit()
    }

    Drawer {
        id: menuBar
        focus: false
        dragMargin: fuseSettings.swipe4menu ? Qt.styleHints.startDragDistance : 0

        width: Math.max(mainScreen.width, mainScreen.height) / 3
        height: mainScreen.height
        MenuView {
            id: menuView
            anchors.fill: parent
            rootMenu: FuseMenu {}
            transform: Translate {
                x: (menuBar.position - 1.0) * menuView.width
            }
        }
        property bool paused: false
        onPausedChanged: fuse.paused = paused
        onPositionChanged: {
            menuView.reset();
            if (position == 1) {
                pageLoader.source = "";
                menuView.useGamepad = true;
                if (!paused)
                    paused = true;
            }

            if (position == 0) {
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

        Keys.onPressed: {
            switch (event.key) {
            case Qt.Key_F1:
            case Qt.Key_Search:
                if (menuBar.position) {
                    menuBar.close();
                } else {
                    if (pageLoader.source != "") {
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

            case Qt.Key_F10:
                quitDialog.open();
                event.accepted = true;
                break;

            case Qt.Key_F11:
                fuseScreen.fullScreen = !fuseScreen.fullScreen;
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
                    if (pageLoader.source != "") {
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
                    if (pageLoader.source != "") {
                        pageLoader.source = "";
                        event.accepted = true;
                    } else {
                        if (fuseSettings.hasStartButton || fuse.touchscreen)
                            quitDialog.open();
                        else
                            menuBar.open();
                        event.accepted = true;
                    }
                }
                break;
            }
            if (!event.accepted && fuse.processInputEvents) {
                fuse.keyPress(event.key, event.modifiers, event.isAutoRepeat);
                event.accepted = true;
            }
        }

        Keys.onReleased: {
            switch (event.key) {
            case Qt.Key_Return:
            case Qt.Key_Enter:
                if (menuBar.position) {
                    menuView.openSelectedMenu();
                    event.accepted = true;
                }
                break;
            }
            if (!event.accepted && fuse.processInputEvents) {
                fuse.keyRelease(event.key, event.modifiers, event.isAutoRepeat);
                event.accepted = true;
            }
        }

        FuseScreen {
            id: fuseScreen
            anchors.fill: parent
            anchors.leftMargin: (fuseSettings.leftMargin && mainScreen.height < mainScreen.width) ? 10 * Screen.pixelDensity : 0
            onScreenChanged: mainScreen.visibility = fullScreen ? Window.FullScreen : Window.AutomaticVisibility;

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
                    NumberAnimation on opacity {
                        id: cassetteIconHideAnimation
                        running: false
                        to: 0
                        duration: 2000
                        onStopped: {cassetteIcon.visible = false; cassetteIcon.opacity = 1}
                    }

                    source: "qrc:///images/cassette-tape.svg"
                }

                Image {
                    id: diskIcon
                    visible: false
                    height: Screen.pixelDensity * 13
                    width: Screen.pixelDensity * 13
                    NumberAnimation on opacity {
                        id: diskIconHideAnimation
                        running: false
                        to: 0
                        duration: 2000
                        onStopped: {diskIcon.visible = false; diskIcon.opacity = 1}
                    }

                    source: "qrc:///images/floppy.svg"
                }
            }
        }

        MouseArea {
            anchors.fill: parent
            onClicked: pageLoader.source = "";
        }

        Button {
            visible: fuse.touchscreen && fuse.showControlsIcons
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.topMargin: mainScreen.height > mainScreen.width ? mainScreen.height / 2 : 0;
            text: "="
            onClicked: {
                if (menuBar.position)
                    menuBar.close();
                else
                    menuBar.open();
            }
        }

        Column {
            visible: fuse.touchscreen && fuse.showControlsIcons
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.topMargin: mainScreen.height > mainScreen.width ? mainScreen.height / 2 : 0;
            opacity: (onScreenCursorJoystick.visible || onScreen48Keyboard.visible) ? 0.25 : 1.0
            Image {
                height: Screen.pixelDensity * 10
                width: Screen.pixelDensity * 15
                source: "qrc:///images/keyboard-icon.svg"
                MouseArea {
                    anchors.fill: parent
                    onClicked: toggleOnScreenControls(FuseEmulator.Keyboard48K, false);
                }
            }
            Image {
                height: Screen.pixelDensity * 10
                width: Screen.pixelDensity * 13
                source: "qrc:///images/controller-icon.svg"
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        onScreenCursorJoystick.source = "OnScreenJoystick.qml";
                        toggleOnScreenControls(FuseEmulator.CursorJoystick, false);
                    }
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
            source: "OnScreenCursorJoystick.qml"
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
            focus: visible
            enabled: visible
            onVisibleChanged: if (!visible) mainRectangle.forceActiveFocus(Qt.TabFocusReason);
        }

        MessagePage {
            id: messagePage
            z: 100
        }
    }

    function toggleOnScreenControls(type, gamepad) {
        switch (type) {
        case FuseEmulator.CursorJoystick:
            if (onScreenCursorJoystick.visible) {
                onScreenCursorJoystick.visible = false;
            } else {
                onScreenCursorJoystick.visible = true;
            }
            break;

        case FuseEmulator.Keyboard48K:
            onScreen48Keyboard.visible = !onScreen48Keyboard.visible;
            onScreen48Keyboard.gamepadMode = gamepad;
            break;
        }
    }

    MessageDialog {
        id: queryDialog
        icon: StandardIcon.Question
        title: "Spectacol"
        standardButtons: StandardButton.Yes | StandardButton.No
        onYes: fuse.setQuery(FuseEmulator.UiYes)
        onNo: fuse.setQuery(FuseEmulator.UiNo)
    }

    Connections {
        target: fuse
        onError: messagePage.showMessage(level, message);

        onShowDebugger: pageLoader.source = "DebuggerPage.qml"
        onHideDebugger: pageLoader.source = ""

        onConfigureJoystick: pageLoader.source = "Joysticks.qml"

        onShowMenu: menuBar.open();
        onHideMenu: menuBar.close();
        onToggleOnScreenControls: toggleOnScreenControls(type, true)
        onShowWelcome: {
            fuseSettings.swipe4menu = !TextSizes.smallScreen;
            fuseSettings.leftMargin = TextSizes.smallScreen;
            pageLoader.source = "AboutPage.qml";
        }

        onUiIconUpdate: {
            switch (item) {
            case FuseEmulator.Disk:
                if (state === FuseEmulator.Active) {
                    diskIconHideAnimation.stop();
                    diskIcon.visible = true
                } else if (diskIcon.visible) {
                    diskIconHideAnimation.start();
                }
                break;
            case FuseEmulator.Microdrive:
            case FuseEmulator.Tape:
                if (state === FuseEmulator.Active) {
                    cassetteIconHideAnimation.stop();
                    cassetteIcon.visible = true
                } else if (cassetteIcon.visible) {
                    cassetteIconHideAnimation.start();
                }
                break;
            case FuseEmulator.Mouse:
                break;
            case FuseEmulator.Paused:
                break;
            }
        }

        onOpenFile: {
            pageLoader.source = "GetFileBrowserPage.qml"
            if (path.length)
                pageLoader.item.folder = path;
            messagePage.showMessage(FuseEmulator.Info, title);
        }

        onSaveFile: {
            // TODO: Implement save dialog
            fuse.setOpenSaveFile("");
        }

        onQuery: {
            queryDialog.text = message;
            queryDialog.open();
        }

        onGetListIndex: {
            pageLoader.source = "GetListPage.qml";
            pageLoader.item.title = title;
            pageLoader.item.model = list;
        }

        onShowPokememSelector: pageLoader.source = "PokeManagerPage.qml"
    }

    Component.onCompleted: {
        // Check gamepads status
        if (fuse.gamepadId == -1) {
            var len = GamepadManager.connectedGamepads.length;
            if (len > 0) {
                for (var i = 0; i < len; ++i) {
                    var id = GamepadManager.connectedGamepads[i];
                    if (!GamepadManager.isConfigurationNeeded(id)) {
                        fuse.gamepadId = id;
                        break;
                    }
                }

                if (fuse.gamepadId == -1) {
                    fuse.gamepadId = GamepadManager.connectedGamepads[0];
                    pageLoader.source = "CalibrateGamepad.qml";
                }
            } else {
                if (!fuse.touchscreen)
                    messagePage.showMessage(FuseEmulator.Info, qsTr("No gamepad was found, please connect a gamepad"));
            }
        }
    }
}
