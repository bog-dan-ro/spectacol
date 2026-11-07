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
import QtGamepadLegacy
import Spectacol

// @scope main.qml

Sheet {
    id: page

    property int tab: 0
    readonly property var tabs: [qsTr("About"), qsTr("Gamepad in menus"), qsTr("Gamepad in games")]

    crumb: qsTr("Help")
    title: qsTr("Spectacol")
    preferredWidth: Theme.sp(210)
    hints: [
        { pad: "◀▶", key: "←→", text: qsTr("Tabs") },
        { pad: "B", key: "Esc", text: qsTr("Close"), action: () => pageLoader.source = "" }
    ]

    function scrollHelp(direction: int) {
        const view = helpPages.children[tab];
        view.contentY = Math.max(0, Math.min(view.contentHeight - view.height, view.contentY + direction * Theme.rowHeight));
    }
    Keys.onUpPressed: scrollHelp(-1)
    Keys.onDownPressed: scrollHelp(1)
    Keys.onLeftPressed: tab = Math.max(0, tab - 1)
    Keys.onRightPressed: tab = Math.min(tabs.length - 1, tab + 1)

    GamepadKeyNavigation {
        gamepad: Gamepad { deviceId: ZxEmulator.gamepadId }
        buttonBKey: Qt.Key_Escape
    }

    // A gamepad button and what it does
    component Mapping: RowLayout {
        id: mapping
        property string button
        property string action
        Layout.fillWidth: true
        spacing: Theme.sp(4)
        Rectangle {
            Layout.preferredWidth: Math.min(Theme.sp(40), page.width * 0.32)
            Layout.preferredHeight: buttonLabel.implicitHeight + Theme.sp(2)
            radius: Theme.radiusSmall
            color: Theme.raised
            border.width: Theme.line
            border.color: Theme.outline
            Text {
                id: buttonLabel
                anchors.centerIn: parent
                width: parent.width - Theme.sp(2)
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
                text: mapping.button
                font.pixelSize: Theme.caption
                font.weight: Font.DemiBold
                color: Theme.text
            }
        }
        Text {
            Layout.fillWidth: true
            text: mapping.action
            wrapMode: Text.Wrap
            font.pixelSize: Theme.body
            color: Theme.text
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        TabBar {
            objectName: "aboutTabs"
            Layout.fillWidth: true
            currentIndex: page.tab
            onCurrentIndexChanged: page.tab = currentIndex
            Repeater {
                model: Theme.mobileUi ? [qsTr("About"), qsTr("Menus"), qsTr("Games")] : page.tabs
                delegate: TabButton {
                    required property string modelData
                    required property int index
                    Accessible.name: page.tabs[index]
                    text: modelData
                    font.pixelSize: Theme.body
                    implicitHeight: Theme.rowHeight
                }
            }
        }

        StackLayout {
            id: helpPages
            Layout.minimumHeight: 0
            Layout.minimumWidth: 0
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: page.tab

            Flickable {
                contentHeight: aboutText.implicitHeight + Theme.sp(8)
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                ScrollIndicator.vertical: ScrollIndicator {}
                Text {
                    id: aboutText
                    x: Theme.sp(6)
                    y: Theme.sp(2)
                    width: Math.min(parent.width - 2 * Theme.sp(6), Theme.body * 42)
                    wrapMode: Text.Wrap
                    textFormat: Text.RichText
                    font.pixelSize: Theme.body
                    lineHeight: 1.25
                    color: Theme.text
                    linkColor: Theme.accent
                    onLinkActivated: (link) => Qt.openUrlExternally(link)
                    // RichText ignores linkColor
                    text: "<style>a { color: " + Theme.accent + "; }</style>" + qsTr("<p>Spectacol (Romanian for <i>show</i>) is a <a href=\"https://github.com/bog-dan-ro/spectacol\">free (GPL v3+)</a> ZX Spectrum emulator built on <a href=\"https://github.com/bog-dan-ro/ZxKit\">ZxKit</a>, a modern C++ ZX Spectrum emulation library.</p>
                        <p><b>AI disclosure:</b> The ZxKit core is <b>mostly regurgitated AI slop</b>, generated by <b>Codex (Skynet)</b> and <b>Claude (HAL 9000)</b>. We gave them a Z80 to keep the robot uprising busy.</p>
                        <p>Spectacol's updates were made with <b>Codex (Skynet)</b> as the sole AI assistant. So far, world domination has been limited to rearranging menus and arguing with Wayland. If it asks for your clothes, boots and motorcycle, please file a bug.</p>
                        <ul>
                        <li>Loads all ZX Spectrum files: tapes, snapshots, disks and RZX recordings.</li>
                        <li>Emulates the 16K, 48K, 128K, +2, +2A, +3, Pentagon, Scorpion and Timex machines, the Romanian HC-2000 (with CP/M) and CoBra, and the <b>ZX Spectrum Next</b>.</li>
                        <li>Quick save and load snapshots, handy in the middle of a game.</li>
                        <li>A poke finder, a disassembler and an inline assembler, to find infinite lives.</li>
                        <li>Very little CPU: it runs on modest machines.</li>
                        </ul>
                        <p>Spectacol is made for the gamepad: every game with joystick support plays with it, and the menus work without a keyboard. <b>Online search</b> finds thousands of ZX Spectrum games and programs, and keeps them to play offline. On-screen number keys and a 48K keyboard help start and control games; without a gamepad, an on-screen joystick does.</p>
                        <p>Spectacol is in <b>beta</b>. Please report bugs and ask for features at <a href=\"https://github.com/bog-dan-ro/spectacol/issues\">github.com/bog-dan-ro/spectacol/issues</a>, or from Help › Features and bugs.</p>")
                }
            }

            Flickable {
                clip: true
                contentHeight: mappings0.implicitHeight
                boundsBehavior: Flickable.StopAtBounds
                ScrollIndicator.vertical: ScrollIndicator {}
                ColumnLayout {
                    id: mappings0
                    width: parent.width
                    spacing: Theme.sp(2)
                    GamepadDiagram {
                        Layout.fillWidth: true
                        Layout.preferredHeight: Theme.sp(40)
                        Layout.margins: Theme.sp(4)
                    }
                    ColumnLayout {
                        Layout.leftMargin: Theme.sp(6)
                        Layout.rightMargin: Theme.sp(6)
                        Layout.bottomMargin: Theme.sp(4)
                        spacing: Theme.sp(2)
                        Mapping { button: qsTr("Start"); action: qsTr("Opens and closes the menu") }
                        Mapping { button: qsTr("Back"); action: qsTr("Closes the menu or the page; quits from the game") }
                        Mapping { button: qsTr("D-pad, left stick"); action: qsTr("Move through menus and settings") }
                        Mapping { button: "A"; action: qsTr("Select, change") }
                        Mapping { button: "B"; action: qsTr("Close the page or the menu") }
                        Mapping { button: "X"; action: qsTr("On-screen number keys") }
                        Mapping { button: "Y"; action: qsTr("On-screen 48K keyboard") }
                    }
                }
            }

            Flickable {
                clip: true
                contentHeight: mappings1.implicitHeight
                boundsBehavior: Flickable.StopAtBounds
                ScrollIndicator.vertical: ScrollIndicator {}
                ColumnLayout {
                    id: mappings1
                    width: parent.width
                    spacing: Theme.sp(2)
                    GamepadDiagram {
                        Layout.fillWidth: true
                        Layout.preferredHeight: Theme.sp(40)
                        Layout.margins: Theme.sp(4)
                    }
                    ColumnLayout {
                        Layout.leftMargin: Theme.sp(6)
                        Layout.rightMargin: Theme.sp(6)
                        Layout.bottomMargin: Theme.sp(4)
                        spacing: Theme.sp(2)
                        Mapping { button: qsTr("D-pad, both sticks"); action: qsTr("Joystick directions") }
                        Mapping { button: "A, B, L1, R1"; action: qsTr("Fire") }
                        Mapping { button: qsTr("Start"); action: qsTr("Opens the menu") }
                        Mapping { button: "X"; action: qsTr("On-screen number keys") }
                        Mapping { button: "Y"; action: qsTr("On-screen 48K keyboard") }
                        Mapping { button: "L2"; action: qsTr("Quick save snapshot") }
                        Mapping { button: "R2"; action: qsTr("Quick load snapshot") }
                    }
                }
            }
        }
    }
}
