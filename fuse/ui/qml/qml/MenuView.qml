/*
    Copyright (c) 2015, BogDan Vatra <bogdan@kde.org>

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

import QtQuick 2.3
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.1
import QtQuick.Window 2.2
import QtGamepad 1.0

ColumnLayout {
    property Menu rootMenu: null
    property Menu currentMenu: rootMenu
    property var menus: []
    property real parentWidth: 50
    property alias useGamepad: gamepadKeyNavigation.active

    function decrementCurrentIndex()
    {
        menuView.decrementCurrentIndex();
    }

    function incrementCurrentIndex()
    {
        menuView.incrementCurrentIndex();
    }

    function openMenu(menu, index)
    {
        if (menu.type === MenuItemType.Menu)
            pushMenu(menu, index);
        else if (menu.type === MenuItemType.Item)
            menu.trigger();
    }

    function openSelectedMenu()
    {
        if (menuView.currentIndex !== -1)
            openMenu(menuModel.get(menuView.currentIndex).modelData, menuView.currentIndex);
    }

    ListModel {
        id: menuModel
    }

    onCurrentMenuChanged: {
        menuModel.clear();
        if (currentMenu !== null) {
            var i, items = currentMenu.items, len = currentMenu.items.length;
            for (i = 0; i < len; ++i) {
                var item = items[i];
                if (item.visible)
                    menuModel.append({"modelData": item});
            }
            menuView.currentIndex = 0;
        } else {
            menuView.currentIndex = -1;
        }
    }


    function reset()
    {
        currentMenu = null;
        currentMenu = rootMenu;
        menus = [];
        showParentMenu();
    }

    function pushMenu(menu, index)
    {
        menus.push({"menu": currentMenu, "index": index});
        currentMenu = menu;
        showParentMenu();
    }

    function popMenu()
    {
        if (menus.length) {
            var menu = menus.pop();
            currentMenu = menu.menu;
            menuView.currentIndex = menu.index;
            showParentMenu();
        } else {
            reset();
            menuBar.close();
        }
    }

    function showParentMenu()
    {
        title.text = (menus.length > 1 ? "&lt;/../" : "&lt;/") + _menuText(currentMenu.title);
    }

    function _menuText(text)
    {
        var ampersantPos = text.indexOf("&");
        if (ampersantPos === -1)
            return text;
        return text.substr(0, ampersantPos) + "<b><u>" + text.substr(ampersantPos + 1, 1) + "</u></b>" + text.substr(ampersantPos + 2);
    }

    spacing: 0.5 * Screen.pixelDensity

    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 10 * Screen.pixelDensity
        border.width: 0.25 * Screen.pixelDensity
        border.color: "black"
        color: Qt.rgba(0.25, 0.25, 0.25, 0.75);

        FancyText {
            id: title
            anchors.fill: parent
        }

        MouseArea {
            anchors.fill: parent
            onClicked: popMenu()
        }
    }

    ListView {
        id: menuView
        clip: true
        Layout.fillWidth: true
        Layout.fillHeight: true

        snapMode: ListView.SnapToItem
        highlightFollowsCurrentItem: true

        model: menuModel
        delegate: Rectangle {
            id: menuItemRect
            visible: modelData.visible
            width: parentWidth
            height: visible ? 9 * Screen.pixelDensity : 0
            border.width: 0.25 * Screen.pixelDensity
            border.color: (index !== menuView.currentIndex) ? "black" : "lightgreen"
            color: (index !== menuView.currentIndex) ? Qt.rgba(0.25, 0.25, 0.25, 0.75) : Qt.rgba(0.0, 0.85, 0.0, 0.75)

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Screen.pixelDensity
                FancyText {
                    Layout.fillWidth: true
                    height: parent.height
                    fontSize: 4
                    text: modelData.type === MenuItemType.Item ? _menuText(modelData.text) : _menuText(modelData.title)
                }
                FancyText {
                    visible: modelData.type === MenuItemType.Menu
                    fontSize: 5
                    font.bold: true
                    text: modelData.type === MenuItemType.Menu ? ">" : ""
                }
            }

            MouseArea {
                anchors.fill: parent
                onClicked: openMenu(modelData, index)
            }
        }
    }


    GamepadKeyNavigation {
        id: gamepadKeyNavigation
        gamepad: Gamepad { deviceId: fuse.gamepadId }
        active: false
        onActiveChanged: fuse.processJoysticksEvents = !active
        upKey: Qt.Key_Up
        downKey: Qt.Key_Down
        leftKey: Qt.Key_Escape
        rightKey: Qt.Key_Return
        buttonAKey: Qt.Key_Return
        buttonStartKey: Qt.Key_F1
        buttonBKey: Qt.Key_Escape
    }
}
