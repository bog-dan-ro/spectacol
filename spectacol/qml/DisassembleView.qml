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
import QtQuick.Layouts
import QtQuick.Window
import Spectacol
import "private"


ListView {
    id: view
    clip: true

    snapMode: ListView.SnapToItem
    highlightFollowsCurrentItem: true
    highlightMoveDuration: 100

    currentIndex: disassambleModel.delta

    Z80Assembler {
        id: assembler
    }

    VisualDataModel {
        id: visualModel
        model: FuseEmulator.disassambleModel
        delegate: Rectangle {
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
                asmEdit.validator = assembler;
                return true;
            }

            property color paper: view.currentIndex !== index ? background : selectedBackground
            property color ink: view.currentIndex !== index ? foreground : selectedForeground

            width: view.width
            height: asmEdit.contentHeight + asmEdit.topPadding + asmEdit.bottomPadding + Screen.pixelDensity
            color: paper
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    view.forceActiveFocus(Qt.TabFocusReason);
                    view.currentIndex = model.index;
                }
                onDoubleClicked: FuseEmulator.addBreakpoint(model.address)
                onPressAndHold: FuseEmulator.addBreakpoint(model.address)
            }
            RowLayout {
                anchors.fill: parent
                FancyText {
                    Layout.fillHeight: true
                    Layout.fillWidth: false
                    horizontalAlignment: Text.AlignRight
                    font.pixelSize: TextSizes.scale12
                    style: Text.Normal
                    Layout.preferredWidth: 4 * TextSizes.scale12
                    color: ink
                    text: model.addressText
                }

                Item { Layout.fillWidth: false; Layout.preferredWidth: 1.5 * Screen.pixelDensity }

                FancyText {
                    Layout.fillHeight: true
                    Layout.fillWidth: false
                    Layout.preferredWidth: 4 * TextSizes.scale12
                    horizontalAlignment: Text.AlignHCenter
                    font.pixelSize: TextSizes.scale16 / 2
                    style: Text.Normal
                    color: ink
                    wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                    text: model.bytesText
                }

                Item { Layout.fillWidth: false; Layout.preferredWidth: 1.5 * Screen.pixelDensity }

                FancyTextField {
                    id: asmEdit
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignLeft
                    font.pixelSize: TextSizes.scale12
                    color: ink
                    font.bold: false
                    readOnly: true
                    inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhPreferUppercase | Qt.ImhNoPredictiveText
                    text: model.disassable
                    onEditingFinished: {
                        assembler.write(text, model.address, model.bytes)
                        FuseEmulator.disassambleModel.update();
                        validator = null;
                        if (!readOnly)
                            toggleEdit();
                    }
                    Keys.onReturnPressed: toggleEdit()
                    Keys.onEnterPressed: toggleEdit()

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            view.forceActiveFocus(Qt.TabFocusReason);
                            view.currentIndex = model.index;
                        }

                        onDoubleClicked:
                            toggleEdit();

                        onPressAndHold:
                            toggleEdit();
                    }
                }
            }
        }
    }

    Keys.onPressed: (event) => {
        switch (event.key) {
        case Qt.Key_Home:
            FuseEmulator.disassamble(0, 0);
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
                FuseEmulator.disassambleFetchUp(10);
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
            FuseEmulator.disassamble(0xffff);
            event.accepted = true;
            break;

        case Qt.Key_Escape:
            pageLoader.source = "";
            event.accepted = true;
            break;

        case Qt.Key_Enter:
        case Qt.Key_Return:
            event.accepted = currentItem.toggleEdit();
            break;

        case Qt.Key_C:
            FuseEmulator.debuggerRun();
            event.accepted = true;
            break;

        case Qt.Key_S:
            FuseEmulator.debuggerStep();
            event.accepted = true;
            break;

        case Qt.Key_N:
            FuseEmulator.debuggerNext();
            event.accepted = true;
            break;

        case Qt.Key_X:
            FuseEmulator.addBreakpoint(FuseEmulator.disassambleModel.address(view.currentIndex));
            event.accepted = true;
            break;
        }
    }
    model: visualModel
}
