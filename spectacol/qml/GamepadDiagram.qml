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

Image {
    id: diagram
    source: "qrc:/images/gamepad.svg"
    sourceSize: Qt.size(Theme.sp(100), Theme.sp(50))
    fillMode: Image.PreserveAspectFit
    Accessible.role: Accessible.Graphic
    Accessible.name: qsTr("Gamepad controls")
    Accessible.description: qsTr("D-pad on the left. Face buttons: Y above, X left, B right, A below. Back and Start in the centre, left and right sticks below them. L1 and L2 above the left grip; R1 and R2 above the right grip.")

    Repeater {
        model: [
            { label: "Y", face: true, cx: 66, cy: 18 },
            { label: "X", face: true, cx: 60, cy: 24 },
            { label: "B", face: true, cx: 72, cy: 24 },
            { label: "A", face: true, cx: 66, cy: 30 },
            { label: "L1 / L2", cx: 30, cy: 4 },
            { label: "R1 / R2", cx: 66, cy: 4 },
            { label: qsTr("Back", "gamepad button"), cx: 5, cy: 22 },
            { label: qsTr("Start", "gamepad button"), cx: 91, cy: 22 }
        ]
        delegate: Text {
            required property var modelData
            x: (diagram.width - diagram.paintedWidth) / 2 + (modelData.cx + 4) * diagram.paintedWidth / 104 - width / 2
            y: (diagram.height - diagram.paintedHeight) / 2 + modelData.cy * diagram.paintedHeight / 48 - height / 2
            text: modelData.label
            color: modelData.face === true ? Theme.textOnAccent : Theme.text
            font.pixelSize: Theme.caption
            font.weight: Font.DemiBold
            Accessible.ignored: true
        }
    }
}
