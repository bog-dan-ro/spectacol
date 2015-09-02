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
                onStatusChanged: {
                    switch (status) {
                    case Loader.Ready:
                        fuse.paused = true;
                        break;
                    case Loader.Null:
                        fuse.paused = false;
                        break;
                    }
                }
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
