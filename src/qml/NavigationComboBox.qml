/*
    Copyright (c) 2015-2026, BogDan Vatra <bogdan@kde.org>

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

import QtQuick
import QtQuick.Controls
import Spectacol

ComboBox {
    popup.popupType: Popup.Item
    // Release the popup grab as soon as a selection changes the page below it.
    popup.exit: Transition {}
    Keys.onPressed: event => {
        if (!popup.visible && [Qt.Key_Up, Qt.Key_Down, Qt.Key_Left, Qt.Key_Right].includes(event.key)) {
            ZxEmulator.sendNavigationKey(event.key === Qt.Key_Up || event.key === Qt.Key_Left ? Qt.Key_Backtab : Qt.Key_Tab);
            event.accepted = true;
        }
    }
}
