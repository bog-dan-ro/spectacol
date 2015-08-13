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
        fontSize: 4
        id: message
        anchors.fill: parent
        elide: Text.ElideRight
        wrapMode: Text.WrapAtWordBoundaryOrAnywhere
        horizontalAlignment: Text.AlignHCenter
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
            msg = qsTr("<b>Info:</b> ");
            break;

        case FuseScreen.Warning:
            msg = qsTr("<b>Warning:</b> ");
            break;

        case FuseScreen.Error:
            msg = qsTr("<b>Error:</b> ");
            break;
        }

        message.text = msg + text;

        timer.interval = 5000 + 1000 * (text.length / 20)

        timer.restart();
        msgDialog.visible = true;
    }
}

