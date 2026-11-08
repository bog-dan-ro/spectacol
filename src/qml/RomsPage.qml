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
import QtQuick.Controls
import Spectacol

Sheet {
    id: page
    property int machineIndex: ZxEmulator.settings.currentMachineIndex
    crumb: qsTr("Machine")
    title: qsTr("Custom ROMs")
    Component.onCompleted: {
        ZxEmulator.processInputEvents = false;
        ZxEmulator.roms.machineIndex = machineIndex;
    }
    Component.onDestruction: ZxEmulator.processInputEvents = true
    function backToMachines() {
        pageLoader.setSource("MachineTypePage.qml", {focusIndex: ZxEmulator.roms.machineIndex});
    }
    closeAction: () => page.backToMachines()
    function closeDialog(): bool {
        if (chooser.closeNested()) return true;
        backToMachines();
        return true;
    }
    DebuggerGamepad { transportKeys: false }
    Keys.onPressed: event => {
        if (event.key === Qt.Key_F2 || event.key === Qt.Key_F3 || event.key === Qt.Key_Menu) event.accepted = true;
    }
    HostPathDialog {
        id: chooser
        property int slot: -1
        title: qsTr("Choose custom ROM")
        nameFilters: [qsTr("ROM images (*.rom *.bin)"), qsTr("All files (*)")]
        submit: path => ZxEmulator.roms.useFile(slot, path)
        returnFocus: romList
        onClosed: Qt.callLater(() => {
            const rows = romList.settingRows();
            const row = rows[Math.max(0, Math.min(rows.length - 1, slot + 1))];
            if (row) { row.forceActiveFocus(); romList.ensureVisible(row); }
        })
        Label {
            Layout.fillWidth: true
            text: ZxEmulator.roms.error
            visible: text.length > 0
            wrapMode: Text.Wrap
            color: Theme.danger
        }
    }
    hints: [{ pad: "A", key: "Enter", text: qsTr("Choose") },
            { pad: "B", key: "Esc", text: qsTr("Back"), action: () => page.backToMachines() }]
    SettingsList {
        id: romList
        SettingRow {
            objectName: "romMachine"
            label: qsTr("Machine")
            kind: "choice"
            model: ZxEmulator.roms.machines
            currentIndex: ZxEmulator.roms.machineIndex
            choiceMaximumWidth: Theme.sp(48)
            onActivated: index => ZxEmulator.roms.machineIndex = index
        }
        SectionTitle { text: qsTr("Applied at the next hard reset or machine switch") }
        Repeater {
            model: ZxEmulator.roms.romSlots
            delegate: SettingRow {
                id: slotRow
                required property var modelData
                required property int index
                objectName: "romSlot" + index
                label: modelData.name
                description: qsTr("%1 bytes · %2").arg(modelData.size).arg(modelData.source)
                kind: "action"
                actionText: qsTr("Choose")
                onClicked: {
                    chooser.slot = index;
                    chooser.open();
                }
            }
        }
        SettingRow {
            objectName: "romRestore"
            label: qsTr("Restore default ROMs for this machine")
            description: qsTr("Other machines keep their custom ROMs. Next defaults come from its SD card.")
            kind: "action"
            actionText: qsTr("Restore")
            onClicked: ZxEmulator.roms.restoreMachine()
        }
        SettingRow {
            objectName: "romRestart"
            label: qsTr("Hard reset the running machine")
            description: qsTr("Restarts emulation and applies its selected ROMs.")
            kind: "action"
            actionText: qsTr("Restart")
            onClicked: { ZxEmulator.hardReset(); pageLoader.source = ""; }
        }
    }
}
