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

import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Dialogs 1.2
import QtQuick.Window 2.0
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

    NavigationDrawer {
        id: menuBar
        color: Qt.rgba(0, 0, 0, 0.5)
        width: parent.width / 4

        MenuView {
            anchors.fill: parent
            id: menuView
            rootMenu: FuseMenu {}
            parentWidth: menuBar.width
        }

        onOpenChanged: {
            menuView.reset();
            if (open)
                pageLoader.source = "";
        }
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
                    menuBar.open = !menuBar.open;
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
                    if (menuBar.open) {
                        menuView.incrementCurrentIndex();
                        event.accepted = true;
                    }
                    break;

                case Qt.Key_Up:
                    if (menuBar.open) {
                        menuView.decrementCurrentIndex();
                        event.accepted = true;
                    }
                    break;

                case Qt.Key_Return:
                case Qt.Key_Enter:
                    if (menuBar.open) {
                        menuView.openSelectedMenu();
                        event.accepted = true;
                    }
                    break;

                case Qt.Key_Escape:
                    if (menuBar.open) {
                        menuView.popMenu();
                        event.accepted = true;
                    } else {
                        if (pageLoader.source != "") {
                            pageLoader.source = "";
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

            Loader {
                id: pageLoader
                anchors.fill: parent
                asynchronous: true
                visible: status === Loader.Ready
                focus: visible
                enabled: visible
//                onStatusChanged: {
//                    switch (status) {
//                    case Loader.Ready:
//                        fuse.paused = true;
//                        break;
//                    case Loader.Null:
//                        fuse.paused = false;
//                        break;
//                    }
//                }
            }
        }

        MessagePage {
            id: messagePage
            z: 100
            Connections {
                target: fuse
                onError: messagePage.showMessage(level, message);
            }
        }
    }

    Connections {
        target: fuse
        onShowDebugger: pageLoader.source = "DebuggerPage.qml";
    }

    Connections {
        target: fuse
        onHideDebugger: pageLoader.source = "";
    }
}
