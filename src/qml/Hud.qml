/*
    Copyright (c) 2015-2026, BogDan Vatra <bogdan@kde.org>

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

pragma ComponentBehavior: Bound

import QtQuick
import Spectacol

// The status chips in the top right corner: tape and disk activity, paused, the speed
Row {
    id: hud

    property bool tapeActive: false
    property bool diskActive: false
    property bool paused: false
    property int speed: 100

    spacing: Theme.sp(2)

    component Chip: Rectangle {
        id: chip
        property string text
        property color dot: Theme.success
        property bool shown: false

        visible: opacity > 0
        opacity: shown ? 1 : 0
        Behavior on opacity {
            // linger a little: a loader stops and starts the tape
            NumberAnimation { duration: chip.shown ? Theme.enterDuration : Theme.reduceMotion ? 0 : 1500 }
        }
        height: label.implicitHeight + Theme.sp(2)
        width: label.implicitWidth + dotItem.width + Theme.sp(6)
        radius: height / 2
        color: Theme.surface
        border.width: Theme.line
        border.color: Theme.outline

        Rectangle {
            id: dotItem
            x: Theme.sp(2.5)
            anchors.verticalCenter: parent.verticalCenter
            width: Theme.sp(2)
            height: width
            radius: width / 2
            color: chip.dot
        }
        Text {
            id: label
            anchors.left: dotItem.right
            anchors.leftMargin: Theme.sp(1.5)
            anchors.verticalCenter: parent.verticalCenter
            text: chip.text
            font.pixelSize: Theme.caption
            color: Theme.text
        }
    }

    Chip { text: qsTr("Tape"); shown: hud.tapeActive }
    Chip { text: qsTr("Disk"); shown: hud.diskActive }
    Chip { text: qsTr("%1%").arg(hud.speed); dot: Theme.accent; shown: hud.speed !== 100 }
    Chip {
        objectName: "rzxProgressOverlay"
        text: ZxEmulator.recording.statusText
        shown: ZxEmulator.recording.hasProgress
        dot: ZxEmulator.recording.playbackError.length ? Theme.warning : Theme.accent
    }
    Chip {
        text: ZxEmulator.recording.seeking ? qsTr("Seeking…")
                      : ZxEmulator.recording.type === ZxRecording.None ? qsTr("End of playback; rewind to resume")
                      : qsTr("Press a game key to resume")
        shown: ZxEmulator.recording.waitingForInput
        dot: Theme.warning
    }
    Chip { objectName: "fastForwardOverlay"; text: qsTr("Fast-forward"); dot: Theme.accent; shown: ZxEmulator.fastForward && !ZxEmulator.paused }
    Chip { objectName: "pausedOverlay"; text: qsTr("Paused"); dot: Theme.warning; shown: hud.paused }
}
