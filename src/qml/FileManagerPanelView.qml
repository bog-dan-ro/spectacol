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

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Spectacol

// One panel of the file manager: where it is, the entries, and what the cursor is on
FocusScope {
    id: view

    required property FileManagerPanel panel
    property bool active: false
    property alias currentIndex: list.currentIndex
    // The current row's, set by its delegate
    property string currentName
    property string currentKind
    property string currentInfo
    // A refresh keeps the cursor on the same entry, or at the same place
    property string resetName
    property int resetIndex: 0
    readonly property bool wide: width > Theme.sp(110)
    signal clicked
    signal openRequested

    function select(row: int) {
        list.currentIndex = Math.max(0, Math.min(row, list.count - 1));
        list.positionViewAtIndex(list.currentIndex, ListView.Contain);
    }

    function sizeText(bytes: real): string {
        if (bytes < 0)
            return "";
        if (bytes < 100000)
            return String(bytes);
        if (bytes < 10 * 1024 * 1024)
            return qsTr("%1K").arg(Math.round(bytes / 1024));
        return qsTr("%1M").arg(Math.round(bytes / (1024 * 1024)));
    }

    function tag(kind: string): string {
        switch (kind) {
        case "parent": return qsTr("UP");
        case "folder": return qsTr("DIR");
        case "tape": return qsTr("TAP");
        case "disk": return qsTr("DSK");
        case "program": return qsTr("PRG");
        case "snapshot": return qsTr("SNA");
        case "recording": return qsTr("RZX");
        case "basic": return qsTr("BAS");
        case "code": return qsTr("BIN");
        case "screen": return qsTr("SCR");
        case "array": return qsTr("ARR");
        case "block": return qsTr("BLK");
        case "text": return qsTr("TXT");
        }
        return "";
    }

    function tagColor(kind: string): color {
        switch (kind) {
        case "parent":
        case "folder": return Theme.accent;
        case "tape":
        case "disk":
        case "program": return Theme.warning;
        case "basic":
        case "snapshot": return Theme.success;
        case "screen":
        case "code": return Theme.padX;
        }
        return Theme.textMuted;
    }

    Connections {
        target: view.panel
        function onModelAboutToBeReset() {
            view.resetName = view.currentName;
            view.resetIndex = list.currentIndex;
        }
        function onModelReset() {
            const row = view.panel.rowOf(view.resetName);
            view.select(row >= 0 ? row : view.resetIndex);
        }
    }

    Rectangle {
        anchors.fill: parent
        radius: Theme.radiusSmall
        color: Theme.raised
        border.width: view.active ? Theme.focusLine : Theme.line
        border.color: view.active ? Theme.accent : Theme.outline
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.sp(2)
        spacing: Theme.sp(1)

        FancyText {
            objectName: "fileManagerLocation"
            Layout.fillWidth: true
            Layout.leftMargin: Theme.sp(2)
            Layout.rightMargin: Theme.sp(2)
            text: view.panel.location
            elide: Text.ElideMiddle
            font.weight: Font.DemiBold
            color: view.active ? Theme.accent : Theme.text
        }

        ListView {
            id: list
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            focus: true
            model: view.panel
            boundsBehavior: Flickable.StopAtBounds
            highlightMoveDuration: 0
            keyNavigationEnabled: true
            ScrollIndicator.vertical: ScrollIndicator {}
            highlight: Rectangle {
                visible: view.active
                radius: Theme.radiusSmall
                color: Theme.accentFill
                border.width: Theme.focusLine
                border.color: Theme.accent
            }

            delegate: Item {
                id: entry
                required property int index
                required property string name
                required property string label
                required property string kind
                required property string info
                required property real size
                required property bool marked
                width: list.width
                height: Theme.listRowHeight

                Rectangle {
                    anchors.fill: parent
                    visible: entry.marked
                    radius: Theme.radiusSmall
                    color: Qt.alpha(Theme.warning, 0.16)
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: Theme.sp(2)
                    anchors.rightMargin: Theme.sp(2)
                    spacing: Theme.sp(2)

                    Text {
                        Layout.preferredWidth: Theme.caption * 2.4
                        text: view.tag(entry.kind)
                        font.family: Theme.monoFamily
                        font.pixelSize: Theme.caption
                        font.weight: Font.DemiBold
                        color: view.tagColor(entry.kind)
                    }
                    Text {
                        Layout.fillWidth: true
                        text: entry.label
                        elide: Text.ElideMiddle
                        font.pixelSize: Theme.body
                        font.weight: entry.kind === "folder" || entry.kind === "parent" ? Font.DemiBold : Font.Normal
                        color: entry.marked ? Theme.warning : Theme.text
                    }
                    Text {
                        visible: view.wide && entry.info !== ""
                        Layout.maximumWidth: view.width * 0.4
                        text: entry.info
                        elide: Text.ElideRight
                        font.pixelSize: Theme.caption
                        color: Theme.textMuted
                    }
                    Text {
                        Layout.minimumWidth: Theme.caption * 4
                        horizontalAlignment: Text.AlignRight
                        text: view.sizeText(entry.size)
                        font.family: Theme.monoFamily
                        font.pixelSize: Theme.caption
                        color: Theme.textMuted
                    }
                }

                // A finger opens with one tap (a long press marks); the mouse selects, and
                // opens with a double click
                TapHandler {
                    onTapped: (eventPoint, button) => {
                        list.currentIndex = entry.index;
                        if (eventPoint.device.type === PointerDevice.TouchScreen)
                            view.openRequested();
                        else
                            view.clicked();
                    }
                    onDoubleTapped: (eventPoint, button) => {
                        if (eventPoint.device.type === PointerDevice.TouchScreen)
                            return;
                        list.currentIndex = entry.index;
                        view.openRequested();
                    }
                    onLongPressed: {
                        list.currentIndex = entry.index;
                        view.clicked();
                        view.panel.toggleMark(entry.index);
                    }
                }
                Binding {
                    when: entry.ListView.isCurrentItem
                    target: view
                    property: "currentName"
                    value: entry.name
                    restoreMode: Binding.RestoreNone
                }
                Binding {
                    when: entry.ListView.isCurrentItem
                    target: view
                    property: "currentKind"
                    value: entry.kind
                    restoreMode: Binding.RestoreNone
                }
                Binding {
                    when: entry.ListView.isCurrentItem
                    target: view
                    property: "currentInfo"
                    value: entry.info
                    restoreMode: Binding.RestoreNone
                }
            }
        }

        FancyText {
            Layout.fillWidth: true
            Layout.leftMargin: Theme.sp(2)
            Layout.rightMargin: Theme.sp(2)
            elide: Text.ElideRight
            font.pixelSize: Theme.caption
            color: Theme.textMuted
            text: {
                if (view.panel.markedCount > 0)
                    return qsTr("%n marked", "", view.panel.markedCount);
                const parts = [];
                if (view.panel.inMedia)
                    parts.push(view.panel.filesystem);
                if (!view.panel.writable)
                    parts.push(qsTr("read only"));
                if (view.currentInfo)
                    parts.push(view.currentInfo);
                return parts.join(" · ");
            }
        }
    }
}
