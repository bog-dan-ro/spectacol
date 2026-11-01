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

// A page's buttons: the gamepad's (A, B, X, Y, Menu) when one is connected, else the keys.
// Each hint is also a button for touch and mouse:
//   hints: [{ pad: "A", key: "Enter", text: qsTr("Select"), action: () => ... }, ...]
Item {
    id: bar

    property var hints: []
    readonly property bool gamepad: ZxEmulator.gamepadId !== -1

    implicitHeight: flow.implicitHeight + Theme.sp(6)

    function padColor(pad: string): color {
        switch (pad) {
        case "A": return Theme.padA;
        case "B": return Theme.padB;
        case "X": return Theme.padX;
        case "Y": return Theme.padY;
        }
        return "#c9ced8";
    }

    Rectangle {
        width: parent.width
        height: Theme.line
        color: Theme.outline
    }

    Flow {
        id: flow
        anchors.fill: parent
        anchors.leftMargin: Theme.sp(5)
        anchors.rightMargin: Theme.sp(5)
        anchors.topMargin: Theme.sp(3)
        spacing: Theme.sp(5)

        Repeater {
            model: bar.hints

            delegate: Item {
                id: hint
                required property var modelData
                readonly property bool round: bar.gamepad && hint.modelData.pad.length === 1
                implicitWidth: content.implicitWidth
                implicitHeight: Math.max(content.implicitHeight, Theme.sp(8))

                Row {
                    id: content
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: Theme.sp(1.5)

                    Rectangle {
                        anchors.verticalCenter: parent.verticalCenter
                        height: Math.round(Theme.caption * 1.7)
                        width: hint.round ? height : Math.max(height, keyLabel.implicitWidth + Theme.sp(3))
                        radius: hint.round ? height / 2 : Theme.radiusSmall
                        color: bar.gamepad ? bar.padColor(hint.modelData.pad) : Theme.raised
                        border.width: bar.gamepad ? 0 : Theme.line
                        border.color: Theme.outline

                        Text {
                            id: keyLabel
                            anchors.centerIn: parent
                            text: bar.gamepad ? (hint.modelData.pad === "Menu" ? "☰" : hint.modelData.pad) : hint.modelData.key
                            font.pixelSize: Math.round(Theme.caption * 0.9)
                            font.weight: Font.DemiBold
                            color: bar.gamepad ? "#101217" : Theme.text
                        }
                    }

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: hint.modelData.text
                        font.pixelSize: Theme.caption
                        color: Theme.textMuted
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    enabled: hint.modelData.action !== undefined
                    cursorShape: Qt.PointingHandCursor
                    onClicked: hint.modelData.action()
                }
            }
        }
    }
}
