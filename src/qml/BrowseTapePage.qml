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
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtGamepadLegacy
import Spectacol

Sheet {
    id: page
    crumb: qsTr("Tape")
    title: qsTr("Blocks")
    preferredWidth: Theme.sp(130)
    hints: [
        { pad: "A", key: "Enter", text: qsTr("Play from here"), action: () => tapeData.selectBlock(view.currentIndex) },
        { pad: "Y", key: "L", text: qsTr("Reset and load from here"), action: () => page.resetAndLoad() },
        { pad: "X", key: "V", text: qsTr("View data"), action: () => page.showBlock(view.currentIndex) },
        { pad: "B", key: "Esc", text: qsTr("Close"), action: () => pageLoader.source = "" }
    ]

    Component.onCompleted: ZxEmulator.processInputEvents = false
    Component.onDestruction: ZxEmulator.processInputEvents = true

    property string base64CompressedData: ""
    // docked (desktop layout): the tape plays on while the blocks are shown
    readonly property bool pausesEmulation: !Theme.desktopMode

    function resetAndLoad() {
        if (tapeData.resetAndLoadBlock(view.currentIndex)) pageLoader.source = "";
    }

    function showBlock(index) {
        base64CompressedData = tapeData.base64CompressedBlockData(index);
        offset.value = tabbar.currentIndex;
        offset.to = tapeData.getBufferLength(index);
        showData.open();
    }

    Popup {
        id: showData
        modal: true
        focus: true
        x: (parent.width - width) / 2
        y: (parent.height - height) / 2
        height: parent.height - Theme.sp(10)
        width: parent.width - Theme.sp(10)
        background: Rectangle {
            radius: Theme.radius
            color: Theme.raised
            border.width: Theme.line
            border.color: Theme.outline
        }
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnReleaseOutsideParent
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
                        mono: true
                        textFormat: Text.Normal
                        style: Text.Normal
                        wrapMode: Text.WordWrap
                        horizontalAlignment: Text.AlignLeft
                        verticalAlignment: Text.AlignTop
                        text: ZxEmulator.dumpData(page.base64CompressedData, offset.value, Theme.smallScreen ? 8 : 16);
                    }
                }
                Image {
                    fillMode: Image.PreserveAspectFit
                    source: page.base64CompressedData === "" ? "" : "image://spectrum/compressed64:" + offset.value + ":" + page.base64CompressedData
                }
            }

            Button {
                Layout.fillWidth: true
                text: Theme.gamepadHints ? qsTr("Close <b>(B)</b>") : qsTr("Close")
                onClicked: showData.close()
            }
        }
    }

    GamepadNavigation {
        gamepad: Gamepad { deviceId: ZxEmulator.gamepadId }
        buttonAKey: Qt.Key_Return
        buttonBKey: Qt.Key_Escape
        buttonXKey: Qt.Key_V
        buttonYKey: Qt.Key_L
    }

    ZxTapeData {
        id: tapeData
    }

    ListView {
        id: view
        anchors.fill: parent
        anchors.margins: Theme.sp(2)
        clip: true
        focus: true
        spacing: Theme.sp(0.5)
        boundsBehavior: Flickable.StopAtBounds
        highlightMoveDuration: Theme.reduceMotion ? 0 : 100
        ScrollIndicator.vertical: ScrollIndicator {}
        model: tapeData
        // the block playing (after the last one when the tape has ended); then the user's
        Component.onCompleted: currentIndex = Math.max(0, Math.min(tapeData.currentBlock, count - 1))

        Keys.onReturnPressed: tapeData.selectBlock(currentIndex)
        Keys.onEnterPressed: tapeData.selectBlock(currentIndex)
        Keys.onPressed: (event) => {
            if (event.key === Qt.Key_L || event.key === Qt.Key_Y) {
                page.resetAndLoad();
                event.accepted = true;
            } else if (event.key === Qt.Key_V || event.key === Qt.Key_X) {
                page.showBlock(currentIndex);
                event.accepted = true;
            }
        }

        highlight: Rectangle {
            radius: Theme.radiusSmall
            color: Theme.accentFill
            border.width: Theme.focusLine
            border.color: Theme.accent
        }

        delegate: Item {
            id: block
            required property int index
            required property string label
            required property bool isCurrent
            width: view.width
            height: Math.max(Theme.rowHeight, blockColumn.implicitHeight + Theme.sp(4))

            Text {
                id: playing
                x: Theme.sp(3)
                anchors.verticalCenter: parent.verticalCenter
                width: Theme.sp(5)
                text: block.isCurrent ? "▶" : ""
                font.pixelSize: Theme.body
                color: Theme.accent
            }
            Column {
                id: blockColumn
                anchors.left: playing.right
                anchors.right: parent.right
                anchors.rightMargin: Theme.sp(3)
                anchors.verticalCenter: parent.verticalCenter
                spacing: Theme.sp(0.5)
                Text {
                    width: parent.width
                    text: block.label
                    elide: Text.ElideRight
                    font.pixelSize: Theme.body
                    color: Theme.text
                }
            }
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    if (view.currentIndex !== block.index)
                        view.currentIndex = block.index;
                    else
                        tapeData.selectBlock(block.index);
                }
                onPressAndHold: page.showBlock(block.index)
            }
        }
    }
}
