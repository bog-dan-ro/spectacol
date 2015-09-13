/*
    Copyright (c) 2015, BogDan Vatra <bogdan@kde.org>

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

import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.1
import QtQml.Models 2.2
import QtQuick.Window 2.0
import Fuse 1.0

Rectangle {
    id: pokeFinderPage
    color: Qt.rgba(0, 0, 0, 0.75);

    Component.onCompleted: {
        fuse.pokeFinderResetIfNeeded();
        fuse.paused = true;
    }
    Component.onDestruction: fuse.paused = false

    function addBreakpoint(type)
    {
        var model = visualModel.items.get(view.currentIndex).model;
        fuse.addBreakpointPage(model.offset, model.page, type);
    }

    Keys.onPressed: {
        switch (event.key) {
        case Qt.Key_C:
            pageLoader.source = "";
            event.accepted = true;
            break;

        case Qt.Key_I:
            fuse.pokeFinderInced();
            event.accepted = true;
            break;

        case Qt.Key_D:
            fuse.pokeFinderDeced();
            event.accepted = true;
            break;

        case Qt.Key_R:
            fuse.pokeFinderReset();
            event.accepted = true;
            break;

        case Qt.Key_B:
            addBreakpoint(BreakpointsModel.BreakOnWrite);
            event.accepted = true;
            break;

        case Qt.Key_D:
            addBreakpoint(BreakpointsModel.BreakOnRead);
            event.accepted = true;
            break;
        }
    }

    ColumnLayout {
        anchors.centerIn: parent
        RowLayout {
            Layout.fillWidth: true
            FancyText {
                Layout.alignment: Qt.AlignRight
                text: qsTr("Search for")
            }

            FancyTextField {
                Layout.fillWidth: true
                id: value
                focus: true
                activeFocusOnTab: true
                implicitWidth: pokeFinderPage.width / 2
                validator: IntValidator {bottom: 0; top: 255;}
                placeholderText: qsTr("Enter a numeric value")
                Keys.onPressed: {
                    switch (event.key) {
                    case Qt.Key_C:
                        pageLoader.source = "";
                        event.accepted = true;
                        break;

                    case Qt.Key_I:
                        fuse.pokeFinderInced();
                        event.accepted = true;
                        break;

                    case Qt.Key_D:
                        fuse.pokeFinderDeced();
                        event.accepted = true;
                        break;

                    case Qt.Key_R:
                        fuse.pokeFinderReset();
                        event.accepted = true;
                        break;

                    case Qt.Key_B:
                        addBreakpoint(BreakpointsModel.BreakOnWrite);
                        event.accepted = true;
                        break;

                    case Qt.Key_D:
                        addBreakpoint(BreakpointsModel.BreakOnRead);
                        event.accepted = true;
                        break;

                    case Qt.Key_S:
                    case Qt.Key_Enter:
                    case Qt.Key_Return:
                        fuse.pokeFinderSearch(value.text);
                        event.accepted = true;
                        break;
                    }
                }
            }
        }
        RowLayout {
            Layout.fillWidth: true
            ColumnLayout {
                Button {
                    Layout.fillWidth: true
                    activeFocusOnTab: false
                    text: qsTr("&Incremented")
                    onClicked: fuse.pokeFinderInced()
                }
                Button {
                    Layout.fillWidth: true
                    activeFocusOnTab: false
                    text: qsTr("&Decremented")
                    onClicked: fuse.pokeFinderDeced()
                }
                Button {
                    Layout.fillWidth: true
                    activeFocusOnTab: false
                    text: qsTr("&Search")
                    onClicked: fuse.pokeFinderSearch(value.text)
                }
                Button {
                    Layout.fillWidth: true
                    activeFocusOnTab: false
                    text: qsTr("&Reset")
                    onClicked: fuse.pokeFinderReset()
                }
                Button {
                    Layout.fillWidth: true
                    enabled: view.currentIndex !== -1
                    activeFocusOnTab: false
                    text: qsTr("&Break on write")
                    onClicked: addBreakpoint(BreakpointsModel.BreakOnWrite);
                }
                Button {
                    Layout.fillWidth: true
                    activeFocusOnTab: false
                    enabled: view.currentIndex !== -1
                    text: qsTr("Break on rea&d")
                    onClicked: addBreakpoint(BreakpointsModel.BreakOnRead);
                }
                Button {
                    Layout.fillWidth: true
                    activeFocusOnTab: false
                    text: qsTr("&Close")
                    onClicked: pageLoader.source = "";
                }
            }
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                FancyText {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignRight
                    text: qsTr("Possible locations: ") + fuse.pokeFinderCount
                }

                VisualDataModel {
                    id: visualModel
                    model: pokeFinderModel
                    delegate: Rectangle {
                        property color paper: view.currentIndex !== index ? Qt.rgba(0, 0, 0, 0.9) : "white"
                        property color ink: view.currentIndex !== index ? "white": Qt.rgba(0, 0, 0, 0.9)

                        width: view.width
                        height: 7 * Screen.pixelDensity
                        color: paper
                        RowLayout {
                            anchors.fill: parent
                            FancyText {
                                Layout.fillHeight: true
                                Layout.fillWidth: false
                                Layout.preferredWidth: 10 * Screen.pixelDensity
                                horizontalAlignment: Text.AlignRight
                                fontSize: 4
                                style: Text.Normal
                                color: ink
                                text: model.page
                            }

                            Item { Layout.fillWidth: false; width: 1.5 * Screen.pixelDensity }

                            FancyText {
                                Layout.fillHeight: true
                                Layout.fillWidth: false
                                Layout.preferredWidth: 15 * Screen.pixelDensity
                                horizontalAlignment: Text.AlignHCenter
                                fontSize: 4
                                style: Text.Normal
                                color: ink
                                text: model.offsetText
                            }

                            Item { Layout.fillWidth: false; width: 1.5 * Screen.pixelDensity }

                            FancyText {
                                Layout.fillHeight: true
                                Layout.fillWidth: false
                                horizontalAlignment: Text.AlignLeft
                                fontSize: 4
                                color: ink
                                style: Text.Normal
                                text: model.value
                            }

                            Item { Layout.fillWidth: false; width: 1.5 * Screen.pixelDensity }

                            FancyText {
                                Layout.fillHeight: true
                                Layout.fillWidth: true
                                horizontalAlignment: Text.AlignLeft
                                fontSize: 4
                                color: ink
                                style: Text.Normal
                                text: model.breakpoint
                            }
                        }
                        MouseArea {
                            anchors.fill: parent
                            onClicked: {view.focus = true; view.currentIndex = model.index}
                        }
                    }
                }
                ListView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    id: view
                    clip: true
                    snapMode: ListView.SnapToItem
                    highlightFollowsCurrentItem: true
                    focus: true
                    activeFocusOnTab: fuse.pokeFinderCount <= 20

                    Keys.onUpPressed: decrementCurrentIndex();
                    Keys.onDownPressed: incrementCurrentIndex()

                    model: visualModel

                    currentIndex: fuse.pokeFinderCount <= 20 ? 0 : -1

                    header: Rectangle {
                        height: 5 * Screen.pixelDensity
                        RowLayout {
                            anchors.fill: parent
                            FancyText {
                                Layout.fillHeight: true
                                Layout.fillWidth: false
                                Layout.preferredWidth: 10 * Screen.pixelDensity
                                Layout.alignment: Qt.AlignRight
                                fontSize: 4
                                style: Text.Normal
                                text: "Bank"
                            }

                            Item { Layout.fillWidth: false; width: 1.5 * Screen.pixelDensity }

                            FancyText {
                                Layout.fillHeight: true
                                Layout.fillWidth: false
                                Layout.preferredWidth: 15 * Screen.pixelDensity
                                Layout.alignment: Qt.AlignLeft
                                fontSize: 4
                                style: Text.Normal
                                text: "Offset"
                            }

                            Item { Layout.fillWidth: false; width: 1.5 * Screen.pixelDensity }

                            FancyText {
                                Layout.fillHeight: true
                                Layout.fillWidth: false
                                horizontalAlignment: Text.AlignHCenter
                                fontSize: 4
                                style: Text.Normal
                                text: "Value"
                            }

                            Item { Layout.fillWidth: false; width: 1.5 * Screen.pixelDensity }

                            FancyText {
                                Layout.fillHeight: true
                                Layout.fillWidth: false
                                horizontalAlignment: Text.AlignRight
                                fontSize: 4
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
