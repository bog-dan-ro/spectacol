import QtQuick 2.3
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.1
import QtQuick.Window 2.2

Column {
    property Menu rootMenu: null
    property Menu currentMenu: rootMenu
    property var menus: []
    property real parentWidth: 50

    function reset()
    {
        currentMenu = rootMenu;
        menus = [];
        showParentMenu();
    }

    function pushMenu(menu)
    {
        menus.push(currentMenu);
        currentMenu = menu;
        showParentMenu();
    }

    function popMenu()
    {
        if (menus.length) {
            var menu = menus.pop();
            currentMenu = menu;
            showParentMenu();
        }
    }

    function showParentMenu()
    {
        title.text = menus.length > 1 ? "&lt;/../" : "&lt;/" + _menuText(currentMenu.title);
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
        width: parentWidth
        height: 10 * Screen.pixelDensity
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

    Repeater {
        id: repeater
        model: currentMenu.items
        delegate: Rectangle {
            visible: modelData.visible
            id: menuItemRect
            width: parentWidth
            height: 9 * Screen.pixelDensity
            border.width: 0.25 * Screen.pixelDensity
            border.color: "black"
            color: Qt.rgba(0.25, 0.25, 0.25, 0.75);

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
                onClicked: {
                    if (modelData.type === MenuItemType.Menu) {
                        pushMenu(modelData);
                    } else if (modelData.type === MenuItemType.Item) {
                        modelData.trigger();
                    }
                }
            }
        }
    }
}
