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
import QtQuick.Layouts
import Spectacol

// @scope main.qml

// Every page: a sheet over the dimmed game, from the right (from the left for menus), with a
// header (where it is, what it is), the page's content and its hint bar. Escape, or a click
// on the dimmed game, closes it.
FocusScope {
    id: sheet

    property string crumb
    property string title
    property bool fromLeft: false
    // 600 px, at most the window
    property real preferredWidth: Theme.sp(150)
    property alias hints: hintBar.hints
    default property alias content: body.data
    // what the sheet covers, for the messages to stay clear of it
    readonly property real panelWidth: panel.width
    readonly property real hintBarHeight: hintBar.visible ? hintBar.height : 0

    property var closeAction: null
    signal closeRequested
    onCloseRequested: {
        if (closeAction) closeAction();
        else pageLoader.source = "";
    }

    anchors.fill: parent
    function back() {
        const hint = hints.find(h => h.pad === "B" && typeof h.action === "function");
        if (hint) hint.action();
        else closeRequested();
    }
    function closeDialog(): bool {
        if (!closePageActions()) back();
        return true;
    }
    function closePageActions(): bool { return pageActions.close(); }
    Keys.onEscapePressed: closeDialog()

    // Docked next to the screen in the desktop layout: the game stays bright
    readonly property bool docked: Theme.desktopMode && panel.width < width * 0.7

    Rectangle {
        id: scrim
        anchors.fill: parent
        color: sheet.docked ? "transparent" : Theme.scrim
        opacity: 0

        MouseArea {
            anchors.fill: parent
            onClicked: sheet.closeRequested()
        }
    }

    Rectangle {
        id: panel

        property real offset: sheet.fromLeft ? -Theme.slide : Theme.slide

        width: Math.min(sheet.width, sheet.preferredWidth)
        height: sheet.height
        x: (sheet.fromLeft ? 0 : sheet.width - width) + offset
        opacity: 0
        color: Theme.surface

        // Clicks on the sheet stay in it
        MouseArea {
            anchors.fill: parent
        }

        Rectangle {
            x: sheet.fromLeft ? parent.width - width : 0
            width: Theme.line
            height: parent.height
            color: Theme.outline
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            Item {
                id: header
                objectName: "sheetHeader"
                visible: sheet.title !== "" || sheet.crumb !== ""
                Layout.fillWidth: true
                Layout.preferredHeight: Theme.touchScreenUi ? Theme.headerHeight : desktopHeader.implicitHeight + Theme.sp(10)

                ColumnLayout {
                    id: desktopHeader
                    visible: !Theme.touchScreenUi
                    anchors.fill: parent
                    anchors.leftMargin: Theme.sp(6)
                    anchors.rightMargin: Theme.sp(6)
                    anchors.topMargin: Theme.sp(6)
                    anchors.bottomMargin: Theme.sp(4)
                    spacing: Theme.sp(1)
                    Text {
                        visible: text !== ""
                        Layout.fillWidth: true
                        text: sheet.crumb.toUpperCase()
                        elide: Text.ElideRight
                        font.pixelSize: Theme.caption
                        font.letterSpacing: Theme.uiScale
                        color: Theme.textMuted
                    }
                    Text {
                        Layout.fillWidth: true
                        text: sheet.title
                        elide: Text.ElideRight
                        font.pixelSize: Theme.pageTitle
                        font.weight: Font.DemiBold
                        color: Theme.text
                    }
                }
                RowLayout {
                    visible: Theme.touchScreenUi
                    anchors.fill: parent
                    anchors.leftMargin: Theme.sp(1)
                    anchors.rightMargin: Theme.sp(1)
                    spacing: Theme.sp(2)
                    ToolButton {
                        objectName: "sheetBack"
                        text: "‹"
                        font.pixelSize: Theme.pageTitle
                        implicitWidth: Theme.rowHeight
                        implicitHeight: Theme.rowHeight
                        Accessible.name: qsTr("Back")
                        onClicked: sheet.back()
                    }
                    Label {
                        Layout.fillWidth: true
                        text: sheet.title || sheet.crumb
                        elide: Text.ElideRight
                        font.pixelSize: Theme.body
                        font.weight: Font.DemiBold
                        color: Theme.text
                    }
                    PageActions {
                        id: pageActions
                        actions: sheet.hints
                    }
                }
            }

            Rectangle {
                visible: header.visible
                Layout.fillWidth: true
                Layout.preferredHeight: Theme.line
                color: Theme.outline
            }

            Item {
                id: body
                objectName: "sheetBody"
                clip: true
                Layout.fillWidth: true
                Layout.fillHeight: true
            }

            HintBar {
                id: hintBar
                visible: Theme.showHintBar && hints.length > 0
                Layout.fillWidth: true
            }
        }
    }

    // The menu's drawer gives the focus back to the screen when it has closed: take it again
    Component.onCompleted: forceActiveFocus()

    ParallelAnimation {
        running: true
        onFinished: Qt.callLater(() => {
            if (sheet.visible && !sheet.activeFocus)
                sheet.forceActiveFocus();
        })
        NumberAnimation { target: panel; property: "offset"; to: 0; duration: Theme.enterDuration; easing.type: Easing.OutCubic }
        NumberAnimation { target: panel; property: "opacity"; to: 1; duration: Theme.enterDuration; easing.type: Easing.OutCubic }
        NumberAnimation { target: scrim; property: "opacity"; to: 1; duration: Theme.enterDuration; easing.type: Easing.OutCubic }
    }
}
