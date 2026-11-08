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
import QtQuick.Controls
import QtQuick.Layouts
import Spectacol

// A list to pick one item from: Up and Down move, Return (the gamepad's A) or a click picks it.
// The current choice is ticked.
ListView {
    id: list

    property int checkedIndex: -1
    property var textOf: (item) => String(item)
    signal chosen(int index)

    anchors.fill: parent
    anchors.margins: Theme.sp(2)
    clip: true
    focus: true
    spacing: Theme.sp(0.5)
    boundsBehavior: Flickable.StopAtBounds
    highlightMoveDuration: Theme.reduceMotion ? 0 : 100
    ScrollIndicator.vertical: ScrollIndicator {}

    highlight: Rectangle {
        radius: Theme.radiusSmall
        color: Theme.accentFill
        border.width: Theme.focusLine
        border.color: Theme.accent
    }

    delegate: Item {
        id: entry
        required property var modelData
        required property int index
        width: list.width
        height: Theme.rowHeight

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: Theme.sp(4)
            anchors.rightMargin: Theme.sp(4)
            Text {
                Layout.fillWidth: true
                text: list.textOf(entry.modelData)
                elide: Text.ElideRight
                font.pixelSize: Theme.body
                color: Theme.text
            }
            Text {
                visible: entry.index === list.checkedIndex
                text: "✓"
                font.pixelSize: Theme.title
                font.weight: Font.Bold
                color: Theme.success
            }
        }

        MouseArea {
            anchors.fill: parent
            onClicked: {
                list.currentIndex = entry.index;
                list.chosen(entry.index);
            }
        }
    }

    Keys.onReturnPressed: chosen(currentIndex)
    Keys.onEnterPressed: chosen(currentIndex)
    Keys.onSpacePressed: chosen(currentIndex)
    Component.onCompleted: currentIndex = Math.max(0, checkedIndex)
}
