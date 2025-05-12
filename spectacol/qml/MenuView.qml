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
import QtQuick.Layouts
import QtQuick.Window
import QtGamepadLegacy
import Spectacol
import "Utils.js" as Utils
import "private"

ColumnLayout {
    property ZxMenuItem rootMenu: null
    property ZxMenuItem currentMenu: rootMenu
    property var menus: []
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
        if (menu.children.length)
            pushMenu(menu, index);
        else
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
            let items = currentMenu.children, len = currentMenu.children.length;
            for (let i = 0; i < len; ++i) {
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
        title.text = (menus.length > 1 ? "&lt;/../" : "&lt;/") + Utils.shortcutText(currentMenu.text);
    }

    spacing: 0.5 * Screen.pixelDensity

    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: (TextSizes.smallScreen ? TextSizes.scale16 : TextSizes.scale20) * 2
        border.width: 0.25 * Screen.pixelDensity
        border.color: "black"
        color: Qt.rgba(0.25, 0.25, 0.25, 0.75);

        FancyText {
            id: title
            font.pixelSize: TextSizes.scale16
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
            width: menuView.width
            height: visible ? TextSizes.scale20 * 2 : 0
            border.width: 0.25 * Screen.pixelDensity
            border.color: (index !== menuView.currentIndex) ? "black" : "lightgreen"
            color: (index !== menuView.currentIndex) ? Qt.rgba(0.25, 0.25, 0.25, 0.75) : Qt.rgba(0.0, 0.85, 0.0, 0.75)

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: TextSizes.scale16 / 4
                anchors.rightMargin: TextSizes.scale16 / 4
                FancyText {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    height: parent.height
                    font.pixelSize: TextSizes.scale16
                    text: Utils.shortcutText(modelData.text)
                }
                FancyText {
                    Layout.fillHeight: true
                    visible: modelData.children.length > 0
                    font.pixelSize: TextSizes.scale20
                    font.bold: true
                    text: modelData.children.length > 0 ? ">" : ""
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
        gamepad: Gamepad { deviceId: FuseEmulator.gamepadId }
        active: false
        onActiveChanged: FuseEmulator.processInputEvents = !active
        upKey: Qt.Key_Up
        downKey: Qt.Key_Down
        buttonAKey: Qt.Key_Return
        buttonStartKey: Qt.Key_F1
        buttonBKey: Qt.Key_Escape
    }
}
