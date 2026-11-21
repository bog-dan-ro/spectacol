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

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import Spectacol

Button {
    id: button

    implicitWidth: Math.max(Theme.sp(24), implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(Theme.rowHeight, implicitContentHeight + topPadding + bottomPadding)
    horizontalPadding: Theme.sp(3)
    leftPadding: horizontalPadding
    rightPadding: horizontalPadding
    verticalPadding: Theme.sp(2)
    topInset: 0
    bottomInset: 0
    leftInset: 0
    rightInset: 0

    background: Rectangle {
        radius: Theme.radiusSmall
        color: button.activeFocus ? Theme.accent
             : button.down || button.hovered ? Theme.accentFill : "transparent"
        border.width: Theme.line
        border.color: button.activeFocus ? Theme.accent : Theme.outline
    }
    contentItem: Text {
        text: button.text
        textFormat: Text.PlainText
        wrapMode: Text.Wrap
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        font.pixelSize: Theme.body
        font.weight: Font.DemiBold
        color: !button.enabled ? Theme.textMuted
             : button.activeFocus ? Theme.textOnAccent : Theme.text
    }
    Keys.onPressed: event => {
        if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter || event.key === Qt.Key_Select) {
            if (!event.isAutoRepeat)
                button.click();
            event.accepted = true;
        }
    }
    Accessible.name: text
}
