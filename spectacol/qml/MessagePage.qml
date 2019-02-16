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

import QtQuick 2.12
import QtQuick.Window 2.12
import Fuse 1.0
import "private" 1.0

Rectangle {
    id: msgDialog
    anchors.left: parent.left
    anchors.right: parent.right
    anchors.top: parent.top
    border.width: 0.25 * Screen.pixelDensity
    border.color: "red"
    color: Qt.rgba(0.15, 0.05, 0.05, 0.75)
    height: TextSizes.scale20 * 3
    visible: false

    FancyText {
        font.pixelSize: TextSizes.scale16
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
        if (level < fuseSettings.messageLevel)
            return;

        var msg = "";
        switch(level) {
        case FuseEmulator.Info:
            msgDialog.border.color = "white"
            msg = qsTr("<b>Info:</b> ");
            break;

        case FuseEmulator.Warning:
            msgDialog.border.color = "yellow"
            msg = qsTr("<b>Warning:</b> ");
            break;

        case FuseEmulator.Error:
            msgDialog.border.color = "red"
            msg = qsTr("<b>Error:</b> ");
            break;
        }

        message.text = msg + text;

        timer.interval = 2500 * (text.length / 20)

        timer.restart();
        msgDialog.visible = true;
    }
}
