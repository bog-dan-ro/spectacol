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

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtGamepadLegacy
import Spectacol

// @scope main.qml

// The menu: a list of ZxMenuItems; an item with children opens them in place
ColumnLayout {
    id: root

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

    // "&Quick Open .." -> "Quick Open"
    function plainText(text: string): string
    {
        return text.replace("&", "").replace(/ *\.\.+ *$/, "").trim();
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
    }

    function pushMenu(menu, index)
    {
        menus = menus.concat([{"menu": currentMenu, "index": index}]);
        currentMenu = menu;
    }

    function popMenu()
    {
        if (menus.length) {
            const menu = menus[menus.length - 1];
            menus = menus.slice(0, -1);
            currentMenu = menu.menu;
            menuView.currentIndex = menu.index;
        } else {
            reset();
            menuBar.close();
        }
    }

    spacing: 0

    // The header: Spectacol and the machine at the top level, a way back in a sub menu
    Item {
        Layout.fillWidth: true
        Layout.preferredHeight: headerColumn.implicitHeight + Theme.sp(10)

        ColumnLayout {
            id: headerColumn
            anchors.fill: parent
            anchors.leftMargin: Theme.sp(6)
            anchors.rightMargin: Theme.sp(6)
            anchors.topMargin: Theme.sp(6)
            anchors.bottomMargin: Theme.sp(4)
            spacing: Theme.sp(1)

            RowLayout {
                spacing: Theme.sp(3)
                // the 128K's rainbow stripe
                Row {
                    visible: root.menus.length === 0
                    Repeater {
                        model: Theme.stripe
                        delegate: Rectangle {
                            required property color modelData
                            width: Theme.sp(2.5)
                            height: Theme.sp(2.5)
                            color: modelData
                            transform: Shear { xFactor: -0.5 }
                        }
                    }
                }
                Text {
                    visible: root.menus.length > 0
                    text: "‹"
                    font.pixelSize: Theme.pageTitle
                    color: Theme.textMuted
                }
                Text {
                    Layout.fillWidth: true
                    text: root.menus.length === 0 || !root.currentMenu ? "Spectacol" : root.plainText(root.currentMenu.text)
                    elide: Text.ElideRight
                    font.pixelSize: Theme.pageTitle
                    font.weight: Font.DemiBold
                    color: Theme.text
                }
            }
            Text {
                Layout.fillWidth: true
                text: (root.menus.length === 0 ? ZxEmulator.settings.currentMachine
                                               : root.menus.map(m => m.menu === root.rootMenu ? "Spectacol" : root.plainText(m.menu.text)).join(" › ")).toUpperCase()
                elide: Text.ElideRight
                font.pixelSize: Theme.caption
                font.letterSpacing: Theme.uiScale
                color: Theme.textMuted
            }
        }

        MouseArea {
            anchors.fill: parent
            enabled: root.menus.length > 0
            onClicked: root.popMenu()
        }

        Rectangle {
            anchors.bottom: parent.bottom
            width: parent.width
            height: Theme.line
            color: Theme.outline
        }
    }

    ListView {
        id: menuView
        clip: true
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.margins: Theme.sp(2)
        spacing: Theme.sp(0.5)
        boundsBehavior: Flickable.StopAtBounds
        highlightFollowsCurrentItem: true
        highlightMoveDuration: Theme.reduceMotion ? 0 : 100
        keyNavigationWraps: true

        model: menuModel

        highlight: Rectangle {
            radius: Theme.radiusSmall
            color: Theme.accentFill
            Rectangle {
                width: Theme.sp(0.75)
                height: parent.height
                radius: width / 2
                color: Theme.accent
            }
        }

        delegate: Item {
            id: menuItem
            required property var modelData
            required property int index
            width: menuView.width
            height: Theme.rowHeight

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.sp(4)
                anchors.rightMargin: Theme.sp(4)
                spacing: Theme.sp(2)
                Text {
                    Layout.fillWidth: true
                    text: root.plainText(menuItem.modelData.text)
                    elide: Text.ElideRight
                    font.pixelSize: Theme.body
                    color: Theme.text
                }
                Text {
                    visible: menuItem.modelData.children.length > 0
                    text: "›"
                    font.pixelSize: Theme.title
                    color: menuItem.index === menuView.currentIndex ? Theme.text : Theme.textMuted
                }
            }

            MouseArea {
                anchors.fill: parent
                hoverEnabled: true
                onEntered: menuView.currentIndex = menuItem.index
                onClicked: root.openMenu(menuItem.modelData, menuItem.index)
            }
        }
    }

    HintBar {
        Layout.fillWidth: true
        hints: [
            { pad: "A", key: "Enter", text: qsTr("Select"), action: () => root.openSelectedMenu() },
            { pad: "B", key: "Esc", text: root.menus.length ? qsTr("Back") : qsTr("Close"), action: () => root.popMenu() },
            { pad: "Menu", key: "F1", text: qsTr("Close"), action: () => menuBar.close() }
        ]
    }

    GamepadKeyNavigation {
        id: gamepadKeyNavigation
        gamepad: Gamepad { deviceId: ZxEmulator.gamepadId }
        active: false
        onActiveChanged: ZxEmulator.processInputEvents = !gamepadKeyNavigation.active
        upKey: Qt.Key_Up
        downKey: Qt.Key_Down
        buttonAKey: Qt.Key_Return
        buttonStartKey: Qt.Key_F1
        buttonBKey: Qt.Key_Escape
    }
}
