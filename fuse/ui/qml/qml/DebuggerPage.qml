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
                width: 50 * Screen.pixelDensity
                Layout.fillWidth: false
                Layout.fillHeight: true
                Rectangle {
                    id: registersRect
                    color: backgroudColor
                    radius: Screen.pixelDensity
                    border.width: (focus ? 1 : 0.5) * Screen.pixelDensity
                    border.color: "white"
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    activeFocusOnTab: true
                    FancyText {
                        anchors.centerIn: parent
                        text: "registersRect"
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
