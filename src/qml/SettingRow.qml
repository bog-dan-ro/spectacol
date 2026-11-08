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
import QtQuick.Layouts
import Spectacol

// A setting in a SettingsList: its label (and a line of explanation) on the left, its control on
// the right. Left and Right change it, Return or Space (the gamepad's A) toggle or trigger it, a
// click or a tap does too; Up and Down go to the next setting (SettingsList).
//   kind "switch": checked, toggled(checked)
//   kind "choice": model (strings), currentIndex, activated(index)
//   kind "stepper": value, from, to, stepSize, valueText(value), moved(value)
//   kind "action": actionText, clicked()
FocusScope {
    id: row

    readonly property bool isSettingRow: true
    property string label
    property string description
    property string kind: "switch"

    property bool checked: false
    signal toggled(bool checked)

    property var model: []
    property int currentIndex: 0
    property real choiceMaximumWidth: 0
    signal activated(int index)

    property real value: 0
    property real from: 0
    property real to: 100
    property real stepSize: 1
    property var valueText: (v) => String(v)
    signal moved(real value)

    property string actionText
    signal clicked

    activeFocusOnTab: true
    width: parent ? parent.width : implicitWidth
    implicitHeight: Math.max(Theme.rowHeight + Theme.sp(3), labels.implicitHeight + Theme.sp(5))

    function change(delta: int) {
        switch (kind) {
        case "switch":
            if ((delta > 0) !== checked)
                toggled(!checked);
            break;
        case "choice": {
            const index = Math.max(0, Math.min(model.length - 1, currentIndex + delta));
            if (index !== currentIndex)
                activated(index);
            break;
        }
        case "stepper": {
            const v = Math.max(from, Math.min(to, Math.round((value + delta * stepSize) / stepSize) * stepSize));
            if (v !== value)
                moved(v);
            break;
        }
        }
    }

    function trigger() {
        switch (kind) {
        case "switch": toggled(!checked); break;
        case "choice": activated((currentIndex + 1) % model.length); break;
        case "action": clicked(); break;
        }
    }

    Keys.onLeftPressed: change(-1)
    Keys.onRightPressed: change(1)
    Keys.onReturnPressed: trigger()
    Keys.onEnterPressed: trigger()
    Keys.onSpacePressed: trigger()

    Rectangle {
        anchors.fill: parent
        anchors.topMargin: Theme.sp(0.5)
        anchors.bottomMargin: Theme.sp(0.5)
        radius: Theme.radiusSmall
        color: row.activeFocus ? Theme.accentFill : "transparent"
        border.width: row.activeFocus ? Theme.focusLine : 0
        border.color: Theme.accent
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {
            row.forceActiveFocus(Qt.MouseFocusReason);
            if (row.kind === "switch" || row.kind === "action")
                row.trigger();
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.sp(3)
        anchors.rightMargin: Theme.sp(3)
        spacing: Theme.sp(4)

        ColumnLayout {
            id: labels
            Layout.fillWidth: true
            spacing: Theme.sp(0.5)
            Text {
                Layout.fillWidth: true
                text: row.label
                wrapMode: Text.Wrap
                font.pixelSize: Theme.body
                color: Theme.text
            }
            Text {
                visible: text !== ""
                Layout.fillWidth: true
                text: row.description
                wrapMode: Text.Wrap
                font.pixelSize: Theme.caption
                color: Theme.textMuted
            }
        }

        // switch
        Rectangle {
            visible: row.kind === "switch"
            Layout.preferredWidth: Theme.sp(11)
            Layout.preferredHeight: Theme.sp(6)
            radius: height / 2
            color: row.checked ? Theme.accent : "#3a404c"
            Rectangle {
                x: row.checked ? parent.width - width - Theme.sp(0.75) : Theme.sp(0.75)
                anchors.verticalCenter: parent.verticalCenter
                width: parent.height - Theme.sp(1.5)
                height: width
                radius: width / 2
                color: row.checked ? Theme.textOnAccent : "#c9ced8"
                Behavior on x { NumberAnimation { duration: Theme.reduceMotion ? 0 : 120; easing.type: Easing.OutCubic } }
            }
        }

        // choice: segments for a few short values, else ‹ value ›
        Row {
            id: segments
            readonly property bool fits: row.model.length <= 3 && row.model.every(t => String(t).length <= 8)
            visible: row.kind === "choice" && fits
            Repeater {
                model: segments.visible ? row.model : []
                delegate: Rectangle {
                    id: segment
                    required property string modelData
                    required property int index
                    readonly property bool selected: index === row.currentIndex
                    width: segmentLabel.implicitWidth + Theme.sp(6)
                    height: Theme.sp(8)
                    color: selected ? Theme.accent : "transparent"
                    border.width: Theme.line
                    border.color: selected ? Theme.accent : Theme.outline
                    radius: Theme.radiusSmall
                    Text {
                        id: segmentLabel
                        anchors.centerIn: parent
                        text: segment.modelData
                        font.pixelSize: Theme.caption
                        font.weight: segment.selected ? Font.DemiBold : Font.Normal
                        color: segment.selected ? Theme.textOnAccent : Theme.textMuted
                    }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            row.forceActiveFocus(Qt.MouseFocusReason);
                            if (segment.index !== row.currentIndex)
                                row.activated(segment.index);
                        }
                    }
                }
            }
        }

        RowLayout {
            visible: (row.kind === "choice" && !segments.fits) || row.kind === "stepper"
            spacing: Theme.sp(2)

            component StepButton: Rectangle {
                id: stepButton
                property string glyph
                property int delta
                property bool active: true
                Layout.preferredWidth: Theme.sp(8)
                Layout.preferredHeight: Theme.sp(8)
                radius: Theme.radiusSmall
                color: "transparent"
                border.width: Theme.line
                border.color: Theme.outline
                opacity: active ? 1 : 0.35
                Text {
                    anchors.centerIn: parent
                    text: stepButton.glyph
                    font.pixelSize: Theme.title
                    color: Theme.text
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        row.forceActiveFocus(Qt.MouseFocusReason);
                        row.change(stepButton.delta);
                    }
                }
            }

            StepButton {
                glyph: row.kind === "stepper" ? "−" : "‹"
                delta: -1
                active: row.kind === "stepper" ? row.value > row.from : row.currentIndex > 0
            }
            Text {
                Layout.minimumWidth: Theme.sp(14)
                Layout.maximumWidth: row.choiceMaximumWidth > 0 ? row.choiceMaximumWidth : Number.POSITIVE_INFINITY
                elide: row.choiceMaximumWidth > 0 ? Text.ElideRight : Text.ElideNone
                horizontalAlignment: Text.AlignHCenter
                text: row.kind === "stepper" ? row.valueText(row.value) : (row.model[row.currentIndex] ?? "")
                font.pixelSize: Theme.body
                font.features: { "tnum": 1 }
                color: Theme.text
            }
            StepButton {
                glyph: row.kind === "stepper" ? "+" : "›"
                delta: 1
                active: row.kind === "stepper" ? row.value < row.to : row.currentIndex < row.model.length - 1
            }
        }

        // action
        Text {
            visible: row.kind === "action"
            text: row.actionText + "  ›"
            font.pixelSize: Theme.body
            color: Theme.textMuted
        }
    }

    Rectangle {
        anchors.bottom: parent.bottom
        x: Theme.sp(3)
        width: parent.width - 2 * Theme.sp(3)
        height: Theme.line
        color: Theme.outline
        visible: !row.activeFocus
    }
}
