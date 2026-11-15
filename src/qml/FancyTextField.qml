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
import QtQuick.Templates as T
import Spectacol

// A text field in Theme's style; `mono` for code, addresses and hex. Built on the template,
// not on the application's Controls style: Material's floating placeholder does not fit here.
T.TextField {
    id: field
    property bool mono: false
    implicitWidth: Math.max(Theme.sp(30), contentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(Theme.sp(10), contentHeight + topPadding + bottomPadding)
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
    Text {
        x: field.leftPadding
        y: field.topPadding
        width: field.width - field.leftPadding - field.rightPadding
        height: field.height - field.topPadding - field.bottomPadding
        visible: !field.length && !field.preeditText
        text: field.placeholderText
        font: field.font
        color: field.placeholderTextColor
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
    background: Rectangle {
        radius: Theme.radiusSmall
        color: field.readOnly ? "transparent" : Theme.raised
        border.width: field.activeFocus ? Theme.focusLine : field.readOnly ? 0 : Theme.line
        border.color: field.activeFocus ? Theme.accent : Theme.outline
    }
}
