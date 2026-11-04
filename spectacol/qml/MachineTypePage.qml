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
import Spectacol

// The machines, grouped by family. The arrows move, Return (the gamepad's A) or a click switches
Sheet {
    id: page

    readonly property var groups: [qsTr("Sinclair"), qsTr("Next"), qsTr("Romanian"), qsTr("Clones"), qsTr("Timex")]
    property int focusIndex: ZxEmulator.settings.currentMachineIndex

    function groupOf(name: string): int {
        if (name.startsWith("Timex"))
            return 4;
        if (/Pentagon|Scorpion/.test(name))
            return 3;
        if (/HC-2000|CoBra/.test(name))
            return 2;
        if (/Next/.test(name))
            return 1;
        return 0;
    }

    function shortName(name: string): string {
        return name.startsWith("ZX Spectrum ") && !/Next/.test(name) ? name.substring(12) : name;
    }

    function openRoms() {
        pageLoader.setSource("RomsPage.qml", {machineIndex: focusIndex});
    }
    Keys.onPressed: event => { if (event.key === Qt.Key_F2) { openRoms(); event.accepted = true; } }
    function choose(index: int) {
        if (index !== ZxEmulator.settings.currentMachineIndex)
            ZxEmulator.settings.currentMachineIndex = index;
        pageLoader.source = "";
    }

    // The chip next to the focused one in a direction, by position
    function moveFocus(dx: int, dy: int) {
        const chips = [];
        for (const group of groupsColumn.children) {
            if (group.chips)
                chips.push(...group.chips());
        }
        const current = chips.find(c => c.machineIndex === focusIndex);
        if (!current)
            return;
        const at = (c) => c.mapToItem(groupsColumn, c.width / 2, c.height / 2);
        const here = at(current);
        let best = null, bestScore = Infinity;
        for (const chip of chips) {
            if (chip === current)
                continue;
            const p = at(chip);
            const ddx = p.x - here.x, ddy = p.y - here.y;
            if ((dy !== 0 && Math.sign(ddy) !== dy) || (dy !== 0 && Math.abs(ddy) < 1))
                continue;
            if (dx !== 0 && (Math.abs(ddy) > 1 || Math.sign(ddx) !== dx))
                continue;
            const score = dy !== 0 ? Math.abs(ddy) * 1000 + Math.abs(ddx) : Math.abs(ddx);
            if (score < bestScore) {
                best = chip;
                bestScore = score;
            }
        }
        if (best) {
            focusIndex = best.machineIndex;
            flick.ensureVisible(best);
        }
    }

    crumb: qsTr("Machine")
    title: qsTr("Choose a machine")
    hints: [
        { pad: "A", key: "Enter", text: qsTr("Switch"), action: () => page.choose(page.focusIndex) },
        { pad: "X", key: "F2", text: qsTr("ROMs"), action: () => page.openRoms() },
        { pad: "B", key: "Esc", text: qsTr("Back"), action: () => pageLoader.source = "" }
    ]

    Component.onCompleted: ZxEmulator.processInputEvents = false
    Component.onDestruction: ZxEmulator.processInputEvents = true

    DebuggerGamepad { transportKeys: false }


    Flickable {
        id: flick
        anchors.fill: parent
        contentHeight: groupsColumn.implicitHeight + Theme.sp(6)
        clip: true
        focus: true
        boundsBehavior: Flickable.StopAtBounds
        ScrollIndicator.vertical: ScrollIndicator {}

        function ensureVisible(item) {
            const y = item.mapToItem(groupsColumn, 0, 0).y + groupsColumn.y;
            if (y < contentY)
                contentY = Math.max(0, y - Theme.sp(8));
            else if (y + item.height > contentY + height)
                contentY = Math.min(contentHeight - height, y + item.height - height + Theme.sp(4));
        }

        Keys.onLeftPressed: page.moveFocus(-1, 0)
        Keys.onRightPressed: page.moveFocus(1, 0)
        Keys.onUpPressed: page.moveFocus(0, -1)
        Keys.onDownPressed: page.moveFocus(0, 1)
        Keys.onReturnPressed: page.choose(page.focusIndex)
        Keys.onEnterPressed: page.choose(page.focusIndex)
        Keys.onSpacePressed: page.choose(page.focusIndex)

        Column {
            id: groupsColumn
            x: Theme.sp(6)
            y: Theme.sp(2)
            width: flick.width - 2 * Theme.sp(6)
            spacing: Theme.sp(1)

            Repeater {
                model: page.groups
                delegate: Column {
                    id: group
                    required property string modelData
                    required property int index
                    width: groupsColumn.width
                    spacing: Theme.sp(2)

                    function chips() {
                        const result = [];
                        for (const child of flow.children) {
                            if (child.machineIndex !== undefined && child.visible)
                                result.push(child);
                        }
                        return result;
                    }

                    SectionTitle {
                        leftPadding: 0
                        text: group.modelData
                    }

                    Flow {
                        id: flow
                        width: parent.width
                        spacing: Theme.sp(2)

                        Repeater {
                            model: ZxEmulator.settings.machinesModel
                            delegate: Rectangle {
                                id: chip
                                required property string modelData
                                required property int index
                                readonly property int machineIndex: index
                                readonly property bool current: index === ZxEmulator.settings.currentMachineIndex
                                readonly property bool focused: index === page.focusIndex

                                visible: page.groupOf(modelData) === group.index
                                width: visible ? label.implicitWidth + Theme.sp(6) : 0
                                height: visible ? Theme.sp(10) : 0
                                radius: Theme.radiusSmall
                                color: focused ? Theme.accentFill : "transparent"
                                border.width: focused ? Theme.focusLine : Theme.line
                                border.color: focused ? Theme.accent : current ? Theme.textMuted : Theme.outline

                                Text {
                                    id: label
                                    anchors.centerIn: parent
                                    text: (chip.current ? "✓ " : "") + page.shortName(chip.modelData)
                                    font.pixelSize: Theme.body
                                    color: Theme.text
                                }
                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: page.choose(chip.machineIndex)
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
