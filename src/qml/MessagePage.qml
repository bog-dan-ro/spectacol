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
import QtQuick.Layouts
import Spectacol

// Messages: a toast in the bottom right corner, away from the pages' headers. A click dismisses it
Rectangle {
    id: toast

    property int level: ZxEmulator.Info
    // An open page's sheet (on the right) and its hint bar: the message goes to the left of
    // the sheet when there is room, else above the hint bar
    property real reservedRight: 0
    property real reservedBottom: 0
    readonly property real margin: Theme.sp(6)
    readonly property real freeLeft: parent.width - reservedRight - 2 * margin
    readonly property bool besideSheet: reservedRight === 0 || freeLeft >= Theme.sp(60)

    width: Math.min(Theme.sp(115), besideSheet && reservedRight > 0 ? freeLeft : parent.width - 2 * margin)
    x: reservedRight === 0 ? parent.width - width - margin : besideSheet ? margin : (parent.width - width) / 2
    y: parent.height - height - margin - (besideSheet ? 0 : reservedBottom)
    height: content.implicitHeight + 2 * Theme.sp(3)
    radius: Theme.radius
    color: Theme.raised
    border.width: Theme.line
    border.color: Theme.outline
    opacity: 0
    visible: opacity > 0

    Behavior on opacity {
        NumberAnimation { duration: toast.opacity > 0 ? Theme.exitDuration : Theme.enterDuration; easing.type: Easing.OutCubic }
    }

    RowLayout {
        id: content
        anchors.fill: parent
        anchors.margins: Theme.sp(3)
        anchors.leftMargin: Theme.sp(4)
        spacing: Theme.sp(3)

        // The level: a colour and a symbol, never the colour alone
        Rectangle {
            Layout.alignment: Qt.AlignTop
            Layout.preferredWidth: Math.round(Theme.body * 1.5)
            Layout.preferredHeight: Layout.preferredWidth
            radius: width / 2
            color: toast.level === ZxEmulator.Error ? Theme.danger : toast.level === ZxEmulator.Warning ? Theme.warning : Theme.accent
            Text {
                anchors.centerIn: parent
                text: toast.level === ZxEmulator.Error ? "×" : toast.level === ZxEmulator.Warning ? "!" : "i"
                font.pixelSize: Theme.body
                font.weight: Font.Bold
                color: "#101217"
            }
        }

        Text {
            id: message
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            maximumLineCount: 4
            elide: Text.ElideRight
            textFormat: Text.StyledText
            font.pixelSize: Theme.body
            color: Theme.text
        }
    }

    Timer {
        id: timer
        onTriggered: toast.opacity = 0
    }

    MouseArea {
        anchors.fill: parent
        onClicked: toast.opacity = 0
    }

    function showMessage(level, text) {
        if (level < ZxEmulator.settings.messageLevel)
            return;
        toast.level = level;
        message.text = text;
        // about 2.5s for every 20 characters, but long enough to notice and not too long
        timer.interval = Math.min(8000, Math.max(2500, 2500 * text.length / 20));
        timer.restart();
        toast.opacity = 1;
    }
}
