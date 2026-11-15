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
    readonly property bool compact: width < Theme.sp(Theme.keyboardHints ? 360 : 250)
    // Wide enough: the disassembly stays on the left and the chosen view sits beside it, with
    // the game between them where there is room. Else one view at a time.
    readonly property bool twoPanes: width >= Theme.sp(190)
    readonly property bool showCode: twoPanes || views.currentIndex === 0
    // Beside the disassembly, "Disassembly" shows the registers
    readonly property int sideView: twoPanes && views.currentIndex === 0 ? 1 : views.currentIndex
    readonly property bool showSide: twoPanes || views.currentIndex !== 0
    readonly property real paneWidth: twoPanes ? Math.min(Theme.sp(115), Math.floor((width - Theme.sp(6)) / 2)) : width - Theme.sp(4)
    readonly property var actions: [
        { text: qsTr("Step"), key: "F11", enabled: ZxEmulator.debuggerReady, run: () => ZxEmulator.debuggerStep() },
        { text: qsTr("Next"), key: "F10", enabled: ZxEmulator.debuggerReady, run: () => ZxEmulator.debuggerNext() },
        { text: qsTr("Out"), key: "F12", enabled: ZxEmulator.debuggerReady, run: () => ZxEmulator.debuggerOut() },
        { text: qsTr("To cursor"), key: "F8", enabled: ZxEmulator.debuggerReady && showCode, run: () => page.runToCursor() },
        { text: qsTr("Breakpoint"), key: "X", enabled: ZxEmulator.debuggerReady && showCode && disassembly.selectedAddress >= 0,
          run: () => ZxEmulator.toggleBreakpoint(disassembly.selectedAddress) },
        { text: ZxEmulator.paused ? qsTr("Continue") : qsTr("Pause"), key: "F5", enabled: true, run: () => page.toggleRun() }
    ]
    // The game stays visible between the panes: a click there must not close the debugger
    MouseArea {
        anchors.fill: parent
    }
    Component.onCompleted: {
        ZxEmulator.processInputEvents = false;
        forceActiveFocus();
        Qt.callLater(focusContent);
    }
    Component.onDestruction: {
        ZxEmulator.processInputEvents = true;
        ZxEmulator.debuggerRun();
    }
    function runToCursor() {
        if (showCode && disassembly.selectedAddress >= 0)
            ZxEmulator.debuggerRunTo(disassembly.selectedAddress);
    }
    function toggleRun() {
        if (ZxEmulator.paused)
            ZxEmulator.debuggerContinue();
        else
            ZxEmulator.debuggerTrap();
    }
    DebuggerGamepad {}
    readonly property bool dialogOpen: views.popup.visible || disassembly.dialogOpen
                                       || (content.item && content.item.dialogOpen === true)
    function closeDialog(): bool {
        if (views.popup.visible) {
            views.popup.close();
            return true;
        }
        if (disassembly.closeDialog())
            return true;
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
        if (!ZxEmulator.debuggerReady)
            return;
        const loaded = content.item as Item;
        if (views.currentIndex === 0 || !loaded)
            disassembly.forceActiveFocus();
        else
            loaded.forceActiveFocus();
    }
    function go(field: FancyTextField, side: bool) {
        const parsed = ZxEmulator.resolveDebuggerAddress(field.text);
        if (!isFinite(parsed) || parsed < 0 || parsed > (side && sideView === 4 ? 255 : 65535))
            return;
        const inspector = content.item as DebuggerStateView;
        if (side && inspector) {
            inspector.goTo(parsed);
            inspector.forceActiveFocus();
        } else {
            ZxEmulator.disassamble(parsed, 0);
            disassembly.forceActiveFocus();
        }
    }
    function focusAddress() {
        if (sideView === 4 || sideView === 5)
            sideAddress.forceActiveFocus(Qt.TabFocusReason);
        else if (views.currentIndex === 6) {
            const symbols = content.item as DebugSymbolsView;
            if (symbols) symbols.focusAddress();
        } else if (views.currentIndex === 2) {
            const memory = content.item as DebuggerMemoryView;
            if (memory)
                memory.focusAddress();
        } else if (showCode)
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
        else if (twoPanes && event.key === Qt.Key_Right && disassembly.activeFocus && content.item) {
            // Between the panes, as the cursor keys suggest
            (content.item as Item).forceActiveFocus(Qt.TabFocusReason);
            event.accepted = true;
        } else if (twoPanes && event.key === Qt.Key_Left && sidePane.activeFocus && !sideAddress.activeFocus && !views.activeFocus) {
            disassembly.forceActiveFocus(Qt.BacktabFocusReason);
            event.accepted = true;
        } else if ([Qt.Key_Up, Qt.Key_Down, Qt.Key_Left, Qt.Key_Right].includes(event.key)) {
            ZxEmulator.sendNavigationKey(event.key === Qt.Key_Up || event.key === Qt.Key_Left ? Qt.Key_Backtab : Qt.Key_Tab);
            event.accepted = true;
        }
    }
    Keys.onEscapePressed: ZxEmulator.debuggerRun()

    component Pane: FocusScope {
        default property alias paneData: paneColumn.data
        Rectangle {
            anchors.fill: parent
            color: Theme.surface
            radius: Theme.radiusSmall
            border.width: parent.activeFocus ? Theme.focusLine : Theme.line
            border.color: parent.activeFocus ? Theme.accent : Theme.outline
        }
        ColumnLayout {
            id: paneColumn
            anchors.fill: parent
            anchors.margins: Theme.sp(2)
            spacing: Theme.sp(2)
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.sp(2)

        // What the debugger is doing, the views and the transport, in one row when it fits
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: bar.implicitHeight + Theme.sp(2)
            color: Theme.surface
            GridLayout {
                id: bar
                anchors.fill: parent
                anchors.leftMargin: Theme.sp(2)
                anchors.rightMargin: Theme.sp(2)
                // Created in focus order: the transport, the views, then the view itself
                columns: page.compact ? 2 : 3
                columnSpacing: Theme.sp(2)
                rowSpacing: 0
                RowLayout {
                    Layout.row: 0
                    Layout.column: 0
                    Layout.fillWidth: true
                    spacing: Theme.sp(2)
                    ToolButton {
                        objectName: "debuggerBack"
                        text: qsTr("Close the debugger")
                        icon.source: "qrc:/images/back.svg"
                        icon.color: Theme.text
                        icon.width: Theme.sp(6)
                        icon.height: Theme.sp(6)
                        display: AbstractButton.IconOnly
                        implicitWidth: Theme.touchTarget
                        implicitHeight: Theme.touchTarget
                        onClicked: ZxEmulator.debuggerRun()
                    }
                    Label {
                        Layout.fillWidth: true
                        Layout.minimumWidth: Theme.sp(20)
                        text: ZxEmulator.debuggerStatus
                        elide: Text.ElideRight
                        maximumLineCount: 2
                        wrapMode: Text.Wrap
                        font.pixelSize: Theme.caption
                        color: Theme.textMuted
                    }
                }
                Flow {
                    id: transport
                    readonly property real total: {
                        let sum = 0;
                        for (let i = 0; i < buttons.count; ++i) {
                            const item = buttons.itemAt(i);
                            sum += (item ? item.implicitWidth : 0) + spacing;
                        }
                        return sum;
                    }
                    Layout.row: page.compact ? 1 : 0
                    Layout.column: page.compact ? 0 : 2
                    Layout.columnSpan: page.compact ? 2 : 1
                    Layout.fillWidth: page.compact
                    Layout.preferredWidth: page.compact ? -1 : total
                    Layout.preferredHeight: childrenRect.height
                    spacing: Theme.sp(1)
                    Repeater {
                        id: buttons
                        model: page.actions
                        delegate: Button {
                            required property var modelData
                            text: Theme.keyboardHints && !page.compact ? qsTr("%1 (%2)").arg(modelData.text).arg(modelData.key) : modelData.text
                            enabled: modelData.enabled
                            implicitHeight: Theme.touchTarget
                            onClicked: modelData.run()
                            Keys.onReturnPressed: modelData.run()
                            Keys.onEnterPressed: modelData.run()
                        }
                    }
                }
                NavigationComboBox {
                    id: views
                    objectName: "debuggerViews"
                    displayText: currentIndex >= 0 ? textAt(currentIndex) : ""
                    enabled: ZxEmulator.debuggerReady
                    Layout.row: 0
                    Layout.column: 1
                    Layout.preferredWidth: Math.min(Theme.sp(70), page.width * 0.5)
                    model: [qsTr("Disassembly"), qsTr("CPU registers"), qsTr("Memory"), qsTr("Breakpoints / watchpoints"), qsTr("Next registers"), qsTr("MMU mappings"), qsTr("Labels / source mappings")]
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: Theme.sp(2)
            Layout.rightMargin: Theme.sp(2)
            spacing: Theme.sp(2)
            // The disassembly is on the left, but after the chosen view in the focus order: from
            // the views list, Down goes into the view
            layoutDirection: Qt.RightToLeft

            Pane {
                id: sidePane
                visible: page.showSide
                Layout.preferredWidth: page.paneWidth
                Layout.fillHeight: true
                RowLayout {
                    visible: page.sideView === 4 || page.sideView === 5
                    Layout.fillWidth: true
                    FancyTextField {
                        id: sideAddress
                        objectName: "debuggerSideAddress"
                        mono: true
                        Layout.fillWidth: true
                        placeholderText: qsTr("Hex address")
                        inputMethodHints: Qt.ImhPreferUppercase | Qt.ImhNoPredictiveText
                        onAccepted: page.go(sideAddress, true)
                    }
                    Button {
                        text: qsTr("Go")
                        enabled: ZxEmulator.debuggerReady
                        implicitHeight: Theme.touchTarget
                        onClicked: page.go(sideAddress, true)
                    }
                }
                Loader {
                    id: content
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    enabled: ZxEmulator.debuggerReady
                    active: page.showSide
                    // Beside the disassembly the registers leave room for the breakpoints
                    sourceComponent: page.twoPanes && page.sideView === 1 ? registersAndBreakpoints
                                     : [registers, registers, memory, breakpoints, nextRegisters, mmu, symbols][page.sideView]
                }
            }

            // The game shows through here
            Item {
                visible: page.twoPanes
                Layout.fillWidth: true
            }

            Pane {
                id: codePane
                visible: page.showCode
                Layout.preferredWidth: page.paneWidth
                Layout.fillHeight: true
                RowLayout {
                    Layout.fillWidth: true
                    FancyTextField {
                        id: goAddress
                        objectName: "debuggerAddress"
                        mono: true
                        Layout.fillWidth: true
                        placeholderText: qsTr("Hex address or label")
                        inputMethodHints: Qt.ImhPreferUppercase | Qt.ImhNoPredictiveText
                        onAccepted: page.go(goAddress, false)
                    }
                    Button {
                        text: qsTr("Go")
                        enabled: ZxEmulator.debuggerReady
                        implicitHeight: Theme.touchTarget
                        onClicked: page.go(goAddress, false)
                    }
                }
                DisassembleView {
                    id: disassembly
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    enabled: ZxEmulator.debuggerReady
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: padHints.implicitHeight + Theme.sp(2)
            visible: Theme.gamepadHints
            color: Theme.surface
            Label {
                id: padHints
                anchors.fill: parent
                anchors.leftMargin: Theme.sp(3)
                anchors.rightMargin: Theme.sp(3)
                verticalAlignment: Text.AlignVCenter
                text: page.compact ? qsTr("A: select · B: back · X: views · Y: address")
                                   : qsTr("D-pad / stick: navigate · A: select · B: back · X: views · Y: address · LB: step · RB: continue/pause · LT: out · RT: next")
                color: Theme.textMuted
                font.pixelSize: Theme.caption
                wrapMode: Text.Wrap
            }
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
        id: registers
        RegistersView {
            enabled: ZxEmulator.debuggerReady
        }
    }
    Component {
        id: registersAndBreakpoints
        FocusScope {
            readonly property bool dialogOpen: breakpointsPart.dialogOpen
            function closeDialog(): bool {
                return breakpointsPart.closeDialog();
            }
            ColumnLayout {
                anchors.fill: parent
                spacing: Theme.sp(2)
                RegistersView {
                    Layout.fillWidth: true
                    Layout.preferredHeight: contentHeight
                    enabled: ZxEmulator.debuggerReady
                }
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: Theme.line
                    color: Theme.outline
                }
                BreakpointsView {
                    id: breakpointsPart
                    focus: true
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }
            }
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
