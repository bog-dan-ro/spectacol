/*
    Copyright (c) 2015, BogDan Vatra <bogdan@kde.org>

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

import QtQuick 2.5
import QtQuick.Layouts 1.1
import QtQuick.Window 2.0
import Fuse 1.0

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
        model: disassambleModel
        delegate: Rectangle {
            property color paper: view.currentIndex !== index ? background : selectedBackground
            property color ink: view.currentIndex !== index ? foreground : selectedForeground

            width: view.width
            height: 7 * Screen.pixelDensity
            color: paper
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    view.focus = true;
                    view.currentIndex = model.index;
                }
                onDoubleClicked: fuse.addBreakpoint(model.address)
                onPressAndHold: fuse.addBreakpoint(model.address)
            }
            RowLayout {
                anchors.fill: parent
                FancyText {
                    Layout.fillHeight: true
                    Layout.fillWidth: false
                    horizontalAlignment: Text.AlignRight
                    fontSize: 4
                    style: Text.Normal
                    Layout.preferredWidth: 14 * Screen.pixelDensity
                    color: ink
                    text: model.addressText
                }

                Item { Layout.fillWidth: false;width: 1.5 * Screen.pixelDensity }

                FancyText {
                    Layout.fillHeight: true
                    Layout.fillWidth: false
                    Layout.preferredWidth: 15 * Screen.pixelDensity
                    horizontalAlignment: Text.AlignHCenter
                    fontSize: 2.5
                    style: Text.Normal
                    color: ink
                    wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                    text: model.bytesText
                }

                Item { Layout.fillWidth: false;width: 1.5 * Screen.pixelDensity }

                FancyTextField {
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignLeft
                    fontSize: 4
                    textColor: ink
                    readOnly: true
                    style: Text.Normal
                    inputMethodHints: Qt.ImhPreferUppercase
                    text: model.disassable
                    onAccepted: {
                        readOnly = true
                        assembler.write(text, model.address, model.bytes)
                        disassambleModel.update();
                        validator = null;
                        console.log(text)
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            view.focus = true;
                            view.currentIndex = model.index;
                        }
                        onDoubleClicked: {
                            parent.readOnly = false;
                            parent.selectAll();
                            parent.focus = true;
                            parent.validator = assembler;
                        }
                        onPressAndHold: Qt.quit()
                    }
                }
            }
        }
    }

    Keys.onPressed: {
        event.accepted = true;
        switch (event.key) {
        case Qt.Key_Home:
            fuse.disassamble(0, 0);
            view.currentIndex = 0;
            break;

        case Qt.Key_PageUp:
            view.currentIndex = Math.max(0, view.currentIndex - 10);
            break;

        case Qt.Key_Up:
            if (view.currentIndex > 1) {
                decrementCurrentIndex();
            } else {
                fuse.disassambleFetchUp(10);
                decrementCurrentIndex();
            }
            break;

        case Qt.Key_Down:
            incrementCurrentIndex();
            break;

        case Qt.Key_PageDown:
            view.currentIndex = Math.min(view.count -1, view.currentIndex + 10);
            break;

        case Qt.Key_End:
            fuse.disassamble(0xffff);
            break;

        case Qt.Key_Escape:
            pageLoader.source = "";
            break;
        }
    }
    model: visualModel
}
