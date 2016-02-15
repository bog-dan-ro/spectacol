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
import Qt.labs.controls 1.0
import QtGamepad 1.0
import Fuse 1.0

ApplicationWindow {
    id: mainScreen
    visible: true

    width: 320 * 3
    height: 256 * 3

    title: qsTr("Fuse QML")

    property int dpiMultiplier: 1
    property bool portrait: false

    MessageDialog {
        id: quitDialog
        icon: StandardIcon.Question
        title: "Fuse QML"
        text: "Quit ?"
        standardButtons: StandardButton.Yes | StandardButton.No
        onYes: Qt.quit()
    }

    Drawer {
        id: menuBar
        anchors.fill: parent
        MenuView {
            id: menuView
            width: mainScreen.width / 4
            height: mainScreen.height
            visible: menuBar.position > 0

            parentWidth: menuBar.width

            rootMenu: FuseMenu {}
            transform: Translate {
                x: (menuBar.position - 1.0) * menuView.width
            }
        }

        onPositionChanged: {
            menuView.reset();
            if (position == 1) {
                pageLoader.source = "";
                menuView.useGamepad = true;
            }

            if (position == 0)
                menuView.useGamepad = false;
        }
        onClicked: close()
    }

    Rectangle {
        anchors.fill: parent
        color: "black"
        FuseScreen {
            id: fuseScreen
            focus: true
            anchors.fill: parent
            onScreenChanged: mainScreen.visibility = fullScreen ? Window.FullScreen : Window.AutomaticVisibility;
            Keys.onPressed: {
                switch (event.key) {
                case Qt.Key_F1:
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
                    fullScreen = !fullScreen;
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
                    if (menuBar.position) {
                        menuView.openSelectedMenu();
                        event.accepted = true;
                    }
                    break;

                case Qt.Key_Back:
                case Qt.Key_Escape:
                    if (menuBar.position) {
                        menuView.popMenu();
                        event.accepted = true;
                    } else {
                        if (pageLoader.source != "") {
                            pageLoader.source = "";
                            event.accepted = true;
                        } else {
                            quitDialog.open();
                            event.accepted = true;
                        }
                    }
                    break;
                }
            }
            MouseArea {
                anchors.fill: parent
                onClicked: pageLoader.source = "";
            }

            Button {
                anchors.top: parent.top
                anchors.left: parent.left
                text: "="
                onClicked: {
                    if (menuBar.position)
                        menuBar.close();
                    else
                        menuBar.open();
                }
            }

            Loader {
                id: pageLoader
                anchors.fill: parent
                asynchronous: true
                visible: status === Loader.Ready
                focus: visible
                enabled: visible
            }
        }

        MessagePage {
            id: messagePage
            z: 100
        }
    }

    Connections {
        target: fuse
        onError: messagePage.showMessage(level, message);

        onShowDebugger: pageLoader.source = "DebuggerPage.qml";
        onHideDebugger: pageLoader.source = "";

        onShowMenu: menuBar.open();
        onHideMenu: menuBar.close();
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
            }
        }
    }
}
