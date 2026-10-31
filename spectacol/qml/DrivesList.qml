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
import QtQuick.Controls
import QtGamepadLegacy
import Spectacol
import "private"

// Lets the user pick one of the machine's floppy drives (A:, B:, ...)
FancyBaseList {
    id: view
    property string title: qsTr("Select drive")
    signal driveSelected(int drive)

    animation: false
    width: 13 * (TextSizes.smallScreen ? TextSizes.scale12 : TextSizes.scale14)
    model: ZxEmulator.disks.count

    GamepadKeyNavigation {
        gamepad: Gamepad { deviceId: ZxEmulator.gamepadId }
    }

    Keys.onEscapePressed: pageLoader.source = ""
    Keys.onReturnPressed: driveSelected(currentIndex)

    header: FancyText {
        width: view.width
        font.pixelSize: (TextSizes.smallScreen ? TextSizes.scale14 : TextSizes.scale16)
        horizontalAlignment: Text.AlignHCenter
        text: view.title
    }

    delegate: Rectangle {
        width: view.width
        height: (TextSizes.smallScreen ? TextSizes.scale12 : TextSizes.scale14) * 2
        color: Qt.rgba(0, 0, 0, 0.75)

        FancyText {
            anchors.fill: parent
            anchors.leftMargin: (TextSizes.smallScreen ? TextSizes.scale12 : TextSizes.scale14) / 2
            font.pixelSize: (TextSizes.smallScreen ? TextSizes.scale12 : TextSizes.scale14)
            text: qsTr("Drive %1:").arg(String.fromCharCode(65 + model.index))
        }
        MouseArea {
            anchors.fill: parent
            onClicked: view.driveSelected(model.index)
        }
    }

    footer: Button {
        width: view.width
        text: qsTr("Close (B)")
        onClicked: pageLoader.source = ""
    }
}
