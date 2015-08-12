import QtQuick 2.0
import QtQuick.Window 2.0
import Fuse 1.0

Rectangle {
    id: msgDialog
    anchors.left: parent.left
    anchors.right: parent.right
    anchors.top: parent.top
    border.width: 0.25 * Screen.pixelDensity
    border.color: "red"
    color: Qt.rgba(0.15, 0.05, 0.05, 0.75)
    height: 20 * Screen.pixelDensity
    visible: false

    FancyText {
        id: message
        horizontalAlignment: Text.AlignHCenter
        anchors.centerIn: parent
    }

    Timer {
        id: timer
        interval: 5000
        onTriggered: msgDialog.visible = false
    }

    MouseArea {
        anchors.fill: parent
        onClicked: msgDialog.visible = false
    }

    function showMessage(level, text) {
        var msg = "";
        switch(level) {
        case FuseScreen.Info:
            msg = qsTr("Info: ");
            break;

        case FuseScreen.Warning:
            msg = qsTr("Warning: ");
            break;

        case FuseScreen.Error:
            msg = qsTr("Error: ");
            break;
        }
        message.text = msg + text;
        timer.restart();
        msgDialog.visible = true;
    }
}

