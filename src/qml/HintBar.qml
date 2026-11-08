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
import Spectacol

// Desktop keys, connected gamepad buttons, or Android TV's OK/Back. Dialogs
// without a controller keep ordinary action buttons, never keyboard badges.
Item {
    id: bar
    property var hints: []
    readonly property bool compact: Theme.touchScreenUi || width < Theme.sp(150)
    readonly property bool gamepad: Theme.gamepadHints
    readonly property var shownHints: Theme.remoteHints
        ? hints.filter(h => ["A", "B", "◀▶", "↑↓"].includes(h.pad)) : hints
    readonly property bool plainActions: Theme.touchScreenUi && !Theme.showHintBar
    implicitHeight: bar.compact ? Theme.rowHeight + Theme.sp(2) : flow.implicitHeight + Theme.sp(6)

    function padColor(pad: string): color {
        switch (pad) {
        case "A": return Theme.padA;
        case "B": return Theme.padB;
        case "X": return Theme.padX;
        case "Y": return Theme.padY;
        }
        return Theme.textMuted;
    }
    Rectangle {
        width: parent.width
        height: Theme.line
        color: Theme.outline
    }
    Flickable {
        id: viewport
        anchors.fill: parent
        anchors.leftMargin: Theme.sp(bar.compact ? 2 : 5)
        anchors.rightMargin: Theme.sp(bar.compact ? 2 : 5)
        anchors.topMargin: Theme.sp(bar.compact ? 1 : 3)
        contentWidth: bar.compact ? Math.max(width, flow.childrenRect.width) : width
        contentHeight: flow.implicitHeight
        flickableDirection: Flickable.HorizontalFlick
        interactive: bar.compact && contentWidth > width
        boundsBehavior: Flickable.StopAtBounds
        clip: true
        ScrollIndicator.horizontal: ScrollIndicator {}
        Flow {
            id: flow
            width: viewport.width
            height: bar.compact ? Theme.rowHeight : implicitHeight
            // One item per column makes a single row without measuring delegates
            // before the Repeater has created them.
            flow: bar.compact ? Flow.TopToBottom : Flow.LeftToRight
            spacing: Theme.sp(bar.compact ? 2 : 5)
            Repeater {
                id: repeater
                model: bar.shownHints
                delegate: Item {
                    id: hint
                    required property var modelData
                    readonly property bool actionable: typeof modelData.action === "function"
                    readonly property bool round: bar.gamepad && modelData.pad.length === 1
                    visible: !bar.plainActions || actionable
                    implicitWidth: content.implicitWidth + (bar.plainActions ? Theme.sp(4) : 0)
                    implicitHeight: bar.compact ? Theme.rowHeight : Math.max(content.implicitHeight, Theme.sp(8))
                    enabled: actionable
                    activeFocusOnTab: actionable
                    Accessible.name: modelData.text
                    Accessible.role: actionable ? Accessible.Button : Accessible.StaticText
                    Accessible.onPressAction: if (actionable) modelData.action()
                    Keys.onReturnPressed: if (actionable) modelData.action()
                    Keys.onEnterPressed: if (actionable) modelData.action()
                    Keys.onSelectPressed: if (actionable) modelData.action()
                    Keys.onSpacePressed: if (actionable) modelData.action()
                    TapHandler {
                        id: press
                        enabled: hint.actionable
                        // A drag belongs to the surrounding Flickable, not an action.
                        gesturePolicy: TapHandler.DragThreshold
                        onTapped: hint.modelData.action()
                    }
                    HoverHandler {
                        enabled: hint.actionable
                        cursorShape: Qt.PointingHandCursor
                    }
                    onActiveFocusChanged: {
                        if (activeFocus && bar.compact)
                            viewport.contentX = Math.max(0, Math.min(x, viewport.contentWidth - viewport.width));
                    }
                    Rectangle {
                        anchors.fill: parent
                        visible: bar.plainActions || hint.activeFocus
                        color: press.pressed ? Theme.accentFill : Theme.raised
                        radius: Theme.radiusSmall
                        border.width: hint.activeFocus ? Theme.focusLine : Theme.line
                        border.color: hint.activeFocus ? Theme.accent : Theme.outline
                    }
                    Item {
                        anchors.fill: parent
                        Row {
                            id: content
                            anchors.centerIn: parent
                            spacing: Theme.sp(1.5)
                            Rectangle {
                                visible: !bar.plainActions
                                anchors.verticalCenter: parent.verticalCenter
                                height: Math.round(Theme.caption * 1.7)
                                width: visible ? (hint.round ? height : Math.max(height, keyLabel.implicitWidth + Theme.sp(3))) : 0
                                radius: hint.round ? height / 2 : Theme.radiusSmall
                                color: bar.gamepad ? bar.padColor(hint.modelData.pad) : Theme.raised
                                border.width: bar.gamepad ? 0 : Theme.line
                                border.color: Theme.outline
                                Text {
                                    id: keyLabel
                                    anchors.centerIn: parent
                                    text: bar.gamepad ? (hint.modelData.pad === "Menu" ? "☰" : hint.modelData.pad)
                                          : Theme.remoteHints ? (hint.modelData.pad === "A" ? qsTr("OK") : hint.modelData.pad === "B" ? qsTr("Back") : hint.modelData.pad)
                                          : hint.modelData.key
                                    font.pixelSize: Math.round(Theme.caption * 0.9)
                                    font.weight: Font.DemiBold
                                    color: bar.gamepad ? Theme.textOnAccent : Theme.text
                                }
                            }
                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                text: hint.modelData.text
                                font.pixelSize: bar.plainActions ? Theme.body : Theme.caption
                                color: bar.plainActions ? Theme.text : Theme.textMuted
                            }
                        }
                    }
                }
            }
        }
    }
}
