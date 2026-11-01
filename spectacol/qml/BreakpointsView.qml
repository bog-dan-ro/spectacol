/*
    Copyright (c) 2015-2025, BogDan Vatra <bogdan@kde.org>

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

import QtQuick
import QtQml.Models
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import Spectacol
import "private"

Item {
    property alias focused: view.focus
    signal breakpointSelected(var item)

    function removeCurrent() {
        if (view.currentIndex < 0 || view.currentIndex >= view.count)
            return;
        ZxEmulator.debuggerCommand("delete " + visualModel.items.get(view.currentIndex).model.id);
    }

    DelegateModel {
        id: visualModel
        model: ZxEmulator.breakpointsModel
        delegate: Rectangle {
            property color paper: view.currentIndex !== index ? "transparent" : Theme.accentFill
            property color ink: Theme.text

            width: view.width
            height: 7 * Screen.pixelDensity
            color: paper
            RowLayout {
                anchors.fill: parent
                FancyText {
                    mono: true
                    Layout.fillHeight: true
                    Layout.fillWidth: false
                    Layout.preferredWidth: 2 * TextSizes.scale12
                    horizontalAlignment: Text.AlignRight
                    font.pixelSize: TextSizes.scale12
                    style: Text.Normal
                    color: ink
                    text: model.id
                }

                Item { Layout.fillWidth: false; Layout.preferredWidth: 1.5 * Screen.pixelDensity }

                FancyText {
                    mono: true
                    Layout.fillHeight: true
                    Layout.fillWidth: false
                    Layout.preferredWidth: 3 * TextSizes.scale12
                    horizontalAlignment: Text.AlignHCenter
                    font.pixelSize: TextSizes.scale12
                    style: Text.Normal
                    color: ink
                    text: model.type + "," + model.life
                }

                Item { Layout.fillWidth: false; Layout.preferredWidth: 1.5 * Screen.pixelDensity }

                FancyText {
                    mono: true
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

        Keys.onPressed: (event) => {
            switch (event.key) {
            case Qt.Key_Y:
                removeCurrent();
                break;
            case Qt.Key_X:
                ZxEmulator.debuggerCommand("delete");
                break;
            }
        }

        model: visualModel
        footer: RowLayout {
            width: view.width
            Button {
                Layout.alignment: Qt.AlignVCenter
                Layout.fillWidth: true
                Layout.maximumWidth: implicitWidth
                enabled: view.currentIndex >= 0 && view.currentIndex < view.count
                text:"Remove <b>(Y)</b>"
                onClicked: removeCurrent()
            }
            Item {
                Layout.fillWidth: true
            }
            Button {
                Layout.alignment: Qt.AlignVCenter
                Layout.fillWidth: true
                Layout.maximumWidth: implicitWidth
                enabled: view.count > 0
                text:"Remove All <b>(X)</b>"
                onClicked: ZxEmulator.debuggerCommand("delete")
            }
        }
    }
}
