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

// @scope main.qml

import QtQuick 2.6
import QtQuick.Layouts 1.1
import QtQuick.Window 2.2
import Qt.labs.controls 1.0
import QtGamepad 1.0

Rectangle {
    color: Qt.rgba(0, 0, 0, 0.75);
    Timer {
        id: timer
        repeat: false
        interval: 500
        running: false
        onTriggered: {
            onlineGamesModel.search(searchText.text);
            grid.focus = true;
        }
    }

    GamepadKeyNavigation {
        id: gamepadKeyNavigation
        gamepad: Gamepad { deviceId: fuse.gamepadId }
        active: false
        onActiveChanged: fuse.processJoysticksEvents = !active
        buttonAKey: Qt.Key_Return
        buttonStartKey: Qt.Key_F1
        buttonYKey: Qt.Key_Tab
        buttonBKey: Qt.Key_Escape
    }

    ColumnLayout {
        anchors.fill: parent
        FancyTextField {
            id: searchText
            focus: false
            Layout.fillWidth: true
            placeholderText: qsTr("Type to search")
            onTextChanged: timer.restart()
            KeyNavigation.tab: grid
        }
        GridView {
            id: grid
            KeyNavigation.tab: searchText
            Keys.onReturnPressed:{
                pageLoader.source = "";
                fuse.load(model.getPath(currentIndex))
            }
            clip: true
            focus: true
            currentIndex: 0
            Layout.fillWidth: true
            Layout.fillHeight: true
            cellWidth: 320 + 4 * Screen.pixelDensity
            cellHeight: 320 + 4 * Screen.pixelDensity
            populate: Transition {
                NumberAnimation { properties: "x,y"; duration: 200 }
            }
            model: onlineGamesModel
            highlight: Rectangle {
                border.width: 0.25 * Screen.pixelDensity
                color: Qt.rgba(0.0, 0.85, 0.0, 0.75)
                border.color: "lightgreen"
            }
            delegate: Item {
                width: grid.cellWidth; height: grid.cellHeight
                ColumnLayout {
                    anchors.fill: parent
                    Image {
                        Layout.margins: 2 * Screen.pixelDensity
                        Layout.alignment: Qt.AlignHCenter
                        Layout.preferredHeight: 240
                        Layout.preferredWidth: 320
                        source: "image://spectrum/" + screenFile
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    FancyText {
                        Layout.fillHeight: true
                        Layout.fillWidth: true
                        text: title
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        if (grid.currentIndex != index)
                            grid.currentIndex = index;
                        else {
                            fuse.load(path);
                            pageLoader.source = "";
                        }
                    }
                }
            }
        }
    }
    Component.onCompleted: {
        onlineGamesModel.search();
        fuse.paused = true;
        gamepadKeyNavigation.active = true;
    }
    Component.onDestruction: {
        gamepadKeyNavigation.active = false;
        fuse.paused = false
    }
}
