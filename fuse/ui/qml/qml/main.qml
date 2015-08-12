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

        Flickable {
            anchors.fill: parent
            contentHeight: menuView.height; contentWidth: menuView.width
            flickableDirection: Flickable.VerticalFlick
            MenuView {
                anchors.fill: parent
                id: menuView
                rootMenu: FuseMenu {}
                parentWidth: menuBar.width
            }
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
            id: fuse
            focus: true
            anchors.fill: parent
            onScreenChanged: mainScreen.visibility = fullScreen ? Window.FullScreen : Window.AutomaticVisibility;
            onError: messagePage.showMessage(level, message);

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
        }
    }
}
