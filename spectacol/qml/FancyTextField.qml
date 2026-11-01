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

import QtQuick
import QtQuick.Controls
import Spectacol

// A text field in Theme's style; `mono` for code, addresses and hex
TextField {
    id: field
    property bool mono: false
    font.family: mono ? Theme.monoFamily : Qt.application.font.family
    font.pixelSize: Theme.body
    color: Theme.text
    placeholderTextColor: Theme.textMuted
    selectionColor: Theme.accent
    selectedTextColor: Theme.textOnAccent
    leftPadding: Theme.sp(3)
    rightPadding: Theme.sp(3)
    topPadding: Theme.sp(2)
    bottomPadding: Theme.sp(2)
    horizontalAlignment: Text.AlignLeft
    verticalAlignment: Text.AlignVCenter
    inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText
    background: Rectangle {
        implicitHeight: Theme.sp(10)
        radius: Theme.radiusSmall
        color: field.readOnly ? "transparent" : Theme.raised
        border.width: field.activeFocus ? Theme.focusLine : field.readOnly ? 0 : Theme.line
        border.color: field.activeFocus ? Theme.accent : Theme.outline
    }
}
