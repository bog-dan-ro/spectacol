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
import QtGamepadLegacy
import Spectacol

Sheet {
    crumb: qsTr("Joystick")
    title: qsTr("Choose a joystick")
    preferredWidth: Theme.sp(100)
    hints: [
        { pad: "A", key: "Enter", text: qsTr("Choose"), action: () => list.chosen(list.currentIndex) },
        { pad: "B", key: "Esc", text: qsTr("Back"), action: () => pageLoader.source = "" }
    ]

    GamepadNavigation {
        gamepad: Gamepad { deviceId: ZxEmulator.gamepadId }
        buttonAKey: Qt.Key_Return
        buttonBKey: Qt.Key_Escape
    }

    OptionList {
        id: list
        model: ZxEmulator.joysticksModel
        checkedIndex: ZxEmulator.selectedJoysticksIndex
        onChosen: (index) => {
            ZxEmulator.selectedJoysticksIndex = index;
            if (ZxEmulator.joystickAvailable(index))
                pageLoader.source = "";
        }
    }
}
