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
    id: root
    signal goTo(int address)
    readonly property bool dialogOpen: chooser.visible
    function closeDialog(): bool { return chooser.closeNested(); }
    function focusAddress() { search.forceActiveFocus(); }
    function jump(row: int) {
        const address = ZxEmulator.symbolAddress(row);
        if (address >= 0) goTo(address);
    }
    HostPathDialog {
        id: chooser
        title: qsTr("Import assembler labels / SLD")
        nameFilters: [qsTr("Symbols / source mappings (*.sym *.sld *.sld.txt *.map)"), qsTr("All files (*)")]
        submit: path => ZxEmulator.symbols.load(path)
        Label {
            Layout.fillWidth: true
            text: ZxEmulator.symbols.error
            color: Theme.danger
            wrapMode: Text.Wrap
            visible: text.length > 0
        }
    }
    Connections {
        target: ZxEmulator.symbols
        function onModelReset() {
            Qt.callLater(() => labels.currentIndex = labels.count ? 0 : -1);
        }
    }
    ColumnLayout {
        anchors.fill: parent
        RowLayout {
            Layout.fillWidth: true
            Button {
                id: importButton
                objectName: "importSymbols"
                text: qsTr("Import…")
                onClicked: { chooser.returnFocus = importButton; chooser.open(); }
            }
            Button {
                objectName: "clearSymbols"
                text: qsTr("Clear")
                onClicked: ZxEmulator.symbols.clear()
            }
            TextField {
                id: search
                objectName: "symbolsSearch"
                Layout.fillWidth: true
                placeholderText: qsTr("Find label")
                text: ZxEmulator.symbols.search
                onTextEdited: ZxEmulator.symbols.search = text
                onAccepted: labels.forceActiveFocus()
                Keys.onDownPressed: labels.forceActiveFocus()
            }
        }
        Label {
            Layout.fillWidth: true
            text: ZxEmulator.symbols.path ? qsTr("%1 labels · %2").arg(ZxEmulator.symbols.count).arg(ZxEmulator.symbols.path)
                : qsTr("Import sjasmplus SLD or label EQU address symbols. Banked labels require the matching RAM page to be mapped.")
            color: Theme.textMuted
            wrapMode: Text.Wrap
        }
        ListView {
            id: labels
            objectName: "symbolLabels"
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            focus: true
            activeFocusOnTab: true
            model: ZxEmulator.symbols
            Keys.onReturnPressed: root.jump(currentIndex)
            Keys.onSelectPressed: root.jump(currentIndex)
            Keys.onUpPressed: event => {
                if (currentIndex > 0) decrementCurrentIndex();
                else ZxEmulator.sendNavigationKey(Qt.Key_Backtab);
            }
            delegate: ItemDelegate {
                required property int index
                required property string label
                required property string location
                width: labels.width
                text: label + "  " + location
                highlighted: labels.currentIndex === index
                onClicked: { labels.currentIndex = index; root.jump(index); }
            }
            ScrollBar.vertical: ScrollBar {}
        }
    }
}
