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
import QtQuick.Controls.Material
import QtQuick.Layouts
import Spectacol

FocusScope {
    id: pane
    readonly property bool dialogOpen: addDialog.visible
    function closeDialog(): bool {
        if (kind.popup.visible) {
            kind.popup.close();
            return true;
        }
        if (!addDialog.visible)
            return false;
        addDialog.reject();
        return true;
    }
    ColumnLayout {
        anchors.fill: parent
        RowLayout {
            Button {
                id: addButton
                objectName: "breakpointAdd"
                text: qsTr("Add breakpoint")
                onClicked: addDialog.open()
            }
            Button {
                text: qsTr("Remove all")
                enabled: view.count > 0
                onClicked: ZxEmulator.debuggerCommand("delete")
            }
        }
        Label {
            Layout.fillWidth: true
            text: qsTr("Memory watches also cover bank changes. IRQ/NMI traps stop after interrupt acceptance.")
            color: Theme.textMuted
            wrapMode: Text.Wrap
        }
        ListView {
            id: view
            objectName: "debuggerBreakpoints"
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            focus: true
            activeFocusOnTab: true
            model: ZxEmulator.breakpointsModel
            Keys.onReturnPressed: {
                if (currentItem)
                    ZxEmulator.enableBreakpoint(currentItem.breakpointId, !currentItem.active);
            }
            Keys.onUpPressed: {
                if (currentIndex > 0)
                    decrementCurrentIndex();
                else
                    ZxEmulator.sendNavigationKey(Qt.Key_Backtab);
            }
            Keys.onDownPressed: {
                if (currentIndex < count - 1)
                    incrementCurrentIndex();
                else
                    ZxEmulator.sendNavigationKey(Qt.Key_Tab);
            }
            delegate: ItemDelegate {
                id: row
                required property int index
                required property int breakpointId
                required property string type
                required property string value
                required property string life
                required property bool active
                required property double hits
                width: view.width
                highlighted: view.currentIndex === index
                onClicked: {
                    view.currentIndex = index;
                    view.forceActiveFocus();
                }
                contentItem: RowLayout {
                    CheckBox {
                        checked: row.active
                        onClicked: ZxEmulator.enableBreakpoint(row.breakpointId, checked)
                        Accessible.name: qsTr("Enable breakpoint %1").arg(row.breakpointId)
                    }
                    Label {
                        Layout.fillWidth: true
                        text: "#" + row.breakpointId + " " + row.type + " " + row.value + " · " + row.life + qsTr(" · hits %1").arg(row.hits)
                        wrapMode: Text.Wrap
                        color: Theme.text
                        font.family: "monospace"
                    }
                    Button {
                        text: qsTr("Remove")
                        onClicked: ZxEmulator.debuggerCommand("delete " + row.breakpointId)
                    }
                }
            }
            ScrollBar.vertical: ScrollBar {}
        }
    }
    FormDialog {
        id: addDialog
        title: qsTr("Add breakpoint / watchpoint")
        initialFocus: kind
        returnFocus: addButton
        acceptable: address.acceptableInput && mask.acceptableInput
        commit: () => ZxEmulator.createBreakpoint(kind.currentIndex, address.text, mask.text, once.checked)
        NavigationComboBox {
            id: kind
            objectName: "breakpointType"
            Layout.fillWidth: true
            model: [qsTr("Execute"), qsTr("Memory read"), qsTr("Memory write"), qsTr("Port read"), qsTr("Port write"), qsTr("Next register write"), qsTr("Raster line"), qsTr("Interrupt event")]
        }
        TextField {
            id: address
            objectName: "breakpointAddress"
            Layout.fillWidth: true
            placeholderText: kind.currentIndex === 7 ? qsTr("0 = IRQ, 1 = NMI") : kind.currentIndex === 6 ? qsTr("Decimal scanline") : qsTr("Hex address / port / register")
            inputMethodHints: kind.currentIndex >= 6 ? Qt.ImhDigitsOnly : Qt.ImhPreferUppercase | Qt.ImhNoPredictiveText
            validator: RegularExpressionValidator {
                regularExpression: kind.currentIndex >= 6 ? /[0-9]{1,4}/ : /(?:\$|0[xX])?[0-9a-fA-F]{1,4}/
            }
            onAccepted: mask.forceActiveFocus(Qt.TabFocusReason)
        }
        TextField {
            id: mask
            objectName: "breakpointMask"
            text: "FFFF"
            Layout.fillWidth: true
            placeholderText: qsTr("Hex mask")
            inputMethodHints: Qt.ImhPreferUppercase | Qt.ImhNoPredictiveText
            validator: RegularExpressionValidator { regularExpression: /(?:\$|0[xX])?[0-9a-fA-F]{1,4}/ }
            onAccepted: addDialog.save()
        }
        CheckBox {
            id: once
            text: qsTr("One-shot")
        }
        Label {
            Layout.fillWidth: true
            text: qsTr("Masks match (value & mask). Use the memory view to watch a physical bank byte.")
            color: Theme.textMuted
            wrapMode: Text.Wrap
        }
    }
}
