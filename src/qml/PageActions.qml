/*
    Copyright (c) 2015-2026, BogDan Vatra <bogdan@kde.org>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import Spectacol

// Secondary page actions stay reachable by touch and D-pad without a shortcut row.
ToolButton {
    id: button
    objectName: "pageActions"
    property var actions: []
    readonly property var availableActions: actions.filter(h => typeof h.action === "function" && h.pad !== "B")
    readonly property real windowWidth: Window.width
    signal dismissed

    visible: availableActions.length > 0
    implicitWidth: Theme.rowHeight
    implicitHeight: Theme.rowHeight
    text: "⋮"
    font.pixelSize: Theme.title
    Accessible.name: qsTr("Page actions")
    onClicked: menu.open()
    function close(): bool {
        if (!menu.visible) return false;
        menu.close();
        return true;
    }
    Keys.onDownPressed: menu.open()
    Menu {
        id: menu
        popupType: Popup.Item
        modal: true
        dim: false
        x: button.width - width
        y: button.height
        width: Math.min(Theme.sp(65), button.windowWidth)
        // Some actions replace the page. Release its popup grab before invoking them.
        exit: Transition {}
        onClosed: {
            button.forceActiveFocus();
            button.dismissed();
        }
        Repeater {
            model: button.availableActions
            delegate: MenuItem {
                required property var modelData
                text: modelData.text
                implicitHeight: Theme.rowHeight
                onTriggered: modelData.action()
            }
        }
    }
}
