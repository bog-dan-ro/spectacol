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

// @scope main.qml

import QtQuick
import QtQml.Models
import QtQuick.Layouts
import QtQuick.Window
import Spectacol
import "private"


ListView {
    id: view
    readonly property int selectedAddress: currentItem ? currentItem.address : -1
    readonly property bool dialogOpen: currentItem && currentItem.editing
    function closeDialog(): bool {
        if (!dialogOpen)
            return false;
        currentItem.cancelEdit();
        return true;
    }
    clip: true
    activeFocusOnTab: true

    snapMode: ListView.SnapToItem
    highlightFollowsCurrentItem: true
    highlightMoveDuration: 100

    // Not a binding: the navigation keys and clicks below assign currentIndex
    // directly, which would remove a binding after the first interaction.
    Component.onCompleted: currentIndex = ZxEmulator.disassambleModel.delta
    Connections {
        target: ZxEmulator.disassambleModel
        function onDeltaChanged() { view.currentIndex = ZxEmulator.disassambleModel.delta; }
    }

    Z80Assembler {
        id: assembler
    }

    DelegateModel {
        id: visualModel
        model: ZxEmulator.disassambleModel
        delegate: Rectangle {
            id: row
            // Required properties keep their last value while a delegate is being released
            // (e.g. by a model reset), the context properties become undefined.
            required property int index
            required property int address
            required property string addressText
            required property var bytes
            required property string bytesText
            required property string annotation
            required property string disassable
            required property color background
            required property color foreground
            required property color selectedBackground
            required property color selectedForeground

            readonly property bool editing: !asmEdit.readOnly
            function cancelEdit() {
                asmEdit.text = row.disassable;
                asmEdit.validator = null;
                asmEdit.readOnly = true;
                Qt.inputMethod.hide();
                view.forceActiveFocus();
            }
            function toggleEdit()
            {
                asmEdit.readOnly = !asmEdit.readOnly;
                if (asmEdit.readOnly) {
                    asmEdit.focus = false;
                    return false;
                }

                asmEdit.readOnly = false;
                asmEdit.selectAll();
                asmEdit.forceActiveFocus(Qt.TabFocusReason);
                assembler.address = row.address;
                asmEdit.validator = assembler;
                return true;
            }

            readonly property color paper: row.background
            readonly property color ink: row.foreground

            width: view.width
            height: asmEdit.contentHeight + asmEdit.topPadding + asmEdit.bottomPadding + Theme.sp(1) + (annotationLabel.visible ? annotationLabel.implicitHeight + Theme.sp(1) : 0)
            color: paper
            radius: Theme.radiusSmall
            border.width: view.currentIndex === row.index ? Theme.focusLine : 0
            border.color: Theme.accent
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    view.forceActiveFocus(Qt.TabFocusReason);
                    view.currentIndex = row.index;
                }
                onDoubleClicked: ZxEmulator.toggleBreakpoint(row.address)
                onPressAndHold: ZxEmulator.toggleBreakpoint(row.address)
            }
            Text {
                id: annotationLabel
                anchors { left: parent.left; right: parent.right; bottom: parent.bottom; margins: Theme.sp(1) }
                visible: row.annotation.length > 0
                text: row.annotation
                color: Theme.textMuted
                font.pixelSize: Theme.caption
                elide: Text.ElideMiddle
            }
            RowLayout {
                anchors.fill: parent
                anchors.bottomMargin: annotationLabel.visible ? annotationLabel.implicitHeight + Theme.sp(1) : 0
                FancyText {
                    mono: true
                    Layout.fillHeight: true
                    Layout.fillWidth: false
                    horizontalAlignment: Text.AlignRight
                    font.pixelSize: TextSizes.scale12
                    style: Text.Normal
                    Layout.preferredWidth: 4 * TextSizes.scale12
                    color: row.ink
                    text: row.addressText
                }

                Item { Layout.fillWidth: false; Layout.preferredWidth: 1.5 * Screen.pixelDensity }

                FancyText {
                    mono: true
                    Layout.fillHeight: true
                    Layout.fillWidth: false
                    Layout.preferredWidth: 4 * TextSizes.scale12
                    horizontalAlignment: Text.AlignHCenter
                    font.pixelSize: TextSizes.scale16 / 2
                    style: Text.Normal
                    color: row.ink
                    wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                    text: row.bytesText
                }

                Item { Layout.fillWidth: false; Layout.preferredWidth: 1.5 * Screen.pixelDensity }

                FancyTextField {
                    mono: true
                    id: asmEdit
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignLeft
                    font.pixelSize: TextSizes.scale12
                    color: row.ink
                    font.bold: false
                    readOnly: true
                    inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhPreferUppercase | Qt.ImhNoPredictiveText
                    text: row.disassable
                    onEditingFinished: {
                        // Unchanged text: keep the bytes (another encoding could print the same)
                        if (text !== row.disassable)
                            assembler.write(text, row.address);
                        ZxEmulator.disassambleModel.update();
                        validator = null;
                        if (!readOnly)
                            row.toggleEdit();
                    }
                    Keys.onEscapePressed: event => {
                        event.accepted = view.closeDialog();
                    }
                    Keys.onReturnPressed: row.toggleEdit()
                    Keys.onEnterPressed: row.toggleEdit()

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            view.forceActiveFocus(Qt.TabFocusReason);
                            view.currentIndex = row.index;
                        }

                        onDoubleClicked: row.toggleEdit()
                        onPressAndHold: row.toggleEdit()
                    }
                }
            }
        }
    }

    Keys.onPressed: (event) => {
        switch (event.key) {
        case Qt.Key_Home:
            ZxEmulator.disassamble(0, 0);
            view.currentIndex = 0;
            event.accepted = true;
            break;

        case Qt.Key_PageUp:
            view.currentIndex = Math.max(0, view.currentIndex - 10);
            event.accepted = true;
            break;

        case Qt.Key_Up:
            if (view.currentIndex > 1) {
                decrementCurrentIndex();
            } else {
                ZxEmulator.disassambleFetchUp(10);
                decrementCurrentIndex();
            }
            event.accepted = true;
            break;

        case Qt.Key_Down:
            incrementCurrentIndex();
            event.accepted = true;
            break;

        case Qt.Key_PageDown:
            view.currentIndex = Math.min(view.count -1, view.currentIndex + 10);
            event.accepted = true;
            break;

        case Qt.Key_End:
            ZxEmulator.disassamble(0xffff);
            event.accepted = true;
            break;

        case Qt.Key_Escape:
            pageLoader.source = "";
            event.accepted = true;
            break;

        case Qt.Key_Enter:
        case Qt.Key_Return:
            if (currentItem)
                event.accepted = currentItem.toggleEdit();
            break;

        case Qt.Key_C:
            ZxEmulator.debuggerContinue();
            event.accepted = true;
            break;

        case Qt.Key_S:
            ZxEmulator.debuggerStep();
            event.accepted = true;
            break;

        case Qt.Key_N:
            ZxEmulator.debuggerNext();
            event.accepted = true;
            break;

        case Qt.Key_X: {
            const address = ZxEmulator.disassambleModel.address(view.currentIndex);
            if (address !== -1)
                ZxEmulator.toggleBreakpoint(address);
            event.accepted = true;
            break;
        }
        }
    }
    model: visualModel
}
