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
import QtQuick.Controls.Material
import QtQuick.Layouts
import Spectacol

Dialog {
    id: dialog
    default property alias fields: form.data
    property Item initialFocus
    property Item returnFocus
    property string errorMessage
    property bool acceptable: true
    property var commit: null
    parent: Overlay.overlay
    popupType: Popup.Item
    anchors.centerIn: parent
    width: Math.min(parent.width - Theme.sp(6), Theme.sp(110))
    height: Math.min(implicitHeight, parent.height - Theme.sp(6))
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape
    function save() {
        if (acceptable && (!commit || commit()))
            accept();
    }
    onAboutToShow: errorMessage = ""
    onOpened: {
        if (initialFocus)
            initialFocus.forceActiveFocus(Qt.TabFocusReason);
    }
    onClosed: {
        Qt.inputMethod.hide();
        if (returnFocus && returnFocus.visible && returnFocus.enabled)
            returnFocus.forceActiveFocus(Qt.BacktabFocusReason);
    }
    property Connections messages: Connections {
        target: ZxEmulator
        function onError(level: int, message: string) {
            if (dialog.visible)
                dialog.errorMessage = message;
        }
    }
    background: Rectangle {
        color: Theme.raised
        radius: Theme.radius
        border.color: Theme.outline
    }
    contentItem: ScrollView {
        implicitHeight: form.implicitHeight
        contentWidth: availableWidth
        clip: true
        ColumnLayout {
            id: form
            width: parent.width
            Label {
                Layout.fillWidth: true
                visible: dialog.errorMessage.length > 0
                text: dialog.errorMessage
                color: Theme.danger
                wrapMode: Text.Wrap
            }
            Keys.onPressed: event => {
                if ([Qt.Key_Up, Qt.Key_Down, Qt.Key_Left, Qt.Key_Right].includes(event.key)) {
                    ZxEmulator.sendNavigationKey(event.key === Qt.Key_Up || event.key === Qt.Key_Left ? Qt.Key_Backtab : Qt.Key_Tab);
                    event.accepted = true;
                }
            }
        }
    }
    footer: RowLayout {
        Item { Layout.fillWidth: true }
        Button {
            objectName: "formSave"
            text: qsTr("Save")
            enabled: dialog.acceptable
            onClicked: dialog.save()
        }
        Button {
            objectName: "formCancel"
            text: Theme.gamepadHints ? qsTr("Cancel (B)") : qsTr("Cancel")
            onClicked: dialog.reject()
        }
        Keys.onPressed: event => {
            if ([Qt.Key_Up, Qt.Key_Down, Qt.Key_Left, Qt.Key_Right].includes(event.key)) {
                ZxEmulator.sendNavigationKey(event.key === Qt.Key_Up || event.key === Qt.Key_Left ? Qt.Key_Backtab : Qt.Key_Tab);
                event.accepted = true;
            }
        }
    }
}
