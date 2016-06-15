/*
    Copyright (c) 2016, BogDan Vatra <bogdan@kde.org>

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

import QtQuick 2.6
import QtQuick.Window 2.2
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.1
import QtGamepad 1.0
import Fuse 1.0

import "private" 1.0

Item {
    Component.onCompleted: {
        fuse.processInputEvents = false;
        fuse.paused = true;
    }
    Component.onDestruction: {
        fuse.processInputEvents = true;
        fuse.paused = false;
    }

    anchors.fill: parent

    property string base64CompressedData: ""

    Popup {
        id: showData
        modal: true
        focus: true
        x: (parent.width - width) / 2
        y: (parent.height - height) / 2
        height: parent.height - Screen.pixelDensity * 10
        width: parent.width - Screen.pixelDensity * 10
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnReleaseOutsideParent
        onOpened: {
            height = parent.height - Screen.pixelDensity * 10;
            width = parent.width - Screen.pixelDensity * 10;
        }
        ColumnLayout {
            anchors.fill: parent
            RowLayout {
                Layout.fillWidth: true
                FancyText {
                    text: qsTr("Offset :")
                    Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
                }
                SpinBox {
                    id: offset
                    editable: true
                    value: swipeView.currentIndex
                    from: 0
                }
            }

            TabBar {
                id: tabbar
                currentIndex: swipeView.currentIndex
                onCurrentIndexChanged: {
                    switch (currentIndex) {
                    case 0:
                        if (offset.value === 1)
                            offset.value = 0;
                        break;
                    case 1:
                        if (offset.value === 0)
                            offset.value = 1;
                        break;
                    }
                }
                Layout.fillWidth: true
                TabButton {
                    text: qsTr("Show as binary data")
                }
                TabButton {
                    text: qsTr("Show as screen")
                }
            }
            SwipeView {
                id: swipeView
                currentIndex: tabbar.currentIndex
                clip: true
                Layout.fillWidth: true
                Layout.fillHeight: true
                Flickable {
                    contentHeight: aboutText.height
                    clip: true
                    FancyText {
                        id: aboutText
                        textFormat: Text.Normal
                        style: Text.Normal
                        wrapMode: Text.WordWrap
                        horizontalAlignment: Text.AlignLeft
                        verticalAlignment: Text.AlignTop
                        text: fuse.dumpData(base64CompressedData, offset.value, TextSizes.smallScreen? 8 : 16);
                    }
                }
                Image {
                    fillMode: Image.PreserveAspectFit
                    source: "image://spectrum/compressed64:" + offset.value + ":" + base64CompressedData
                }
            }

            Button {
                Layout.fillWidth: true
                text: qsTr("Close <b>(B)</b>")
                onClicked: showData.close()
            }
        }
    }

    GamepadKeyNavigation {
        gamepad: Gamepad { deviceId: fuse.gamepadId }
        buttonAKey: Qt.Key_Return
        buttonBKey: Qt.Key_Escape
        buttonYKey: Qt.Key_X
    }

    FancyBaseList {
        id: view
        FuseTapeData {
            id: tapeData
        }

        Keys.onLeftPressed: decrementCurrentIndex()
        Keys.onRightPressed: incrementCurrentIndex()
        Keys.onEscapePressed: pageLoader.source = ""
        Keys.onReturnPressed: model.setCurrentIndex(currentIndex)

        footer: Button {
            width: view.width
            text: qsTr("Close (B)")
            onClicked: pageLoader.source = ""
        }

        highlightRangeMode: ListView.NoHighlightRange
        highlightMoveVelocity: -1
        model: tapeData
        currentIndex: tapeData.currentIndex()

        width: 20 * (TextSizes.smallScreen ? TextSizes.scale12 : TextSizes.scale14)
        delegate: Rectangle {
            width: view.width
            height: (TextSizes.smallScreen ? TextSizes.scale12 : TextSizes.scale14) * 4
            color: Qt.rgba(0, 0, 0, 0.75)

            FancyText {
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.leftMargin: (TextSizes.smallScreen ? TextSizes.scale12 : TextSizes.scale14) / 2
                anchors.topMargin: anchors.leftMargin
                font.pixelSize: (TextSizes.smallScreen ? TextSizes.scale12 : TextSizes.scale14)
                text: (isCurrent ? "\u21f6 " : "  ") + label
            }
            FancyText {
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.rightMargin: (TextSizes.smallScreen ? TextSizes.scale12 : TextSizes.scale14) / 2
                anchors.bottomMargin: anchors.rightMargin
                horizontalAlignment: Text.AlignRight
                font.pixelSize: (TextSizes.smallScreen ? TextSizes.scale12 : TextSizes.scale14)
                text: detail
            }
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    if (view.currentIndex !== model.index)
                        view.currentIndex = model.index;
                    else
                        tapeData.setCurrentIndex(model.index);
                }

                onPressAndHold: {
                    base64CompressedData = tapeData.base64CompressedBlockData(model.index);
                    offset.value = tabbar.currentIndex;
                    offset.to = tapeData.getBufferLength(model.index);
                    showData.open();
                }
            }
        }
    }
}
