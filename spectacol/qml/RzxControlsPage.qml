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

Sheet {
    id: root
    readonly property bool pausesEmulation: false
    property int seekStepMs: 5000
    readonly property var recording: ZxEmulator.recording
    readonly property bool seekable: recording.hasProgress && !recording.playbackError.length && recording.type !== ZxRecording.Unsaved
    readonly property bool active: recording.type === ZxRecording.Recording || recording.type === ZxRecording.Playback
    signal openPlaybackRequested()

    title: qsTr("Playback and recording")
    crumb: qsTr("Media")

    hints: [{pad: "B", key: "Esc", text: qsTr("Close"), action: () => root.closeRequested()}]
    Component.onCompleted: ZxEmulator.processInputEvents = false
    Component.onDestruction: ZxEmulator.processInputEvents = true

    SettingsList {
        SettingRow {
            kind: "action"
            label: qsTr("Recording")
            actionText: qsTr("Start")
            visible: !root.active && root.recording.type !== ZxRecording.Unsaved
            onClicked: { root.recording.start(); root.closeRequested(); }
        }
        SettingRow {
            kind: "action"
            label: qsTr("Playback")
            actionText: qsTr("Open…")
            visible: !root.active && root.recording.type !== ZxRecording.Unsaved
            onClicked: root.openPlaybackRequested()
        }
        SettingRow {
            kind: "action"
            label: qsTr("Playback")
            actionText: qsTr("Restart")
            visible: !root.active && root.recording.durationMs > 0 && root.recording.type !== ZxRecording.Unsaved
            onClicked: { root.recording.startPlayback(); root.closeRequested(); }
        }
        SettingRow {
            kind: "action"
            label: qsTr("%1 / %2 s").arg((root.recording.positionMs / 1000).toFixed(2)).arg((root.recording.durationMs / 1000).toFixed(2))
            description: root.recording.seeking ? qsTr("Seeking…")
                         : root.recording.waitingForInput ? qsTr("A fresh gameplay key or joystick movement resumes after closing this root.")
                         : qsTr("Rewind and fast-forward pause at the selected position.")
            actionText: root.recording.waitingForInput ? qsTr("Resume") : qsTr("Pause")
            visible: root.active
            enabled: !root.recording.seeking
            onClicked: {
                if (root.recording.waitingForInput) { root.recording.resume(); root.closeRequested(); }
                else root.recording.pause();
            }
        }
        SettingRow {
            kind: "choice"
            label: qsTr("Seek step")
            model: [qsTr("One frame"), qsTr("1 second"), qsTr("5 seconds"), qsTr("10 seconds")]
            currentIndex: [20,1000,5000,10000].indexOf(root.seekStepMs)
            visible: root.seekable
            onActivated: (index) => root.seekStepMs = [20,1000,5000,10000][index]
        }
        SettingRow {
            kind: "action"
            label: qsTr("Rewind")
            actionText: qsTr("Back")
            visible: root.seekable
            enabled: !root.recording.seeking && root.recording.positionMs > 0
            onClicked: { root.recording.rewind(root.seekStepMs); root.closeRequested(); }
        }
        SettingRow {
            kind: "action"
            label: qsTr("Fast-forward")
            actionText: qsTr("Forward")
            visible: root.seekable
            enabled: !root.recording.seeking && root.recording.positionMs < root.recording.durationMs
            onClicked: { root.recording.fastForward(root.seekStepMs); root.closeRequested(); }
        }
        SettingRow {
            kind: "action"
            label: qsTr("End of recording")
            actionText: qsTr("Seek")
            visible: root.seekable
            enabled: !root.recording.seeking && root.recording.positionMs < root.recording.durationMs
            onClicked: { root.recording.fastForward(-1); root.closeRequested(); }
        }
        SettingRow {
            kind: "action"
            label: root.recording.type === ZxRecording.Unsaved ? qsTr("Retry recording save")
                   : root.recording.type === ZxRecording.Recording ? qsTr("Stop and finalise at cursor") : qsTr("Stop playback")
            actionText: root.recording.type === ZxRecording.Unsaved ? qsTr("Retry") : qsTr("Stop")
            visible: root.active || root.recording.type === ZxRecording.Unsaved
            enabled: !root.recording.seeking
            onClicked: { root.recording.stop(); root.closeRequested(); }
        }
    }
}
