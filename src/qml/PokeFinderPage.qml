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
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtGamepadLegacy
import Spectacol

// Finds where a game keeps a value (lives, energy): search for it, play until it changes,
// then narrow the locations by the new value or by how it changed. A location can then get a
// breakpoint on the code that reads or writes it.
Sheet {
    id: page

    crumb: qsTr("Machine")
    title: qsTr("Poke finder")
    preferredWidth: Theme.sp(150)

    readonly property bool hasCurrentLocation: view.currentIndex >= 0 && view.currentIndex < view.count
    // The current location's breakpoints: 1 on read, 2 on write
    readonly property int currentFlags: hasCurrentLocation && view.currentItem ? (view.currentItem as LocationRow).breakFlags : 0
    readonly property bool breaksOnRead: (currentFlags & 1) !== 0
    readonly property bool breaksOnWrite: (currentFlags & 2) !== 0
    // The locations are listed only when there are at most 20 of them
    readonly property bool listed: ZxEmulator.pokeFinderCount <= 20

    hints: [
        { pad: "A", key: "Enter", text: qsTr("Search"), action: () => page.search() },
        { pad: "Y", key: "Y", text: qsTr("Increased"), action: () => ZxEmulator.pokeFinderInced() },
        { pad: "X", key: "X", text: qsTr("Decreased"), action: () => ZxEmulator.pokeFinderDeced() },
        { pad: "L2", key: "E", text: qsTr("Start over"), action: () => ZxEmulator.pokeFinderReset() },
        { pad: "B", key: "Esc", text: qsTr("Close"), action: () => page.closeRequested() }
    ].concat(hasCurrentLocation ? [
        { pad: "L1", key: "W", text: page.breaksOnWrite ? qsTr("No break on write") : qsTr("Break on write"),
          action: () => page.toggleBreakpoint(BreakpointsModel.BreakOnWrite) },
        { pad: "R1", key: "R", text: page.breaksOnRead ? qsTr("No break on read") : qsTr("Break on read"),
          action: () => page.toggleBreakpoint(BreakpointsModel.BreakOnRead) }
    ] : [])

    function search() {
        if (value.acceptableInput)
            ZxEmulator.pokeFinderSearch(value.text);
    }

    function toggleBreakpoint(type: int) {
        const row = view.currentItem as LocationRow;
        if (!row)
            return;
        // The model resets: keep the cursor on the location
        const index = view.currentIndex;
        ZxEmulator.togglePokeBreakpoint(row.offset, row.page, type);
        view.currentIndex = index;
    }

    function resetCurrentLocation() {
        view.currentIndex = listed ? 0 : -1;
    }

    Component.onCompleted: ZxEmulator.pokeFinderResetIfNeeded()

    Connections {
        target: ZxEmulator
        function onPokeFinderCountChanged() { page.resetCurrentLocation(); }
    }

    GamepadNavigation {
        gamepad: Gamepad { deviceId: ZxEmulator.gamepadId }
        buttonAKey: Qt.Key_Return
        buttonBKey: Qt.Key_Escape
        buttonYKey: Qt.Key_Y
        buttonXKey: Qt.Key_X
        buttonL1Key: Qt.Key_W
        buttonR1Key: Qt.Key_R
        buttonL2Key: Qt.Key_E
        buttonR2Key: Qt.Key_E
    }

    Keys.onPressed: event => {
        switch (event.key) {
        case Qt.Key_Y:
            ZxEmulator.pokeFinderInced();
            break;
        case Qt.Key_X:
            ZxEmulator.pokeFinderDeced();
            break;
        case Qt.Key_E:
            ZxEmulator.pokeFinderReset();
            break;
        case Qt.Key_W:
            toggleBreakpoint(BreakpointsModel.BreakOnWrite);
            break;
        case Qt.Key_R:
            toggleBreakpoint(BreakpointsModel.BreakOnRead);
            break;
        case Qt.Key_Up:
        case Qt.Key_Left:
            ZxEmulator.sendNavigationKey(Qt.Key_Backtab);
            break;
        case Qt.Key_Down:
        case Qt.Key_Right:
            ZxEmulator.sendNavigationKey(Qt.Key_Tab);
            break;
        default:
            return;
        }
        event.accepted = true;
    }

    // Return is the gamepad's A: buttons only take Space themselves
    component ActionButton: Button {
        implicitHeight: Theme.touchTarget
        Keys.onReturnPressed: clicked()
        Keys.onEnterPressed: clicked()
    }

    component LocationRow: Item {
        id: row
        required property int index
        required property int page
        required property int offset
        required property string offsetText
        required property var value
        required property string breakpoint
        required property int breakFlags
        width: ListView.view.width
        height: Theme.listRowHeight
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: Theme.sp(3)
            anchors.rightMargin: Theme.sp(3)
            spacing: Theme.sp(4)
            FancyText { mono: true; Layout.preferredWidth: Theme.sp(12); text: String(row.page) }
            FancyText { mono: true; Layout.preferredWidth: Theme.sp(18); text: row.offsetText }
            FancyText { mono: true; Layout.preferredWidth: Theme.sp(10); text: String(row.value) }
            FancyText { Layout.fillWidth: true; elide: Text.ElideRight; color: Theme.warning; text: row.breakpoint }
        }
        TapHandler {
            onTapped: {
                row.ListView.view.currentIndex = row.index;
                row.ListView.view.forceActiveFocus(Qt.MouseFocusReason);
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.sp(4)
        spacing: Theme.sp(3)

        FancyText {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            color: Theme.textMuted
            font.pixelSize: Theme.caption
            text: qsTr("Search for a value such as the number of lives. Play until it changes, then search for the new value, or say that it increased or decreased.")
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.sp(2)
            FancyTextField {
                id: value
                objectName: "pokeFinderValue"
                mono: true
                focus: true
                Layout.fillWidth: true
                inputMethodHints: Qt.ImhDigitsOnly
                validator: IntValidator { bottom: 0; top: 255 }
                placeholderText: qsTr("Value, 0 to 255")
                onAccepted: {
                    page.search();
                    if (page.listed)
                        view.forceActiveFocus(Qt.TabFocusReason);
                }
            }
            ActionButton {
                text: qsTr("Search")
                enabled: value.acceptableInput
                onClicked: page.search()
            }
        }

        Flow {
            Layout.fillWidth: true
            spacing: Theme.sp(2)
            ActionButton { text: qsTr("Increased"); onClicked: ZxEmulator.pokeFinderInced() }
            ActionButton { text: qsTr("Decreased"); onClicked: ZxEmulator.pokeFinderDeced() }
            ActionButton { text: qsTr("Start over"); onClicked: ZxEmulator.pokeFinderReset() }
        }

        FancyText {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            text: page.listed ? qsTr("%n possible location(s)", "", ZxEmulator.pokeFinderCount)
                              : qsTr("%n possible location(s): they are listed from 20 down", "", ZxEmulator.pokeFinderCount)
        }

        RowLayout {
            visible: page.listed && view.count > 0
            Layout.fillWidth: true
            Layout.leftMargin: Theme.sp(3)
            Layout.rightMargin: Theme.sp(3)
            spacing: Theme.sp(4)
            component Heading: FancyText {
                font.pixelSize: Theme.caption
                color: Theme.textMuted
            }
            Heading { Layout.preferredWidth: Theme.sp(12); text: qsTr("Bank") }
            Heading { Layout.preferredWidth: Theme.sp(18); text: qsTr("Offset") }
            Heading { Layout.preferredWidth: Theme.sp(10); text: qsTr("Value") }
            Heading { Layout.fillWidth: true; text: qsTr("Breakpoint") }
        }

        ListView {
            id: view
            objectName: "pokeFinderLocations"
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            activeFocusOnTab: page.listed && count > 0
            boundsBehavior: Flickable.StopAtBounds
            highlightMoveDuration: 0
            model: ZxEmulator.pokeFinderModel
            delegate: LocationRow {}
            ScrollIndicator.vertical: ScrollIndicator {}
            highlight: Rectangle {
                radius: Theme.radiusSmall
                color: Theme.accentFill
                border.width: view.activeFocus ? Theme.focusLine : 0
                border.color: Theme.accent
            }
            Component.onCompleted: page.resetCurrentLocation()
            Keys.onUpPressed: event => {
                if (currentIndex > 0)
                    decrementCurrentIndex();
                else
                    event.accepted = false;
            }
            Keys.onDownPressed: event => {
                if (currentIndex < count - 1)
                    incrementCurrentIndex();
                else
                    event.accepted = false;
            }
        }

        Flow {
            Layout.fillWidth: true
            spacing: Theme.sp(2)
            ActionButton {
                objectName: "pokeFinderBreakWrite"
                text: page.breaksOnWrite ? qsTr("No break on write") : qsTr("Break on write")
                highlighted: page.breaksOnWrite
                enabled: page.hasCurrentLocation
                onClicked: page.toggleBreakpoint(BreakpointsModel.BreakOnWrite)
            }
            ActionButton {
                objectName: "pokeFinderBreakRead"
                text: page.breaksOnRead ? qsTr("No break on read") : qsTr("Break on read")
                highlighted: page.breaksOnRead
                enabled: page.hasCurrentLocation
                onClicked: page.toggleBreakpoint(BreakpointsModel.BreakOnRead)
            }
        }
    }
}
