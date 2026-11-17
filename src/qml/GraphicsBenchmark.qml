/*
    Copyright (c) 2015-2026, BogDan Vatra <bogdan@kde.org>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
*/
pragma ComponentBehavior: Bound

import QtQuick

Rectangle {
    id: root
    property int frame: 0
    color: "#181c24"

    Item {
        objectName: "benchmarkScreen"
        anchors.fill: parent
    }

    Rectangle {
        width: parent.width / 4
        height: parent.height
        color: "#e0202838"

        Column {
            width: parent.width
            Repeater {
                model: 10
                Rectangle {
                    required property int index
                    width: parent.width
                    height: root.height / 12
                    color: index === root.frame % 10 ? "#526484" : "#283040"
                    Text {
                        anchors.centerIn: parent
                        text: qsTr("Graphics test %1").arg(parent.index + 1)
                        color: "white"
                    }
                }
            }
        }
    }
}
