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
    readonly property DebuggerMemoryModel memory: ZxEmulator.debugMemory
    readonly property bool dialogOpen: editor.visible || bank.popup.visible
    function closeDialog(): bool {
        if (bank.popup.visible) {
            bank.popup.close();
            return true;
        }
        if (!editor.visible)
            return false;
        editor.reject();
        return true;
    }
    function focusAddress() { location.forceActiveFocus(Qt.TabFocusReason); }
    function edit(opener: Item) {
        if (memory.editable) {
            byteValue.text = memory.byteValue.toString(16).toUpperCase();
            byteValue.selectAll();
            editor.returnFocus = opener || bytesView;
            romEdit.checked = false;
            editor.open();

        }
    }
    ColumnLayout {
        anchors.fill: parent
        RowLayout {
            Layout.fillWidth: true
            NavigationComboBox {
                id: bank
                objectName: "debuggerBank"
                Layout.fillWidth: true
                model: pane.memory.bankNames
                currentIndex: pane.memory.bankId < 0 ? 0 : pane.memory.bankId
                onActivated: pane.memory.bankId = currentIndex === 0 ? -1 : currentIndex
            }
            TextField {
                id: location
                objectName: "memoryAddress"
                Layout.preferredWidth: Theme.sp(25)
                placeholderText: qsTr("Hex address")
                inputMethodHints: Qt.ImhPreferUppercase | Qt.ImhNoPredictiveText
                onAccepted: {
                    if (pane.memory.goTo(text))
                        bytesView.forceActiveFocus();
                }
            }
            Button {
                text: qsTr("Go")
                onClicked: pane.memory.goTo(location.text)
            }
        }
        Flow {
            Layout.fillWidth: true
            Layout.preferredHeight: childrenRect.height
            spacing: Theme.sp(1)
            Label {
                text: pane.memory.addressText
                font.family: "monospace"
                color: Theme.text
                padding: Theme.sp(2)
            }
            Button {
                id: editButton
                objectName: "memoryEdit"
                text: qsTr("Edit byte")
                enabled: pane.memory.editable
                onClicked: pane.edit(editButton)
            }
            Button {
                text: qsTr("Watch read")
                enabled: pane.memory.editable
                onClicked: ZxEmulator.watchMemory(1)
            }
            Button {
                objectName: "memoryWatchWrite"
                text: qsTr("Watch write")
                enabled: pane.memory.editable
                onClicked: ZxEmulator.watchMemory(2)
            }
            Button {
                text: qsTr("Previous")
                onClicked: pane.memory.moveCursor(-256)
            }
            Button {
                text: qsTr("Next")
                onClicked: pane.memory.moveCursor(256)
            }
        }
        Label {
            Layout.fillWidth: true
            text: pane.memory.error
            color: Theme.danger
            visible: text.length > 0
            wrapMode: Text.Wrap
        }
        ListView {
            id: bytesView
            objectName: "debuggerMemory"
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            focus: true
            activeFocusOnTab: true
            model: pane.memory
            readonly property int cellWidth: Math.max(Theme.sp(6.5), Math.floor((width - Theme.sp(20) - (width > Theme.sp(225) ? Theme.sp(45) : 0)) / pane.memory.columns))
            onWidthChanged: pane.memory.columns = width > Theme.sp(155) ? 16 : width > Theme.sp(85) ? 8 : 4
            Keys.onLeftPressed: pane.memory.moveCursor(-1)
            Keys.onRightPressed: pane.memory.moveCursor(1)
            Keys.onUpPressed: {
                if (pane.memory.cursor >= pane.memory.columns)
                    pane.memory.moveCursor(-pane.memory.columns);
                else
                    ZxEmulator.sendNavigationKey(Qt.Key_Backtab);
            }
            Keys.onDownPressed: pane.memory.moveCursor(pane.memory.columns)
            Keys.onReturnPressed: pane.edit(bytesView)
            Keys.onEnterPressed: pane.edit(bytesView)
            Keys.onPressed: event => {
                if (event.key === Qt.Key_PageUp || event.key === Qt.Key_PageDown) {
                    pane.memory.moveCursor(event.key === Qt.Key_PageUp ? -256 : 256);
                    event.accepted = true;
                }
            }
            delegate: Row {
                id: row
                required property int index
                required property string addressText
                required property list<string> cells
                required property string ascii
                height: Theme.sp(7.5)
                Label {
                    text: row.addressText
                    width: Theme.sp(20)
                    height: parent.height
                    verticalAlignment: Text.AlignVCenter
                    font.family: "monospace"
                    color: Theme.textMuted
                }
                Repeater {
                    model: row.cells
                    delegate: Rectangle {
                        id: cell
                        required property int index
                        required property string modelData
                        width: bytesView.cellWidth
                        height: row.height
                        readonly property bool selected: pane.memory.cursor === row.index * pane.memory.columns + index
                        color: selected ? Theme.accentFill : "transparent"
                        border.width: selected ? Theme.focusLine : 0
                        border.color: Theme.accent
                        Label {
                            anchors.centerIn: parent
                            text: cell.modelData
                            font.family: "monospace"
                            color: Theme.text
                        }
                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                pane.memory.cursor = row.index * pane.memory.columns + cell.index;
                                bytesView.forceActiveFocus();
                            }
                            onDoubleClicked: pane.edit(bytesView)
                        }
                    }
                }
                Label {
                    visible: bytesView.width > Theme.sp(225)
                    text: row.ascii
                    height: row.height
                    verticalAlignment: Text.AlignVCenter
                    font.family: "monospace"
                    color: Theme.textMuted
                }
            }
            Connections {
                target: pane.memory
                function onCursorChanged() {
                    bytesView.positionViewAtIndex(Math.floor(pane.memory.cursor / pane.memory.columns), ListView.Contain);
                }
            }
            ScrollBar.vertical: ScrollBar {}
        }
        Label {
            Layout.fillWidth: true
            visible: pane.height >= Theme.sp(80)
            text: Theme.keyboardHints ? qsTr("Arrows select bytes · Enter edits · Values are hexadecimal · ROM editing needs explicit confirmation")
                                      : qsTr("Values are hexadecimal · ROM editing needs explicit confirmation")
            wrapMode: Text.Wrap
            color: Theme.textMuted
        }
    }
    FormDialog {
        id: editor
        title: qsTr("Edit byte at %1").arg(pane.memory.addressText)
        initialFocus: byteValue
        acceptable: byteValue.acceptableInput && (pane.memory.writable || romEdit.checked)
        commit: () => pane.memory.writeByte(byteValue.text, romEdit.checked)
        TextField {
            id: byteValue
            objectName: "memoryByte"
            Layout.fillWidth: true
            placeholderText: "00–FF"
            inputMethodHints: Qt.ImhPreferUppercase | Qt.ImhNoPredictiveText
            validator: RegularExpressionValidator { regularExpression: /(?:\$|0[xX])?[0-9a-fA-F]{1,2}/ }
            onAccepted: editor.save()
        }
        CheckBox {
            id: romEdit
            text: qsTr("Allow editing ROM")
            visible: !pane.memory.writable
        }
    }
}
