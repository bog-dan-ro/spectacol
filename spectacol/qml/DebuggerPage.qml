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
import QtQuick.Controls
import QtGamepadLegacy
import Spectacol
import "private"

Item {
    // The debugger pauses and runs the emulation itself (Continue runs it with the page open)
    readonly property bool pausesEmulation: false

    Component.onCompleted: ZxEmulator.processInputEvents = false

    Component.onDestruction: {
        ZxEmulator.processInputEvents = true;
        ZxEmulator.debuggerRun();
    }

    GamepadKeyNavigation {
        gamepad: Gamepad { deviceId: ZxEmulator.gamepadId }
        buttonAKey: Qt.Key_Return
        buttonBKey: Qt.Key_Escape
        buttonXKey: Qt.Key_X
        buttonYKey: Qt.Key_Y
        buttonL1Key: Qt.Key_S
        buttonR1Key: Qt.Key_C
    }

    readonly property color backgroudColor : Theme.surface
    ColumnLayout {
        spacing: TextSizes.smallScreen ? 0 : 2.5 * Screen.pixelDensity
        anchors.fill: parent
        RowLayout {
            spacing: 2.5 * Screen.pixelDensity
            Layout.fillWidth: true
            Layout.fillHeight: true
            Rectangle {
                id: disassambleRect
                color: backgroudColor
                radius: Theme.radiusSmall
                border.width: disassembleView.focus ? Theme.focusLine : Theme.line
                border.color: disassembleView.focus ? Theme.accent : Theme.outline

                Layout.preferredWidth: 18 * TextSizes.scale14
                Layout.fillHeight: true

                DisassembleView {
                    id: disassembleView
                    anchors.fill: parent
                    anchors.margins: 2 * Screen.pixelDensity
                    focus: true
                    KeyNavigation.left: breakpointsView
                    KeyNavigation.right: breakpointsView
                }
            }
            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Rectangle {
                    id: infoRect
                    visible: false
                    anchors.fill: parent
                    radius: Theme.radiusSmall
                    color: backgroudColor
                    border.width: focus ? Theme.focusLine : Theme.line
                    border.color: focus ? Theme.accent : Theme.outline

                    FancyText {
                        anchors.centerIn: parent
                        text: "info"
                    }
                }
            }
            ColumnLayout {
                spacing: TextSizes.smallScreen ? 0 : 2.5 * Screen.pixelDensity
                Layout.preferredWidth: 18 * TextSizes.scale14
                Layout.fillWidth: false
                Layout.fillHeight: true
                Rectangle {
                    id: registersRect
                    color: backgroudColor
                    radius: Theme.radiusSmall
                    border.width: focus ? Theme.focusLine : Theme.line
                    border.color: focus ? Theme.accent : Theme.outline
                    Layout.preferredHeight: 14 * (TextSizes.smallScreen ? TextSizes.scale12 : TextSizes.scale14)
                    Layout.fillWidth: true
                    RegistersView {
                        anchors.fill: parent
                    }
                }
                Rectangle {
                    id: breakPointsRect
                    color: backgroudColor
                    radius: Theme.radiusSmall
                    border.width: breakpointsView.activeFocus ? Theme.focusLine : Theme.line
                    border.color: breakpointsView.activeFocus ? Theme.accent : Theme.outline
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    FocusScope {
                        id: breakpointsView
                        anchors.fill: parent
                        anchors.margins: 2 * Screen.pixelDensity
                        KeyNavigation.left: disassembleView
                        KeyNavigation.right: disassembleView
                        KeyNavigation.tab: commandsLine
                        KeyNavigation.down: commandsLine
                        KeyNavigation.backtab: disassembleView
                        BreakpointsView {
                            anchors.fill: parent
                            onBreakpointSelected: ZxEmulator.disassamble(item.absoluteAddress);
                        }
                    }
                }
            }
        }

        Rectangle {
            id: toolBarRect
            Layout.preferredHeight: commandsLine.contentHeight + commandsLine.topPadding + commandsLine.bottomPadding + Screen.pixelDensity
            Layout.fillWidth: true
            color: backgroudColor
            radius: Theme.radiusSmall
            border.width: focus ? Theme.focusLine : Theme.line
            border.color: focus ? Theme.accent : Theme.outline
            RowLayout {
                anchors.fill: parent
                anchors.margins: parent.border.width
                FancyTextField {
                    mono: true
                    id: commandsLine
                    KeyNavigation.up: disassembleView
                    KeyNavigation.down: breakpointsView
                    KeyNavigation.tab: disassembleView
                    KeyNavigation.backtab: breakpointsView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.leftMargin: toolBarRect.border.width
                    onAccepted: {
                        ZxEmulator.debuggerCommand(text);
                        selectAll();
                    }
                    placeholderText: qsTr("Type a command here")
                }
                Button {
                    Layout.fillHeight: true
                    text: qsTr("Step (L1)")
                    onClicked: ZxEmulator.debuggerStep()
                }
                Button {
                    Layout.fillHeight: true
                    text: qsTr("Next")
                    onClicked: ZxEmulator.debuggerNext()
                }
                Button {
                    Layout.fillHeight: true
                    Layout.rightMargin: toolBarRect.border.width
                    text: qsTr("Continue (R1)")
                    onClicked: ZxEmulator.debuggerRun()
                }
                Button {
                    Layout.fillHeight: true
                    Layout.rightMargin: toolBarRect.border.width
                    text: qsTr("Close (B)")
                    onClicked: pageLoader.source = ""
                }
            }
        }
    }
}
