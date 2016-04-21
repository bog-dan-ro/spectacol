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

// @scope main.qml

import QtQuick 2.6
import QtQuick.Window 2.2
import QtQuick.Controls 2.0
import "private" 1.0

FancyList {
    id: view
    Component.onCompleted: fuse.paused = true
    Component.onDestruction: fuse.paused = false

    model: fuse.filtersModel
    currentIndex: fuse.selectedFilterIndex
    onCurrentIndexChanged: fuse.selectedFilterIndex = currentIndex

    width: 20 * TextSizes.scale14
    delegate: Rectangle {
        width: view.width
        height: TextSizes.scale14
        color: Qt.rgba(0, 0, 0, 0.75);

        FancyText {
            anchors.fill: parent
            anchors.leftMargin: 10 * Screen.pixelDensity
            font.pixelSize: TextSizes.scale12
            text: modelData
        }
        MouseArea {
            anchors.fill: parent
            onClicked: view.currentIndex = model.index
        }
    }
}
