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

import QtQuick
import QtQuick.Layouts
import QtQuick.Window
import QtQuick.Controls
import QtCore
import QtGamepadLegacy
import Spectacol
import "private"

Rectangle {
    color: Qt.rgba(0, 0, 0, 0.75);
    Timer {
        id: timer
        repeat: false
        interval: 5000
        running: false
        onTriggered: {
            noResults.visible = false;
            FuseEmulator.onlineGamesModel.search(searchText.text);
        }
    }

    Connections {
        target: FuseEmulator.onlineGamesModel
        function onNoResults () {
            noResults.visible = true;
            searchText.forceActiveFocus(Qt.TabFocusReason);
        }
    }

    GamepadKeyNavigation {
        id: gamepadKeyNavigation
        gamepad: Gamepad { deviceId: FuseEmulator.gamepadId }
        buttonYKey: Qt.Key_Tab
        buttonBKey: Qt.Key_Escape
        buttonAKey: searchText.focus ? Qt.Key_unknown : Qt.Key_Return
    }

    Settings {
        category: "OnlineSearch"
        property alias searchString: searchText.text
    }

    function performSearch()
    {
        noResults.visible = false;
        timer.stop();
        if (searchText.text.length === 1)
            FuseEmulator.onlineGamesModel.search("", searchText.text);
        else
            FuseEmulator.onlineGamesModel.search(searchText.text);
        grid.forceActiveFocus(Qt.TabFocusReason);
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: TextSizes.scale24
        // Layout.topMargin: TextSizes.scale24
        FancyTextField {
            id: searchText
            focus: false
            font.pixelSize: TextSizes.scale16
            Layout.fillWidth: true
            Layout.leftMargin: TextSizes.scale24 * 4
            Layout.rightMargin: TextSizes.scale24 * 4
            placeholderText: qsTr("Search title or one letter to browse the catalogue")
            onTextChanged: timer.restart()
            KeyNavigation.tab: grid
            KeyNavigation.down: grid
            onAccepted: performSearch()
        }

        GridView {
            id: grid
            KeyNavigation.tab: searchText
            KeyNavigation.up: searchText
            Keys.onReturnPressed: {
                FuseEmulator.copyToFavourites(model.getPath(currentIndex));
                pageLoader.source = "";
            }
            clip: true
            focus: true
            currentIndex: 0
            Layout.alignment: Qt.AlignHCenter
            Layout.fillHeight: true
            // Cells are 320px wide, or a third of the column when that is narrower.
            // The grid shows whole cells, centered. Everything derives from the column's
            // width, not from the grid's own width which the layout sets from these.
            cellWidth: Math.max(1, Math.floor(Math.min(320, parent.width / 3)))
            cellHeight: cellWidth
            Layout.preferredWidth: Math.max(1, Math.floor(parent.width / cellWidth)) * cellWidth
            populate: Transition {
                NumberAnimation { properties: "x,y"; duration: 200 }
            }
            model: FuseEmulator.onlineGamesModel
            highlight: Rectangle {
                border.width: 0.25 * Screen.pixelDensity
                color: Qt.rgba(0.0, 0.85, 0.0, 0.75)
                border.color: "lightgreen"
            }
            delegate: Item {
                width: grid.cellWidth; height: grid.cellHeight
                Image {
                    anchors.margins: 2 * Screen.pixelDensity
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    fillMode: Image.PreserveAspectFit
                    source: "image://spectrum/" + screenFile
                }
                FancyText {
                    anchors.margins: 2 * Screen.pixelDensity
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    text: title
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        if (grid.currentIndex !== index)
                            grid.currentIndex = index;
                        else {
                            FuseEmulator.copyToFavourites(path);
                            pageLoader.source = "";
                        }
                    }
                }
            }
        }
        Button {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("Close (B)")
            onClicked: pageLoader.source = ""
        }
    }

    FancyText {
        id: noResults
        visible: false
        anchors.centerIn: parent
        font.pixelSize: TextSizes.scale16
        text: qsTr("No results")
    }

    Component.onCompleted: {
        noResults.visible = false;
        FuseEmulator.paused = true;
        FuseEmulator.processInputEvents = false;
        if (searchText.text.length)
            performSearch();
    }
    Component.onDestruction: {
        FuseEmulator.paused = false
        FuseEmulator.processInputEvents = true;
    }
}
