/*
    Copyright (c) 2016, BogDan Vatra <bogdan@kde.org>

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


import QtQuick 2.6
import QtQuick.Window 2.2
import QtQuick.Controls 2.0

ListView {
    id: view
    x: (parent.width - view.width) / 2
    PropertyAnimation on x {
        from: 0
        to: (parent.width - view.width) / 2
        easing.type: Easing.InOutBack
        easing.amplitude: 2.0
        easing.period: 1.5
        duration: 500
    }

    anchors.top: parent.top
    anchors.bottom: parent.bottom
    anchors.margins: Screen.pixelDensity * 10

    clip: true
    focus: true
    snapMode: ListView.SnapToItem
    highlightFollowsCurrentItem: true
    highlightRangeMode: ListView.ApplyRange

    Keys.onLeftPressed: decrementCurrentIndex()
    Keys.onRightPressed: incrementCurrentIndex()

    highlight: Rectangle {
        width: view.width
        height: delegate.height
        color: "lightgreen"
        border.width: 0.5 * Screen.pixelDensity
        border.color: "green"
    }
}
