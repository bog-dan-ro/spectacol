import QtQuick 2.3
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.1
import QtQuick.Window 2.2

ColumnLayout {
    property Menu rootMenu: null
    property Menu currentMenu: rootMenu
    property var menus: []
    property real parentWidth: 50

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
            var currentIndex = -1;
            for (i = 0; i < len; ++i) {
                var item = items[i];
                if (currentIndex == -1 && item.visible)
                    currentIndex = i;
                menuModel.append({"modelData": item});
            }
            menuView.currentIndex = currentIndex;
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
            menuBar.open = false;
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
        return text.substr(0, ampersantPos) + "<b>" + text.substr(ampersantPos + 1, 1) + "</b>" + text.substr(ampersantPos + 2);
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
                onClicked: openMenu(menuData, index)
            }
        }
    }
}
