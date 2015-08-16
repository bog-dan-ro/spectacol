// @scope main.qml

import QtQuick 2.5
import QtQuick.Layouts 1.1
import QtQuick.Window 2.0

Item {
    readonly property color backgroudColor : Qt.rgba(0, 0, 0, 0.85);
    ColumnLayout {
        spacing: 2.5 * Screen.pixelDensity
        anchors.fill: parent
        RowLayout {
            spacing: 2.5 * Screen.pixelDensity
            Layout.fillWidth: true
            Layout.fillHeight: true
            Rectangle {
                id: dissasambleRect
                color: backgroudColor
                radius: Screen.pixelDensity
                border.width: (focus ? 1 : 0.5) * Screen.pixelDensity
                border.color: "white"
                activeFocusOnTab: true
                focus: true

                width: 50 * Screen.pixelDensity
                Layout.fillHeight: true

                FancyText {
                    anchors.centerIn: parent
                    text: "Disassemble"
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: dissasambleRect.focus = true
                }
            }
            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Rectangle {
                    id: infoRect
                    visible: false
                    radius: Screen.pixelDensity
                    border.width: (focus ? 1 : 0.5) * Screen.pixelDensity
                    border.color: "white"

                    FancyText {
                        anchors.centerIn: parent
                        text: "info"
                    }
                }
            }
            ColumnLayout {
                spacing: 2.5 * Screen.pixelDensity
                width: 60 * Screen.pixelDensity
                Layout.fillWidth: false
                Layout.fillHeight: true
                Rectangle {
                    id: registersRect
                    color: backgroudColor
                    radius: Screen.pixelDensity
                    border.width: (focus ? 1 : 0.5) * Screen.pixelDensity
                    border.color: "white"
                    height: 60 * Screen.pixelDensity
                    Layout.fillWidth: true
                    activeFocusOnTab: true
                    GridLayout {
                        anchors.fill: parent
                        anchors.topMargin: parent.border.width + Screen.pixelDensity
                        anchors.leftMargin: parent.border.width + Screen.pixelDensity
                        anchors.rightMargin: parent.border.width + Screen.pixelDensity
                        columnSpacing: Screen.pixelDensity
                        columns: 4
                        FancyText {
                            fontSize: 4
                            horizontalAlignment: Text.AlignRight
                            Layout.fillHeight: false
                            Layout.fillWidth: true
                            text: "PC"
                        }
                        FancyText {
                            fontSize: 4
                            Layout.fillHeight: false
                            Layout.fillWidth: false
                            font.family: "Monospace"
                            text: fuse.PC
                        }
                        FancyText {
                            fontSize: 4
                            horizontalAlignment: Text.AlignRight
                            Layout.fillHeight: false
                            Layout.fillWidth: true
                            text: "SP"
                        }
                        FancyText {
                            fontSize: 4
                            Layout.fillHeight: false
                            Layout.fillWidth: false
                            font.family: "Monospace"
                            text: fuse.SP
                        }

                        FancyText {
                            fontSize: 4
                            horizontalAlignment: Text.AlignRight
                            Layout.fillHeight: false
                            Layout.fillWidth: true
                            text: "AF"
                        }
                        FancyText {
                            fontSize: 4
                            Layout.fillHeight: false
                            Layout.fillWidth: false
                            font.family: "Monospace"
                            text: fuse.AF
                        }
                        FancyText {
                            fontSize: 4
                            horizontalAlignment: Text.AlignRight
                            Layout.fillHeight: false
                            Layout.fillWidth: true
                            text: "AF'"
                        }
                        FancyText {
                            fontSize: 4
                            Layout.fillHeight: false
                            Layout.fillWidth: false
                            font.family: "Monospace"
                            text: fuse.AF_
                        }

                        FancyText {
                            fontSize: 4
                            horizontalAlignment: Text.AlignRight
                            Layout.fillHeight: false
                            Layout.fillWidth: true
                            text: "BC"
                        }
                        FancyText {
                            fontSize: 4
                            Layout.fillHeight: false
                            Layout.fillWidth: false
                            font.family: "Monospace"
                            text: fuse.BC
                        }
                        FancyText {
                            fontSize: 4
                            horizontalAlignment: Text.AlignRight
                            Layout.fillHeight: false
                            Layout.fillWidth: true
                            text: "BC'"
                        }
                        FancyText {
                            fontSize: 4
                            Layout.fillHeight: false
                            Layout.fillWidth: false
                            font.family: "Monospace"
                            text: fuse.BC_
                        }

                        FancyText {
                            fontSize: 4
                            horizontalAlignment: Text.AlignRight
                            Layout.fillHeight: false
                            Layout.fillWidth: true
                            text: "DE"
                        }
                        FancyText {
                            fontSize: 4
                            Layout.fillHeight: false
                            Layout.fillWidth: false
                            font.family: "Monospace"
                            text: fuse.DE
                        }
                        FancyText {
                            fontSize: 4
                            horizontalAlignment: Text.AlignRight
                            Layout.fillHeight: false
                            Layout.fillWidth: true
                            text: "DE'"
                        }
                        FancyText {
                            fontSize: 4
                            Layout.fillHeight: false
                            Layout.fillWidth: false
                            font.family: "Monospace"
                            text: fuse.DE_
                        }

                        FancyText {
                            fontSize: 4
                            horizontalAlignment: Text.AlignRight
                            Layout.fillHeight: false
                            Layout.fillWidth: true
                            text: "HL"
                        }
                        FancyText {
                            fontSize: 4
                            Layout.fillHeight: false
                            Layout.fillWidth: false
                            font.family: "Monospace"
                            text: fuse.HL
                        }
                        FancyText {
                            fontSize: 4
                            horizontalAlignment: Text.AlignRight
                            Layout.fillHeight: false
                            Layout.fillWidth: true
                            text: "HL'"
                        }
                        FancyText {
                            fontSize: 4
                            Layout.fillHeight: false
                            Layout.fillWidth: false
                            font.family: "Monospace"
                            text: fuse.HL_
                        }

                        FancyText {
                            fontSize: 4
                            horizontalAlignment: Text.AlignRight
                            Layout.fillHeight: false
                            Layout.fillWidth: true
                            text: "IX"
                        }
                        FancyText {
                            fontSize: 4
                            Layout.fillHeight: false
                            Layout.fillWidth: false
                            font.family: "Monospace"
                            text: fuse.IX
                        }
                        FancyText {
                            fontSize: 4
                            horizontalAlignment: Text.AlignRight
                            Layout.fillHeight: false
                            Layout.fillWidth: true
                            text: "IY"
                        }
                        FancyText {
                            fontSize: 4
                            Layout.fillHeight: false
                            Layout.fillWidth: false
                            font.family: "Monospace"
                            text: fuse.IY
                        }

                        FancyText {
                            fontSize: 4
                            horizontalAlignment: Text.AlignRight
                            Layout.fillHeight: false
                            Layout.fillWidth: true
                            text: "I"
                        }
                        FancyText {
                            fontSize: 4
                            Layout.fillHeight: false
                            Layout.fillWidth: false
                            font.family: "Monospace"
                            text: fuse.I
                        }
                        FancyText {
                            fontSize: 4
                            horizontalAlignment: Text.AlignRight
                            Layout.fillHeight: false
                            Layout.fillWidth: true
                            text: "R"
                        }
                        FancyText {
                            fontSize: 4
                            Layout.fillHeight: false
                            Layout.fillWidth: false
                            font.family: "Monospace"
                            text: fuse.R
                        }

                        FancyText {
                            fontSize: 4
                            horizontalAlignment: Text.AlignRight
                            Layout.fillHeight: false
                            Layout.fillWidth: true
                            text: "IFF1"
                        }
                        FancyText {
                            fontSize: 4
                            Layout.fillHeight: false
                            Layout.fillWidth: false
                            font.family: "Monospace"
                            text: fuse.IFF1
                        }
                        FancyText {
                            fontSize: 4
                            horizontalAlignment: Text.AlignRight
                            Layout.fillHeight: false
                            Layout.fillWidth: true
                            text: "IFF2"
                        }
                        FancyText {
                            fontSize: 4
                            Layout.fillHeight: false
                            Layout.fillWidth: false
                            font.family: "Monospace"
                            text: fuse.IFF2
                        }

                        FancyText {
                            fontSize: 4
                            horizontalAlignment: Text.AlignRight
                            Layout.fillHeight: false
                            Layout.fillWidth: true
                            text: "R7"
                        }
                        FancyText {
                            fontSize: 4
                            Layout.fillHeight: false
                            Layout.fillWidth: false
                            font.family: "Monospace"
                            text: fuse.R7
                        }
                        FancyText {
                            fontSize: 4
                            horizontalAlignment: Text.AlignRight
                            Layout.fillHeight: false
                            Layout.fillWidth: true
                            text: "IM"
                        }
                        FancyText {
                            fontSize: 4
                            Layout.fillHeight: false
                            Layout.fillWidth: false
                            font.family: "Monospace"
                            text: fuse.IM
                        }

                        Item {
                            Layout.fillHeight: true
                            Layout.fillWidth: true
                        }
                    }
                }
                Rectangle {
                    id: breakPointsRect
                    color: backgroudColor
                    radius: Screen.pixelDensity
                    border.width: (focus ? 1 : 0.5) * Screen.pixelDensity
                    border.color: "white"
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    activeFocusOnTab: true
                    FancyText {
                        anchors.centerIn: parent
                        text: "breakPoints"
                    }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: dissasambleRect.focus = true
                    }
                }
            }
        }
        Rectangle {
            id: toolBarRect
            height: 10 * Screen.pixelDensity
            Layout.fillWidth: true
            color: backgroudColor
            radius: Screen.pixelDensity
            border.width: (focus ? 1 : 0.5) * Screen.pixelDensity
            border.color: "white"
            FancyText {
                anchors.centerIn: parent
                text: "toolBar"
            }
        }
    }
}
