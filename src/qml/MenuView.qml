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
import QtQuick.Controls
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

    function focusMenu() { menuView.forceActiveFocus(Qt.PopupFocusReason); }

    function decrementCurrentIndex()
    {
        moveCurrentIndex(-1);
    }

    function incrementCurrentIndex()
    {
        moveCurrentIndex(1);
    }

    function moveCurrentIndex(step)
    {
        for (let i = 0; i < menuModel.count; ++i) {
            menuView.currentIndex = (menuView.currentIndex + step + menuModel.count) % menuModel.count;
            if (menuModel.get(menuView.currentIndex).modelData.enabled)
                return;
        }
    }

    function openMenu(menu, index)
    {
        if (!menu.enabled)
            return;
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
            currentMenu.aboutToShow();
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
            focusMenu();
        } else {
            reset();
            menuBar.close();
        }
    }

    spacing: 0

    // The header: Spectacol and the machine at the top level, a way back in a sub menu
    Item {
        objectName: "menuHeader"
        Layout.fillWidth: true
        Layout.preferredHeight: Theme.touchScreenUi ? Theme.headerHeight : headerColumn.implicitHeight + Theme.sp(10)

        ColumnLayout {
            id: headerColumn
            anchors.fill: parent
            anchors.leftMargin: Theme.sp(Theme.touchScreenUi ? 1 : 6)
            anchors.rightMargin: Theme.sp(Theme.touchScreenUi ? 1 : 6)
            anchors.topMargin: Theme.touchScreenUi ? 0 : Theme.sp(6)
            anchors.bottomMargin: Theme.touchScreenUi ? 0 : Theme.sp(4)
            spacing: Theme.sp(1)

            RowLayout {
                spacing: Theme.sp(3)
                ToolButton {
                    visible: Theme.touchScreenUi
                    objectName: "menuBack"
                    text: root.menus.length ? "‹" : "×"
                    font.pixelSize: Theme.title
                    implicitWidth: Theme.rowHeight
                    implicitHeight: Theme.rowHeight
                    Accessible.name: root.menus.length ? qsTr("Back") : qsTr("Close menu")
                    onClicked: root.popMenu()
                }
                // the 128K's rainbow stripe
                Row {
                    visible: !Theme.touchScreenUi && root.menus.length === 0
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
                    visible: !Theme.touchScreenUi && root.menus.length > 0
                    text: "‹"
                    font.pixelSize: Theme.touchScreenUi ? Theme.body : Theme.pageTitle
                    color: Theme.textMuted
                }
                Text {
                    Layout.fillWidth: true
                    text: root.menus.length === 0 || !root.currentMenu ? "Spectacol" : root.plainText(root.currentMenu.text)
                    Accessible.role: Accessible.StaticText
                    Accessible.name: text
                    elide: Text.ElideRight
                    font.pixelSize: Theme.touchScreenUi ? Theme.body : Theme.pageTitle
                    font.weight: Font.DemiBold
                    color: Theme.text
                }
            }
            Text {
                visible: !Theme.touchScreenUi
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
            enabled: !Theme.touchScreenUi && root.menus.length > 0
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
        focus: true
        clip: true
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.margins: Theme.sp(2)
        spacing: Theme.sp(0.5)
        boundsBehavior: Flickable.StopAtBounds
        highlightFollowsCurrentItem: true
        highlightMoveDuration: Theme.reduceMotion ? 0 : 100
        keyNavigationWraps: true
        Keys.onReturnPressed: event => { event.accepted = true; }
        Keys.onEnterPressed: event => { event.accepted = true; }
        Keys.onEscapePressed: root.popMenu()
        // Activate on release, as the main window does. Opening a page on
        // press lets its closing drawer route the release to that page again.
        Keys.onReleased: event => {
            if ([Qt.Key_Select, Qt.Key_Return, Qt.Key_Enter].includes(event.key)) {
                event.accepted = true;
                root.openSelectedMenu();
            }
        }
        Keys.onPressed: event => {
            if (event.key === Qt.Key_Down) root.incrementCurrentIndex();
            else if (event.key === Qt.Key_Up) root.decrementCurrentIndex();
            else if (event.key === Qt.Key_Select) event.accepted = true;
            else if (event.key === Qt.Key_Left || event.key === Qt.Key_Back) root.popMenu();
            else if (event.key === Qt.Key_Right) {
                if (menuView.currentIndex >= 0 && menuView.currentIndex < menuModel.count
                        && menuModel.get(menuView.currentIndex).modelData.children.length)
                    root.openSelectedMenu();
            }
            else if (event.key === Qt.Key_F5 || event.key === Qt.Key_Menu) menuBar.close();
            else { event.accepted = false; return; }
            event.accepted = true;
        }

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
            enabled: modelData.enabled
            objectName: "menuItem" + index
            width: menuView.width
            height: Theme.rowHeight
            Accessible.role: Accessible.MenuItem
            Accessible.name: root.plainText(modelData.text)
            Accessible.selected: index === menuView.currentIndex
            Accessible.onPressAction: root.openMenu(modelData, index)

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
                    color: menuItem.enabled ? Theme.text : Theme.textMuted
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
                hoverEnabled: !Theme.touchScreenUi
                onEntered: menuView.currentIndex = menuItem.index
                onClicked: root.openMenu(menuItem.modelData, menuItem.index)
            }
        }
    }

    HintBar {
        visible: Theme.showHintBar
        Layout.fillWidth: true
        hints: [
            { pad: "A", key: "Enter", text: qsTr("Select"), action: () => root.openSelectedMenu() },
            { pad: "B", key: "Esc", text: root.menus.length ? qsTr("Back") : qsTr("Close"), action: () => root.popMenu() },
            { pad: "Menu", key: "F5", text: qsTr("Close"), action: () => menuBar.close() }
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
        buttonStartKey: Qt.Key_F5
        buttonBKey: Qt.Key_Escape
    }
}
