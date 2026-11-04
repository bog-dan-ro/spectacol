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
import Spectacol

FocusScope {
    id: pane
    required property DebuggerStateModel stateModel
    property string unavailableText: ""
    property int addressStep: 1
    function goTo(address: int) {
        view.currentIndex = Math.max(0, Math.min(view.count - 1, Math.floor(address / addressStep)));
        view.positionViewAtIndex(view.currentIndex, ListView.Contain);
    }
    ColumnLayout {
        anchors.fill: parent
        Label {
            Layout.fillWidth: true
            text: qsTr("Paused state · inspection does not read emulated I/O ports")
            color: Theme.textMuted
            wrapMode: Text.Wrap
        }
        Label {
            Layout.fillWidth: true
            visible: !pane.stateModel.available
            text: pane.unavailableText
            color: Theme.text
            wrapMode: Text.Wrap
        }
        ListView {
            id: view
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            focus: true
            activeFocusOnTab: true
            model: pane.stateModel
            delegate: ItemDelegate {
                id: row
                required property int index
                required property string addressText
                required property string valueText
                required property string details
                width: view.width
                text: addressText + "   " + valueText + (details.length ? "   " + details : "")
                font.family: "monospace"
                highlighted: view.currentIndex === index
                onClicked: {
                    view.currentIndex = index;
                    view.forceActiveFocus();
                }
                contentItem: Label {
                    text: row.text
                    color: Theme.text
                    wrapMode: Text.Wrap
                    font: row.font
                }
            }
            ScrollBar.vertical: ScrollBar {}
        }
    }
}
