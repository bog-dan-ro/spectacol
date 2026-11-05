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

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Spectacol

FocusScope {
    id: page
    readonly property bool pausesEmulation: false
    readonly property bool compact: width < Theme.sp(150)
    Rectangle {
        anchors.fill: parent
        z: -1
        color: Theme.raised
    }
    Component.onCompleted: {
        ZxEmulator.processInputEvents = false;
        forceActiveFocus();
        Qt.callLater(() => {
            const loaded = content.item as Item;
            if (loaded)
                loaded.forceActiveFocus();
        });
    }
    Component.onDestruction: {
        ZxEmulator.processInputEvents = true;
        ZxEmulator.debuggerRun();
    }
    function runToCursor() {
        const disassembly = content.item as DisassembleView;
        if (views.currentIndex === 0 && disassembly)
            ZxEmulator.debuggerRunTo(disassembly.selectedAddress);
    }
    function toggleRun() {
        if (ZxEmulator.paused)
            ZxEmulator.debuggerContinue();
        else
            ZxEmulator.debuggerTrap();
    }
    DebuggerGamepad {}
    readonly property bool dialogOpen: views.popup.visible || (content.item && content.item.dialogOpen === true)
    function closeDialog(): bool {
        if (views.popup.visible) {
            views.popup.close();
            return true;
        }
        return content.item && typeof content.item.closeDialog === "function" && content.item.closeDialog();
    }
    Connections {
        target: ZxEmulator
        function onPausedChanged() {
            if (ZxEmulator.debuggerReady && !page.dialogOpen)
                page.focusContent();
        }
    }
    function focusContent() {
        const loaded = content.item as Item;
        if (loaded && ZxEmulator.debuggerReady)
            loaded.forceActiveFocus();
    }
    function focusAddress() {
        if (views.currentIndex === 6) {
            const symbols = content.item as DebugSymbolsView;
            if (symbols) symbols.focusAddress();
        } else if (views.currentIndex === 2) {
            const memory = content.item as DebuggerMemoryView;
            if (memory)
                memory.focusAddress();
        } else if (goAddress.visible)
            goAddress.forceActiveFocus(Qt.TabFocusReason);
    }
    // Handle these before Main's gameplay shortcuts, including during an async run.
    function keyPress(key: int, modifiers: int): bool {
        if (dialogOpen)
            return [Qt.Key_F2, Qt.Key_F3, Qt.Key_F5, Qt.Key_F8, Qt.Key_F10, Qt.Key_F11, Qt.Key_F12, Qt.Key_Menu].includes(key);
        switch (key) {
        case Qt.Key_F2:
            views.forceActiveFocus();
            views.popup.open();
            return true;
        case Qt.Key_F3:
            focusAddress();
            return true;
        case Qt.Key_F11:
            if (ZxEmulator.debuggerReady) {
                if (modifiers & Qt.ShiftModifier)
                    ZxEmulator.debuggerOut();
                else
                    ZxEmulator.debuggerStep();
            }
            return true;
        case Qt.Key_F10:
            if (ZxEmulator.debuggerReady) {
                if (modifiers & Qt.ControlModifier)
                    runToCursor();
                else
                    ZxEmulator.debuggerNext();
            }
            return true;
        case Qt.Key_F12:
            if (ZxEmulator.debuggerReady)
                ZxEmulator.debuggerOut();
            return true;
        case Qt.Key_F8:
            if (ZxEmulator.debuggerReady)
                runToCursor();
            return true;
        case Qt.Key_F5:
            toggleRun();
            return true;
        }
        return false;
    }
    Keys.onPressed: event => {
        if (keyPress(event.key, event.modifiers))
            event.accepted = true;
        else if ([Qt.Key_Up, Qt.Key_Down, Qt.Key_Left, Qt.Key_Right].includes(event.key)) {
            ZxEmulator.sendNavigationKey(event.key === Qt.Key_Up || event.key === Qt.Key_Left ? Qt.Key_Backtab : Qt.Key_Tab);
            event.accepted = true;
        }
    }
    Keys.onEscapePressed: ZxEmulator.debuggerRun()
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.sp(3)
        spacing: Theme.sp(2)
        Label {
            text: qsTr("Debugger")
            font.pixelSize: Theme.sp(5.5)
            color: Theme.text
        }
        Label {
            Layout.fillWidth: true
            text: ZxEmulator.debuggerStatus
            color: Theme.textMuted
            wrapMode: Text.Wrap
        }
        Flow {
            Layout.fillWidth: true
            Layout.preferredHeight: childrenRect.height
            spacing: Theme.sp(1)
            Button {
                text: !Theme.keyboardHints || page.compact ? qsTr("Step") : qsTr("Step (F11)")
                enabled: ZxEmulator.debuggerReady
                onClicked: ZxEmulator.debuggerStep()
            }
            Button {
                text: !Theme.keyboardHints || page.compact ? qsTr("Next") : qsTr("Next (F10)")
                enabled: ZxEmulator.debuggerReady
                onClicked: ZxEmulator.debuggerNext()
            }
            Button {
                text: !Theme.keyboardHints || page.compact ? qsTr("Out") : qsTr("Out (F12)")
                enabled: ZxEmulator.debuggerReady
                onClicked: ZxEmulator.debuggerOut()
            }
            Button {
                text: !Theme.keyboardHints || page.compact ? qsTr("Run to cursor") : qsTr("Run to cursor (F8)")
                enabled: ZxEmulator.debuggerReady && views.currentIndex === 0
                onClicked: page.runToCursor()
            }
            Button {
                text: ZxEmulator.paused ? (!Theme.keyboardHints || page.compact ? qsTr("Continue") : qsTr("Continue (F5)")) : (!Theme.keyboardHints || page.compact ? qsTr("Pause") : qsTr("Pause (F5)"))
                onClicked: page.toggleRun()
            }
            Button {
                text: qsTr("Refresh")
                enabled: ZxEmulator.debuggerReady
                onClicked: ZxEmulator.debuggerRefresh()
            }
            Button {
                text: qsTr("Close")
                onClicked: ZxEmulator.debuggerRun()
            }
        }
        RowLayout {
            Layout.fillWidth: true
            NavigationComboBox {
                id: views
                objectName: "debuggerViews"
                displayText: currentIndex >= 0 ? textAt(currentIndex) : ""
                enabled: ZxEmulator.debuggerReady
                Layout.fillWidth: true
                model: [qsTr("Disassembly"), qsTr("CPU registers"), qsTr("Memory"), qsTr("Breakpoints / watchpoints"), qsTr("Next registers"), qsTr("MMU mappings"), qsTr("Labels / source mappings")]
            }
            TextField {
                id: goAddress
                objectName: "debuggerAddress"
                visible: views.currentIndex === 0 || views.currentIndex === 4 || views.currentIndex === 5
                Layout.preferredWidth: Theme.sp(25)
                placeholderText: views.currentIndex === 0 ? qsTr("Hex / label") : qsTr("Hex address")
                inputMethodHints: Qt.ImhPreferUppercase | Qt.ImhNoPredictiveText
                onAccepted: goButton.click()
            }
            Button {
                id: goButton
                text: qsTr("Go")
                visible: goAddress.visible
                enabled: ZxEmulator.debuggerReady
                onClicked: {
                    const parsed = ZxEmulator.resolveDebuggerAddress(goAddress.text);
                    if (isFinite(parsed) && parsed >= 0 && parsed <= (views.currentIndex === 4 ? 255 : 65535)) {
                        const inspector = content.item as DebuggerStateView;
                        if (inspector)
                            inspector.goTo(parsed);
                        else
                            ZxEmulator.disassamble(parsed, 0);
                        page.focusContent();
                    }
                }
            }
        }
        Loader {
            id: content
            Layout.fillWidth: true
            Layout.fillHeight: true
            enabled: ZxEmulator.debuggerReady
            sourceComponent: [disassembly, registers, memory, breakpoints, nextRegisters, mmu, symbols][views.currentIndex]
        }
        Label {
            Layout.fillWidth: true
            visible: Theme.gamepadHints
            text: page.compact ? qsTr("A: select · B: back · X: views · Y: address") : qsTr("D-pad / stick: navigate · A: select · B: back · X: views · Y: address\nLB: step · RB: continue/pause · LT: out · RT: next")
            color: Theme.textMuted
            font.pixelSize: Theme.caption
            wrapMode: Text.Wrap
        }
    }
    Component {
        id: symbols
        DebugSymbolsView {
            onGoTo: address => {
                views.currentIndex = 0;
                ZxEmulator.disassamble(address, 0);
                page.focusContent();
            }
        }
    }
    Component {
        id: disassembly
        DisassembleView {
            enabled: ZxEmulator.debuggerReady
        }
    }
    Component {
        id: registers
        RegistersView {
            enabled: ZxEmulator.debuggerReady
        }
    }
    Component {
        id: memory
        DebuggerMemoryView {}
    }
    Component {
        id: breakpoints
        BreakpointsView {}
    }
    Component {
        id: nextRegisters
        DebuggerStateView {
            stateModel: ZxEmulator.debugNextRegisters
            unavailableText: qsTr("Next registers are available on ZX Spectrum Next machines.")
        }
    }
    Component {
        id: mmu
        DebuggerStateView {
            addressStep: 1024
            stateModel: ZxEmulator.debugMmu
        }
    }
}
