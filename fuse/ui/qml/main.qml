import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.1
import QtQuick.Window 2.0
import Fuse 1.0

ApplicationWindow {
    id: mainWindow
    visible: true
    width: 320*3
    height: 240*3

    title: qsTr("Hello World")

    NavigationDrawer {
        id: menuBar
        color: Qt.rgba(0, 0, 0, 0.5)
        width: parent.width / 4
        Button {
            text: "Full screen"
            onClicked: mainWindow.visibility = Window.FullScreen
        }
    }

    Rectangle {
        anchors.fill: parent
        color: "black"
        FuseScreen {
            id: fuse
            focus: true
            anchors.fill: parent
            Component.onCompleted: console.log("fuse " + fuse.toString())
        }
    }
}
