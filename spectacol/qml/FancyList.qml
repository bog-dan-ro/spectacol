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

FancyBaseList {
    id: view

    GamepadKeyNavigation {
        gamepad: Gamepad { deviceId: FuseEmulator.gamepadId }
    }

    Keys.onLeftPressed: decrementCurrentIndex()
    Keys.onRightPressed: incrementCurrentIndex()
    Keys.onEscapePressed: pageLoader.source = ""
    Keys.onReturnPressed: pageLoader.source = ""

    footer: Button {
        width: view.width
        text: qsTr("Close (B)")
        onClicked: pageLoader.source = ""
    }
}
