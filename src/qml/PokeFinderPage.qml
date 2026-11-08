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

import QtQuick
import QtQml.Models
import QtQuick.Layouts
import QtQuick.Window
import QtQuick.Controls
import QtGamepadLegacy
import Spectacol
import "private"

Rectangle {
    id: pokeFinderPage
    color: Theme.surface

    Component.onCompleted: {
        ZxEmulator.processInputEvents = false;
        ZxEmulator.pokeFinderResetIfNeeded();
    }
    Component.onDestruction: {
        ZxEmulator.processInputEvents = true;
    }

    readonly property bool hasCurrentLocation: view.currentIndex >= 0 && view.currentIndex < view.count

    function addBreakpoint(type)
    {
        if (!hasCurrentLocation)
            return;
        const model = visualModel.items.get(view.currentIndex).model;
        ZxEmulator.addBreakpointPage(model.offset, model.page, type);
    }

    // The locations are listed only when there are at most 20 of them.
    function resetCurrentLocation()
    {
        view.currentIndex = ZxEmulator.pokeFinderCount <= 20 ? 0 : -1;
    }

    Connections {
        target: ZxEmulator
        function onPokeFinderCountChanged() { resetCurrentLocation(); }
    }

    GamepadKeyNavigation {
        gamepad: Gamepad { deviceId: ZxEmulator.gamepadId }
        buttonAKey: Qt.Key_Return
        buttonBKey: Qt.Key_B
        buttonYKey: Qt.Key_Y
        buttonXKey: Qt.Key_X
        buttonL1Key: Qt.Key_W
        buttonR1Key: Qt.Key_R
        buttonL2Key: Qt.Key_E
        buttonR2Key: Qt.Key_E
    }

    Keys.onPressed: (event) => {
        switch (event.key) {
        case Qt.Key_Escape:
        case Qt.Key_B:
            pageLoader.source = "";
            event.accepted = true;
            break;

        case Qt.Key_Y:
            ZxEmulator.pokeFinderInced();
            event.accepted = true;
            break;

        case Qt.Key_X:
            ZxEmulator.pokeFinderDeced();
            event.accepted = true;
            break;

        case Qt.Key_E:
            ZxEmulator.pokeFinderReset();
            event.accepted = true;
            break;

        case Qt.Key_W:
            addBreakpoint(BreakpointsModel.BreakOnWrite);
            event.accepted = true;
            break;

        case Qt.Key_R:
            addBreakpoint(BreakpointsModel.BreakOnRead);
            event.accepted = true;
            break;
        }
    }

    ColumnLayout {
        anchors.centerIn: parent
        width: TextSizes.scale12 * 40
        Component.onCompleted: {
            if (TextSizes.smallScreen)
                spacing = 0;
        }
        RowLayout {
            Layout.fillWidth: true
            FancyText {
                Layout.alignment: Qt.AlignRight
                text: qsTr("Search for")
            }

            FancyTextField {
                mono: true
                id: value
                KeyNavigation.up: view
                KeyNavigation.down: view
                Layout.fillWidth: true
                focus: true
                implicitWidth: pokeFinderPage.width / 2
                inputMethodHints: Qt.ImhDigitsOnly
                validator: IntValidator {bottom: 0; top: 255;}
                placeholderText: qsTr("Enter a numeric value")
                onAccepted: {
                    ZxEmulator.pokeFinderSearch(value.text);
                    view.forceActiveFocus(Qt.TabFocusReason);
                }
            }
        }
        RowLayout {
            Layout.fillWidth: true
            ColumnLayout {
                Component.onCompleted: {
                    if (TextSizes.smallScreen)
                        spacing = 0;
                }
                Button {
                    id: buttonInc
                    Layout.fillWidth: true
                    text: Theme.gamepadHints ? qsTr("Incremented <b>(Y)</b>") : qsTr("Incremented")
                    onClicked: ZxEmulator.pokeFinderInced()
                }
                Button {
                    id: buttonDec
                    Layout.fillWidth: true
                    text: Theme.gamepadHints ? qsTr("Decremented <b>(X)</b>") : qsTr("Decremented")
                    onClicked: ZxEmulator.pokeFinderDeced()
                }
                Button {
                    id: buttonSearch
                    Layout.fillWidth: true
                    text: qsTr("Search <b>(A)</b>")
                    onClicked: ZxEmulator.pokeFinderSearch(value.text)
                }
                Button {
                    id: buttonReset
                    Layout.fillWidth: true
                    text: qsTr("Reset <b>(L2, R2)</b>")
                    onClicked: ZxEmulator.pokeFinderReset()
                }
                Button {
                    id: buttonBW
                    Layout.fillWidth: true
                    enabled: hasCurrentLocation
                    text: qsTr("Break on write <b>(L1)</b>")
                    onClicked: addBreakpoint(BreakpointsModel.BreakOnWrite);
                }
                Button {
                    id: buttonBR
                    Layout.fillWidth: true
                    enabled: hasCurrentLocation
                    text: qsTr("Break on read <b>(R1)</b>")
                    onClicked: addBreakpoint(BreakpointsModel.BreakOnRead);
                }
                Button {
                    id: buttonClose
                    Layout.fillWidth: true
                    text: Theme.gamepadHints ? qsTr("Close <b>(B)</b>") : qsTr("Close")
                    onClicked: pageLoader.source = "";
                }
            }
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                FancyText {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignRight
                    text: qsTr("Possible locations: ") + ZxEmulator.pokeFinderCount
                }

                DelegateModel {
                    id: visualModel
                    model: ZxEmulator.pokeFinderModel
                    delegate: Rectangle {
                        property color paper: view.currentIndex !== index ? "transparent" : Theme.accentFill
                        property color ink: Theme.text

                        width: view.width
                        height: TextSizes.scale12 * 2
                        color: paper
                        RowLayout {
                            anchors.fill: parent
                            FancyText {
                                Layout.fillHeight: true
                                Layout.fillWidth: false
                                Layout.preferredWidth: 5 * TextSizes.scale12
                                horizontalAlignment: Text.AlignRight
                                font.pixelSize: TextSizes.scale12
                                style: Text.Normal
                                color: ink
                                text: model.page
                            }

                            Item { Layout.fillWidth: false; Layout.preferredWidth: TextSizes.smallScreen ? 0 : 1.5 * Screen.pixelDensity }

                            FancyText {
                                Layout.fillHeight: true
                                Layout.fillWidth: false
                                Layout.preferredWidth: 6 * TextSizes.scale12
                                horizontalAlignment: Text.AlignHCenter
                                font.pixelSize: TextSizes.scale12
                                style: Text.Normal
                                color: ink
                                text: model.offsetText
                            }

                            Item { Layout.fillWidth: false; Layout.preferredWidth: TextSizes.smallScreen ? 0 : 1.5 * Screen.pixelDensity }

                            FancyText {
                                Layout.fillHeight: true
                                Layout.fillWidth: false
                                Layout.preferredWidth: 3 * TextSizes.scale12
                                horizontalAlignment: Text.AlignLeft
                                font.pixelSize: TextSizes.scale12
                                color: ink
                                style: Text.Normal
                                text: model.value
                            }

                            Item { Layout.fillWidth: false; Layout.preferredWidth: TextSizes.smallScreen ? 0 : 1.5 * Screen.pixelDensity }

                            FancyText {
                                Layout.fillHeight: true
                                Layout.fillWidth: true
                                horizontalAlignment: Text.AlignLeft
                                font.pixelSize: TextSizes.scale12
                                color: ink
                                style: Text.Normal
                                text: model.breakpoint
                            }
                        }
                        MouseArea {
                            anchors.fill: parent
                            onClicked: {view.forceActiveFocus(Qt.TabFocusReason); view.currentIndex = model.index}
                        }
                    }
                }
                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    radius: Screen.pixelDensity
                    border.width: (view.focus ? 1 : 0.5) * Screen.pixelDensity
                    border.color: Theme.accent
                    color: "transparent"
                    ListView {
                        id: view
                        anchors.fill: parent
                        anchors.margins: Screen.pixelDensity * 2
                        clip: true
                        snapMode: ListView.SnapToItem
                        highlightFollowsCurrentItem: true
                        focus: true
                        activeFocusOnTab: ZxEmulator.pokeFinderCount <= 20

                        KeyNavigation.right: value
                        KeyNavigation.left: value
                        KeyNavigation.up: value
                        KeyNavigation.down: value

                        model: visualModel

                        Component.onCompleted: resetCurrentLocation()

                        header: Item {
                            width: view.width
                            height: 2 * TextSizes.scale12
                            RowLayout {
                                anchors.fill: parent
                                FancyText {
                                    Layout.fillHeight: true
                                    Layout.fillWidth: false
                                    Layout.preferredWidth: 5 * TextSizes.scale12
                                    horizontalAlignment: Text.AlignRight
                                    font.pixelSize: TextSizes.scale12
                                    style: Text.Normal
                                    text: "Bank"
                                }

                                Item { Layout.fillWidth: false; Layout.preferredWidth: TextSizes.smallScreen ? 0 : 1.5 * Screen.pixelDensity }

                                FancyText {
                                    Layout.fillHeight: true
                                    Layout.fillWidth: false
                                    Layout.preferredWidth: 6 * TextSizes.scale12
                                    horizontalAlignment: Text.AlignHCenter
                                    font.pixelSize: TextSizes.scale12
                                    style: Text.Normal
                                    text: "Offset"
                                }

                                Item { Layout.fillWidth: false; Layout.preferredWidth: TextSizes.smallScreen ? 0 : 1.5 * Screen.pixelDensity }

                                FancyText {
                                    Layout.fillHeight: true
                                    Layout.fillWidth: false
                                    Layout.preferredWidth: 3 * TextSizes.scale12
                                    horizontalAlignment: Text.AlignLeft
                                    font.pixelSize: TextSizes.scale12
                                    style: Text.Normal
                                    text: "Value"
                                }

                                Item { Layout.fillWidth: false; Layout.preferredWidth: TextSizes.smallScreen ? 0 : 1.5 * Screen.pixelDensity }

                                FancyText {
                                    Layout.fillHeight: true
                                    Layout.fillWidth: true
                                    horizontalAlignment: Text.AlignLeft
                                    font.pixelSize: TextSizes.scale12
                                    style: Text.Normal
                                    text: "Breakpoint"
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
