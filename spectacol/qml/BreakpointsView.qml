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

import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Window 2.12

import "private" 1.0

Item {
    property alias focused: view.focus
    signal breakpointSelected(var item)

    VisualDataModel {
        id: visualModel
        model: breakpointsModel
        delegate: Rectangle {
            property color paper: view.currentIndex !== index ? Qt.rgba(0, 0, 0, 0.9) : "white"
            property color ink: view.currentIndex !== index ? "white": Qt.rgba(0, 0, 0, 0.9)

            width: view.width
            height: 7 * Screen.pixelDensity
            color: paper
            RowLayout {
                anchors.fill: parent
                FancyText {
                    Layout.fillHeight: true
                    Layout.fillWidth: false
                    Layout.preferredWidth: 2 * TextSizes.scale12
                    horizontalAlignment: Text.AlignRight
                    font.pixelSize: TextSizes.scale12
                    style: Text.Normal
                    color: ink
                    text: model.id
                }

                Item { Layout.fillWidth: false; width: 1.5 * Screen.pixelDensity }

                FancyText {
                    Layout.fillHeight: true
                    Layout.fillWidth: false
                    Layout.preferredWidth: 3 * TextSizes.scale12
                    horizontalAlignment: Text.AlignHCenter
                    font.pixelSize: TextSizes.scale12
                    style: Text.Normal
                    color: ink
                    text: model.type + "," + model.life
                }

                Item { Layout.fillWidth: false; width: 1.5 * Screen.pixelDensity }

                FancyText {
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignLeft
                    font.pixelSize: TextSizes.scale12
                    color: ink
                    style: Text.Normal
                    text: model.value + " " + model.condition + model.commands
                }
            }
            MouseArea {
                anchors.fill: parent
                onClicked: {view.forceActiveFocus(Qt.TabFocusReason); view.currentIndex = model.index}
                onPressAndHold: breakpointSelected(model)
                onDoubleClicked: breakpointSelected(model)
            }
        }
    }
    ListView {
        id: view

        anchors.fill: parent
        clip: true
        snapMode: ListView.SnapToItem
        highlightFollowsCurrentItem: true
        focus: true

        Layout.fillHeight: true
        Layout.fillWidth: true
        Keys.onPressed: {
            switch (event.key) {
            case Qt.Key_Y:
                fuse.debuggerCommand("delete " + visualModel.items.get(view.currentIndex).model.id);
                break;
            case Qt.Key_X:
                fuse.debuggerCommand("delete");
                break;
            }
        }

        model: visualModel
        footer: RowLayout {
            Button {
                Layout.alignment: Qt.AlignVCenter
                enabled: view.currentIndex != -1
                text:"Remove <b>(Y)</b>"
                onClicked: fuse.debuggerCommand("delete " + visualModel.items.get(view.currentIndex).model.id)
            }
            Item {
                Layout.fillWidth: true
            }
            Button {
                Layout.alignment: Qt.AlignVCenter
                enabled: view.currentIndex != -1
                text:"Remove All <b>(X)</b>"
                onClicked: fuse.debuggerCommand("delete")
            }
        }
    }
}
