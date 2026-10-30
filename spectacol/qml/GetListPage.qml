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
import QtQuick.Window
import QtQuick.Controls
import QtGamepadLegacy
import Spectacol

import "private"

Item {
    // FUSE blocks until it gets an answer, so the page must answer however it is
    // closed (Back, F1, a tap outside, opening the menu...), not only from its buttons.
    property bool answered: false
    Component.onCompleted: FuseEmulator.paused = true
    Component.onDestruction: {
        if (!answered)
            FuseEmulator.setListIndex(-1);
        FuseEmulator.paused = false;
    }

    property alias model: view.model
    property string title
    anchors.fill: parent

    function answer(index) {
        answered = true;
        FuseEmulator.setListIndex(index);
        pageLoader.source = "";
    }

    FancyBaseList {
        id: view
        animation: false
        width: title.length * (TextSizes.smallScreen ? TextSizes.scale16 : TextSizes.scale20)
        x: (parent.width - width) / 2
        snapMode: ListView.NoSnap

        GamepadKeyNavigation {
            gamepad: Gamepad { deviceId: FuseEmulator.gamepadId }
        }

        Keys.onEscapePressed: answer(-1)
        Keys.onReturnPressed: answer(view.currentIndex)

        header: FancyText {
            x: parent.x
            font.pixelSize: (TextSizes.smallScreen ? TextSizes.scale16 : TextSizes.scale20)
            text: title
            width: view.width
            horizontalAlignment : Text.AlignHCenter
        }

        footer: Button {
            width: view.width
            text: qsTr("Close (B)")
            onClicked: answer(-1)
        }

        delegate: Rectangle {
            width: view.width
            height: (TextSizes.smallScreen ? TextSizes.scale16 : TextSizes.scale20) * 2
            color: Qt.rgba(0, 0, 0, 0.75);

            FancyText {
                anchors.fill: parent
                anchors.leftMargin: (TextSizes.smallScreen ? TextSizes.scale16 : TextSizes.scale20) / 2
                font.pixelSize: (TextSizes.smallScreen ? TextSizes.scale16 : TextSizes.scale20)
                text: modelData
            }

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    if (view.currentIndex === model.index) {
                        answer(view.currentIndex);
                    } else {
                        view.currentIndex = model.index;
                    }
                }
            }
        }

    }
}
