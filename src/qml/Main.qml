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
    visibility: Theme.touchScreenUi ? Window.FullScreen : Window.AutomaticVisibility

    width: 320 * 3
    height: 256 * 3

    title: qsTr("Spectacol")

    property bool portrait: width < height
    function goBack() {
        const sheet = pageLoader.item as Sheet;
        if (Qt.inputMethod.visible)
            Qt.inputMethod.hide();
        else if (sheet && sheet.closePageActions())
            return;
        else if (pageLoader.item && typeof pageLoader.item.closeDialog === "function" && pageLoader.item.closeDialog())
            return;
        else if (menuBar.position)
            menuView.popMenu();
        else if (pageLoader.status !== Loader.Null)
            pageLoader.source = "";
        else if (onScreen48Keyboard.visible)
            onScreen48Keyboard.visible = false;
        else if (ZxEmulator.settings.hasStartButton || ZxEmulator.touchscreen)
            quitDialog.open();
        else
            menuBar.open();
    }
    Shortcut {
        sequences: ["Back"]
        enabled: Qt.platform.os === "android"
        onActivated: mainScreen.goBack()
    }
    // The menu, for the overlay's drawer and the desktop layout's menu bar
    property ZxMenu zxMenu: ZxMenu {}
    // A page docked next to the screen (desktop layout): the screen makes room for it
    readonly property real dockWidth: Theme.desktopMode && pageLoader.status === Loader.Ready && pageLoader.item.panelWidth !== undefined
                                      && pageLoader.item.panelWidth < mainRectangle.width * 0.7 ? pageLoader.item.panelWidth : 0

    menuBar: DesktopMenuBar {
        visible: Theme.desktopMode
        rootMenu: mainScreen.zxMenu
    }

    footer: Rectangle {
        visible: Theme.desktopMode
        height: statusRow.implicitHeight + Theme.sp(3)
        color: Theme.raised
        Rectangle {
            width: parent.width
            height: Theme.line
            color: Theme.outline
        }
        Row {
            id: statusRow
            anchors.left: parent.left
            anchors.leftMargin: Theme.sp(4)
            anchors.verticalCenter: parent.verticalCenter
            spacing: Theme.sp(6)
            component Status: Text {
                font.pixelSize: Theme.caption
                color: Theme.textMuted
            }
            Status { text: ZxEmulator.settings.currentMachine }
            Status { visible: hud.tapeActive; text: qsTr("Tape playing"); color: Theme.success }
            Status { visible: hud.diskActive; text: qsTr("Disk"); color: Theme.success }
            Status { objectName: "fastForwardStatus"; visible: ZxEmulator.fastForward && !ZxEmulator.paused; text: qsTr("Fast-forward"); color: Theme.accent }
            Status { visible: hud.speed !== 100; text: qsTr("Speed %1%").arg(hud.speed) }
            Status {
                objectName: "rzxProgress"
                visible: ZxEmulator.recording.hasProgress
                text: ZxEmulator.recording.statusText
                color: ZxEmulator.recording.playbackError.length ? Theme.warning : Theme.textMuted
            }
            Status {
                visible: ZxEmulator.recording.waitingForInput
                text: ZxEmulator.recording.seeking ? qsTr("Seeking…")
                      : ZxEmulator.recording.type === ZxRecording.None ? qsTr("End of playback; rewind to resume")
                      : qsTr("Press a game key to resume")
                color: Theme.warning
            }
            Status { visible: hud.paused; text: qsTr("Paused"); color: Theme.warning }
        }
    }

    MessageDialog {
        id: quitDialog
        title: "Spectacol"
        text: "Quit ?"
        buttons: MessageDialog.Yes | MessageDialog.No
        onAccepted: ZxEmulator.quit()
    }

    Drawer {
        id: menuBar
        // Our flat drawer has no rounded-corner spacer above its header.
        topPadding: SafeArea.margins.top
        bottomPadding: SafeArea.margins.bottom
        focus: true
        onOpened: menuView.focusMenu()
        onClosed: Qt.callLater(() => {
            // A page may open while the drawer is leaving. Popup focus
            // restoration must finish before the new page takes focus.
            if (pageLoader.status === Loader.Ready) {
                if (typeof pageLoader.item.focusFiles === "function") pageLoader.item.focusFiles();
                else pageLoader.item.forceActiveFocus(Qt.PopupFocusReason);
            } else {
                mainRectangle.forceActiveFocus(Qt.PopupFocusReason);
            }
        })
        dragMargin: ZxEmulator.settings.swipe4menu ? Qt.styleHints.startDragDistance : 0

        width: Math.min(mainScreen.width * 0.85, Theme.sp(95))
        height: mainScreen.height
        background: Rectangle {
            color: Theme.surface
            Rectangle {
                anchors.right: parent.right
                width: Theme.line
                height: parent.height
                color: Theme.outline
            }
        }
        Overlay.modal: Rectangle {
            color: Theme.scrim
        }
        enter: Transition {
            NumberAnimation { property: "position"; to: 1.0; duration: Theme.enterDuration; easing.type: Easing.OutCubic }
        }
        exit: Transition {
            NumberAnimation { property: "position"; to: 0.0; duration: Theme.exitDuration; easing.type: Easing.InCubic }
        }
        MenuView {
            id: menuView
            anchors.fill: parent
            rootMenu: mainScreen.zxMenu
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
            const debuggerPage = pageLoader.item as DebuggerPage;
            if (debuggerPage && debuggerPage.keyPress(event.key, event.modifiers)) {
                event.accepted = true;
                return;
            }
            switch (event.key) {
            // F1-F4 belong to the emulated machine (the CoBra's keys): Spectacol's own keys are F5 on
            case Qt.Key_F5:
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

            case Qt.Key_F8:
                ZxEmulator.recording.rewind(5000);
                event.accepted = true;
                break;
            case Qt.Key_F9:
                ZxEmulator.recording.fastForward(event.modifiers & Qt.ShiftModifier ? -1 : 5000);
                event.accepted = true;
                break;
            case Qt.Key_F6:
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

            case Qt.Key_Select:
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
                mainScreen.goBack();
                event.accepted = true;
                break;
            }
            if (!event.accepted && ZxEmulator.processInputEvents) {
                ZxEmulator.keyPress(event.key, event.modifiers, event.isAutoRepeat, false);
                event.accepted = true;
            }
        }

        Keys.onReleased: (event)=> {
            switch (event.key) {
            case Qt.Key_Select:
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
            anchors.rightMargin: mainScreen.dockWidth

            ZxScreen {
                id: zxScreen
                fullScreen: Qt.platform.os === "android"
                anchors.fill: parent
                mouseGrabSuspended: pageLoader.status !== Loader.Null || menuBar.position > 0 || !ZxEmulator.processInputEvents

                onFullScreenChanged: mainScreen.visibility = fullScreen ? Window.FullScreen : Window.AutomaticVisibility;
            }
        }

        Hud {
            id: hud
            z: 15
            // the desktop layout has its status bar
            visible: !Theme.desktopMode
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.margins: Theme.sp(4)
            // paused by the user, not by an open page or the menu
            paused: ZxEmulator.paused && pageLoader.status === Loader.Null && menuBar.position === 0
            speed: ZxEmulator.settings.emulationSpeed
        }

        MouseArea {
            anchors.fill: parent
            // let the clicks reach the ZX screen (Kempston mouse) when no page is open
            enabled: pageLoader.status !== Loader.Null
            onClicked: pageLoader.source = "";
        }

        Grid {
            id: screenButtonsGrid
            objectName: "screenControls"
            anchors.fill: parent
            // The landscape control rail belongs to the window edge, outside
            // the safe-area padding used by pages and the ZX viewport.
            anchors.leftMargin: mainScreen.portrait ? 0 : -mainScreen.leftPadding
            anchors.topMargin: mainScreen.portrait ? zxScreen.implicitHeight : 0;
            visible: ZxEmulator.touchscreen && ZxEmulator.showControlsIcons
            horizontalItemAlignment: mainScreen.portrait ? Grid.AlignHCenter : Grid.AlignLeft
            verticalItemAlignment: Grid.AlignVCenter
            columns: mainScreen.portrait ? 6 : 1
            rows: mainScreen.portrait ? 1 : 6
            columnSpacing: Screen.pixelDensity
            rowSpacing: Screen.pixelDensity

            opacity: ZxEmulator.settings.buttonsOpacity
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
                fillMode: Image.PreserveAspectFit
                source: "qrc:/images/keyboard-icon.svg"
                MouseArea {
                    anchors.fill: parent
                    onClicked: toggleOnScreenControls(ZxEmulator.Keyboard48K, false);
                }
            }

            // Not dimmed with the others: it is used together with the on-screen keyboard.
            Image {
                id: stickyCSSSButton
                property bool checked: false
                height: Screen.pixelDensity * 7
                width: Screen.pixelDensity * 7
                sourceSize: Qt.size(width, height)
                fillMode: Image.PreserveAspectFit
                source: checked ? "qrc:/images/sticky-shift-on.svg" : "qrc:/images/sticky-shift.svg"
                Accessible.role: Accessible.CheckBox
                Accessible.name: onScreen48Keyboard.cobraMode ? "Sticky CS/SS/CTRL" : "Sticky CS/SS"
                Accessible.checkable: true
                Accessible.checked: checked
                Accessible.onPressAction: checked = !checked
                Accessible.onToggleAction: checked = !checked
                MouseArea {
                    anchors.fill: parent
                    onClicked: stickyCSSSButton.checked = !stickyCSSSButton.checked
                }
            }

            Image {
                opacity: parent.buttonsOpacity
                height: Screen.pixelDensity * 5
                width: Screen.pixelDensity * 7
                sourceSize: Qt.size(width, height)
                fillMode: Image.PreserveAspectFit
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
                fillMode: Image.PreserveAspectFit
                source: "qrc:/images/fastforward.svg"
                MouseArea {
                    anchors.fill: parent
                    onPressed: ZxEmulator.speedup()
                    onReleased: ZxEmulator.slowdown()
                    onCanceled: ZxEmulator.cancelFastForward()
                }
            }
            Image {
                opacity: parent.buttonsOpacity
                height: Screen.pixelDensity * 7
                width: Screen.pixelDensity * 7
                sourceSize: Qt.size(width, height)
                fillMode: Image.PreserveAspectFit
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
            opacity: ZxEmulator.settings.keyboardOpacity
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
            Connections {
                target: pageLoader.status === Loader.Ready ? pageLoader.item : null
                ignoreUnknownSignals: true
                function onOpenPlaybackRequested() {
                    pageLoader.setSource("FileBrowserPage.qml", {folder: ZxEmulator.recordingsPath()});
                }
            }
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
            reservedRight: pageLoader.status === Loader.Ready && pageLoader.item.panelWidth !== undefined ? pageLoader.item.panelWidth : 0
            reservedBottom: pageLoader.status === Loader.Ready && pageLoader.item.hintBarHeight !== undefined ? pageLoader.item.hintBarHeight : 0
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
        function onToggleOnScreenControls(type) { toggleOnScreenControls(type, true); }
        function onShowWelcome() {
            ZxEmulator.settings.swipe4menu = !TextSizes.smallScreen;
            ZxEmulator.settings.leftMargin = TextSizes.smallScreen;
            pageLoader.source = "AboutPage.qml";
        }

        function onUiIconUpdate(item, state) {
            switch (item) {
            case ZxEmulator.Disk:
                hud.diskActive = state === ZxEmulator.Active;
                break;
            case ZxEmulator.Microdrive:
            case ZxEmulator.Tape:
                hud.tapeActive = state === ZxEmulator.Active;
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
                // the desktop layout is for the keyboard and the mouse anyway
                if (!ZxEmulator.touchscreen && !Theme.desktopMode)
                    messagePage.showMessage(ZxEmulator.Info, qsTr("No gamepad found. Connect one to play with it."));
            }
        }
    }
}
