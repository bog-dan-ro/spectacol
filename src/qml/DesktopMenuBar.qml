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

// The desktop layout's menu bar, built from the same ZxMenu as the overlay's menu: the top level
// actions go into "Spectacol", each top level sub menu becomes a menu. A menu is rebuilt when it
// opens, so it follows the items' visibility (Disks only with drives, SD card on the Next).
MenuBar {
    id: bar

    property ZxMenuItem rootMenu

    function plainText(text: string): string {
        return text.replace("&", "").replace(/ *\.\.+ *$/, "").trim();
    }

    function visibleChildren(item) {
        const result = [];
        for (let i = 0; i < item.children.length; ++i) {
            const child = item.children[i];
            if (child.visible && child.text !== undefined)
                result.push(child);
        }
        return result;
    }

    function fill(menu) {
        (menu.source || rootMenu).aboutToShow();
        for (let i = menu.count - 1; i >= 0; --i) {
            const item = menu.itemAt(i);
            if (item.subMenu)
                menu.removeMenu(item.subMenu);
            else
                menu.removeItem(item);
        }
        const items = menu.source ? visibleChildren(menu.source)
                                  : visibleChildren(rootMenu).filter(c => c.children.length === 0);
        let width = Theme.sp(50);
        for (const child of items) {
            if (child.children.length > 0) {
                menu.addMenu(menuComponent.createObject(menu, { title: plainText(child.text), source: child }));
            } else {
                // "..." opens a page: "…" as desktop menus do
                const item = itemComponent.createObject(menu.contentItem, {
                    text: plainText(child.text) + (child.text.match(/\.\.+ *$/) ? "…" : ""),
                    source: child
                });
                menu.addItem(item);
                width = Math.max(width, item.implicitWidth);
            }
        }
        menu.width = width + Theme.sp(4);
    }

    Component {
        id: menuComponent
        Menu {
            id: menu
            // a ZxMenuItem, or null for the top level actions
            property var source: null
            onAboutToShow: bar.fill(menu)
            background: Rectangle {
                implicitWidth: Theme.sp(50)
                radius: Theme.radiusSmall
                color: Theme.raised
                border.width: Theme.line
                border.color: Theme.outline
            }
        }
    }

    Component {
        id: itemComponent
        MenuItem {
            property var source
            enabled: source ? source.enabled : false
            onTriggered: source.trigger()
        }
    }

    Component.onCompleted: {
        addMenu(menuComponent.createObject(bar, { title: "Spectacol" }));
        for (const child of visibleChildren(rootMenu)) {
            if (child.children.length > 0)
                addMenu(menuComponent.createObject(bar, { title: plainText(child.text), source: child }));
        }
    }
}
