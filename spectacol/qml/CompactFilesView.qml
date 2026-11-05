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
import QtQuick.Controls.Material
import QtQuick.Layouts
import Spectacol

GridView {
    id: view
    property bool grid: false
    readonly property real pixelRatio: Window.window ? Window.window.devicePixelRatio : 1
    signal openRequested(int row)
    clip: true
    activeFocusOnTab: true
    cellWidth: grid ? width / Math.max(1, Math.floor(width / Theme.sp(48))) : width
    cellHeight: grid ? Theme.sp(45) : Theme.rowHeight
    highlightMoveDuration: Theme.reduceMotion ? 0 : Theme.enterDuration
    Keys.onPressed: event => {
        if (event.key === Qt.Key_End) {
            currentIndex = count - 1;
            event.accepted = true;
        }
    }
    Keys.onReturnPressed: openRequested(currentIndex)
    Keys.onEnterPressed: openRequested(currentIndex)
    Keys.onSelectPressed: openRequested(currentIndex)
    Keys.onSpacePressed: openRequested(currentIndex)
    Keys.onUpPressed: event => {
        if (currentIndex < Math.max(1, Math.floor(width / cellWidth)))
            ZxEmulator.sendNavigationKey(Qt.Key_Backtab);
        else
            moveCurrentIndexUp();
    }
    delegate: ItemDelegate {
        id: row
        required property int index
        required property string name
        required property string path
        required property bool isDir
        width: view.cellWidth - Theme.sp(1)
        height: view.cellHeight - Theme.sp(1)
        highlighted: view.currentIndex === index
        text: (isDir ? "▸ " : "") + name
        contentItem: ColumnLayout {
            spacing: Theme.sp(1)
            Image {
                Layout.fillWidth: true
                Layout.fillHeight: true
                visible: view.grid
                source: visible ? (row.isDir ? "qrc:/images/folder.svg" : "image://spectrum/" + row.path) : ""
                sourceSize: Qt.size(Math.ceil(view.cellWidth * view.pixelRatio), Math.ceil(view.cellHeight * view.pixelRatio))
                fillMode: Image.PreserveAspectFit
                smooth: false
                asynchronous: true
            }
            Label {
                Layout.fillWidth: true
                text: row.text
                color: Theme.text
                elide: Text.ElideRight
                font.pixelSize: Theme.body
            }
        }
        onClicked: {
            view.currentIndex = index;
            view.forceActiveFocus();
            view.openRequested(index);
        }
        Accessible.name: name
    }
    ScrollBar.vertical: ScrollBar {}
    Label {
        anchors.centerIn: parent
        visible: view.count === 0
        text: qsTr("No matching files")
        color: Theme.textMuted
    }
}
