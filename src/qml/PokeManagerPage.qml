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
import QtQuick.Controls
import QtQuick.Layouts
import Spectacol

FocusScope {
    id: page
    readonly property PokeModel pokes: ZxEmulator.pokes
    Component.onCompleted: {
        ZxEmulator.processInputEvents = false;
        pokes.refreshBanks();
        view.forceActiveFocus();
    }
    Component.onDestruction: ZxEmulator.processInputEvents = true
    function addPoke() {
        editor.returnFocus = addButton;
        editor.open();
    }
    function closeDialog(): bool {
        if (trainerFile.closeNested() || trainerFolder.closeNested()) return true;
        if (valuesDialog.visible) { valuesDialog.reject(); return true; }
        if (bank.popup.visible) {
            bank.popup.close();
            return true;
        }
        if (!editor.visible)
            return false;
        editor.reject();
        return true;
    }
    function toggleCurrent() {
        const row = view.currentItem;
        if (!row) return;
        const requested = pokes.requestedValues(row.index);
        if (!row.active && requested.length) {
            valuesDialog.row = row.index;
            valuesDialog.requests = requested;
            valuesDialog.returnFocus = view;
            valuesDialog.open();
        } else pokes.setActive(row.index, !row.active);
    }
    Keys.onPressed: event => {
        if (editor.visible)
            return;
        switch (event.key) {
        case Qt.Key_F3:
        case Qt.Key_Menu:
            addPoke();
            break;
        case Qt.Key_F2:
            pokes.clear();
            break;
        case Qt.Key_Escape:
            pageLoader.source = "";
            break;
        default:
            if (![Qt.Key_Up, Qt.Key_Down, Qt.Key_Left, Qt.Key_Right].includes(event.key))
                return;
            ZxEmulator.sendNavigationKey(event.key === Qt.Key_Up || event.key === Qt.Key_Left ? Qt.Key_Backtab : Qt.Key_Tab);
            break;
        }
        event.accepted = true;
    }
    DebuggerGamepad {
        transportKeys: false
    }
    Rectangle {
        anchors.fill: parent
        color: Theme.raised
        z: -1
    }
    HostPathDialog {
        id: trainerFile
        title: qsTr("Load POK trainers")
        nameFilters: [qsTr("POK trainers (*.pok *.POK)")]
        submit: path => pokes.loadPok(path)
    }
    HostPathDialog {
        id: trainerFolder
        title: qsTr("Choose POKES folder")
        directory: true
        path: pokes.pokesFolder
        submit: path => pokes.choosePokesFolder(path)
    }
    FormDialog {
        id: valuesDialog
        property int row: -1
        property var requests: []
        title: qsTr("Trainer values")
        commit: () => {
            const values = [];
            for (let i = 0; i < valueFields.count; ++i) {
                const field = valueFields.itemAt(i);
                if (!field.acceptableInput) return false;
                values.push(Number.fromLocaleString(Qt.locale(field.validator.locale), field.text));
            }
            return pokes.setActive(row, true, values);
        }
        onOpened: if (valueFields.count) valueFields.itemAt(0).forceActiveFocus()
        Repeater {
            id: valueFields
            model: valuesDialog.requests
            delegate: TextField {
                required property string modelData
                Layout.fillWidth: true
                placeholderText: modelData
                inputMethodHints: Qt.ImhDigitsOnly
                validator: IntValidator { bottom: 0; top: 255 }
                onAccepted: valuesDialog.save()
            }
        }
    }
    FormDialog {
        id: editor
        title: qsTr("Add poke")
        initialFocus: bank
        acceptable: address.acceptableInput && value.acceptableInput
        commit: () => {
            if (!pokes.addPoke(pokes.selectedBank, Number.fromLocaleString(Qt.locale(address.validator.locale), address.text), Number.fromLocaleString(Qt.locale(value.validator.locale), value.text)))
                return false;
            view.currentIndex = view.count - 1;
            return true;
        }
        Label { text: qsTr("Writable memory bank"); color: Theme.text }
        NavigationComboBox {
            id: bank
            objectName: "pokeBank"
            Layout.fillWidth: true
            model: pokes.bankNames
            currentIndex: pokes.selectedBank
            onActivated: pokes.selectedBank = currentIndex
        }
        Label {
            text: qsTr("Address / bank offset (decimal, 0–%1)").arg(pokes.addressLimit)
            color: Theme.text
            Layout.fillWidth: true
            wrapMode: Text.Wrap
        }
        TextField {
            id: address
            objectName: "pokeAddress"
            Layout.fillWidth: true
            inputMethodHints: Qt.ImhDigitsOnly
            validator: IntValidator { bottom: 0; top: pokes.addressLimit }
            onAccepted: value.forceActiveFocus(Qt.TabFocusReason)
        }
        Label { text: qsTr("Value (decimal, 0–255)"); color: Theme.text }
        TextField {
            id: value
            objectName: "pokeValue"
            Layout.fillWidth: true
            inputMethodHints: Qt.ImhDigitsOnly
            validator: IntValidator { bottom: 0; top: 255 }
            onAccepted: editor.save()
        }
    }
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.sp(3)
        Label {
            text: qsTr("Poke memory")
            color: Theme.text
            font.pixelSize: Theme.pageTitle
        }
        Label {
            Layout.fillWidth: true
            text: qsTr("CPU write addresses or offsets in a writable physical bank. Addresses and values are decimal.")
            wrapMode: Text.Wrap
            color: Theme.textMuted
        }
        Flow {
            Layout.fillWidth: true
            Layout.preferredHeight: childrenRect.height
            spacing: Theme.sp(2)
            Button {
                id: addButton
                objectName: "pokeAdd"
                text: Theme.gamepadHints ? qsTr("Add poke (Y)") : qsTr("Add poke")
                onClicked: page.addPoke()
            }
            Button {
                text: Theme.gamepadHints ? qsTr("Clear list (X)") : qsTr("Clear list")
                onClicked: pokes.clear()
            }
            Button {
                text: Theme.gamepadHints ? qsTr("Close (B)") : qsTr("Close")
                onClicked: pageLoader.source = ""
            }
        }
        RowLayout {
            Layout.fillWidth: true
            Button {
                id: loadTrainer
                objectName: "loadTrainers"
                text: qsTr("Load .pok…")
                onClicked: { trainerFile.returnFocus = loadTrainer; trainerFile.open(); }
            }
            Button {
                id: chooseFolder
                objectName: "pokesFolder"
                text: qsTr("POKES folder…")
                onClicked: { trainerFolder.returnFocus = chooseFolder; trainerFolder.open(); }
            }
        }
        Label {
            Layout.fillWidth: true
            text: pokes.sourcePath || qsTr("No associated trainers found. Load a .pok file or choose a POKES folder.")
            elide: Text.ElideMiddle
            color: Theme.textMuted
        }
        ListView {
            id: view
            objectName: "manualPokes"
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: pokes
            clip: true
            focus: true
            activeFocusOnTab: true
            Keys.onUpPressed: event => {
                if (currentIndex > 0)
                    decrementCurrentIndex();
                else
                    ZxEmulator.sendNavigationKey(Qt.Key_Backtab);
            }
            Keys.onDownPressed: event => {
                if (currentIndex < count - 1)
                    incrementCurrentIndex();
                else
                    ZxEmulator.sendNavigationKey(Qt.Key_Tab);
            }
            Keys.onReturnPressed: page.toggleCurrent()
            Keys.onSelectPressed: page.toggleCurrent()
            Keys.onSpacePressed: page.toggleCurrent()
            delegate: ItemDelegate {
                id: row
                required property int index
                required property string label
                required property bool active
                required property bool disabled
                objectName: "trainerRow" + index
                width: view.width
                enabled: !disabled
                highlighted: view.currentIndex === index
                text: (active ? "☑ " : "☐ ") + label
                onClicked: {
                    view.currentIndex = index;
                    page.toggleCurrent();
                    view.forceActiveFocus();
                }
            }
            ScrollBar.vertical: ScrollBar {}
        }
        Label {
            Layout.fillWidth: true
            text: (Theme.gamepadHints ? qsTr("A: toggle poke · B: back · X: clear list · Y: add") + "\n" : "")
                  + qsTr("Turning a trainer off restores its original bytes. Clearing the list keeps the current memory values.")
            color: Theme.textMuted
            font.pixelSize: Theme.caption
            wrapMode: Text.Wrap
        }
    }
}
