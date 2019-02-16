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

import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Window 2.12
import QtQuick.Controls 2.12
import Qt.labs.settings 1.0
import QtGamepad 1.0
import "private" 1.0

Rectangle {
    color: Qt.rgba(0, 0, 0, 0.75);
    Timer {
        id: timer
        repeat: false
        interval: 5000
        running: false
        onTriggered: {
            noResults.visible = false;
            onlineGamesModel.search(searchText.text);
        }
    }

    Connections {
        target: onlineGamesModel
        onNoResults: {
            noResults.visible = true;
            searchText.forceActiveFocus(Qt.TabFocusReason);
        }
    }

    GamepadKeyNavigation {
        id: gamepadKeyNavigation
        gamepad: Gamepad { deviceId: fuse.gamepadId }
        buttonYKey: Qt.Key_Tab
        buttonBKey: Qt.Key_Escape
        buttonAKey: searchText.focus ? Qt.Key_unknown : Qt.Key_Return
        onButtonAKeyChanged: console.log(buttonAKey)
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
            onlineGamesModel.search("", searchText.text);
        else
            onlineGamesModel.search(searchText.text);
        grid.forceActiveFocus(Qt.TabFocusReason);
    }

    ColumnLayout {
        anchors.fill: parent
        Layout.topMargin: TextSizes.scale24
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
                pageLoader.source = "";
                fuse.copyToFavourites(model.getPath(currentIndex))
            }
            property real __scale: width / 320. > 3 ? width / 320. : 3
            clip: true
            focus: true
            currentIndex: 0
            Layout.alignment: Qt.AlignHCenter
            Layout.fillHeight: true
            cellWidth: width / __scale
            cellHeight: cellWidth
            Layout.preferredWidth: Math.floor(parent.width / Math.ceil(cellWidth)) * Math.ceil(cellWidth)
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
                        if (grid.currentIndex != index)
                            grid.currentIndex = index;
                        else {
                            fuse.copyToFavourites(path);
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
        text: qsTr("No resutls")
    }

    Component.onCompleted: {
        noResults.visible = false;
        fuse.paused = true;
        fuse.processInputEvents = false;
        if (searchText.text.length)
            performSearch();
    }
    Component.onDestruction: {
        fuse.paused = false
        fuse.processInputEvents = true;
    }
}
