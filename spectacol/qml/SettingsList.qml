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

// A scrolling column of SettingRows (and any other items): Up and Down move the focus from
// row to row and keep it in view; the first row has it at the start.
Flickable {
    id: list

    default property alias rows: column.data

    anchors.fill: parent
    contentHeight: column.implicitHeight + 2 * Theme.sp(2)
    clip: true
    boundsBehavior: Flickable.StopAtBounds
    focus: true
    ScrollIndicator.vertical: ScrollIndicator {}

    function settingRows() {
        const rows = [];
        for (let i = 0; i < column.children.length; ++i) {
            const child = column.children[i];
            if (child.visible && child.enabled && child.isSettingRow === true)
                rows.push(child);
        }
        return rows;
    }

    function move(delta: int) {
        const rows = settingRows();
        if (!rows.length)
            return;
        let index = rows.findIndex(r => r.activeFocus);
        index = Math.max(0, Math.min(rows.length - 1, index < 0 ? 0 : index + delta));
        rows[index].forceActiveFocus(Qt.TabFocusReason);
        ensureVisible(rows[index]);
    }

    function ensureVisible(item) {
        const y = item.mapToItem(column, 0, 0).y + column.y;
        if (y < contentY)
            contentY = Math.max(0, y - Theme.sp(2));
        else if (y + item.height > contentY + height)
            contentY = Math.min(contentHeight - height, y + item.height - height + Theme.sp(2));
    }

    Keys.onUpPressed: move(-1)
    Keys.onDownPressed: move(1)

    Column {
        id: column
        x: Theme.sp(3)
        y: Theme.sp(2)
        width: list.width - 2 * Theme.sp(3)
    }

    Component.onCompleted: Qt.callLater(() => move(0))
}
