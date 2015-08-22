// @scope main.qml

import QtQuick 2.5
import QtQuick.Layouts 1.1
import QtQuick.Window 2.0

Item {
    Component.onCompleted: fuse.disassamble();
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

                ListView {
                    id: view
                    clip: true
                    model: fuse.disassambleModel()
                    delegate: Rectangle {
                        width: view.width
                        height: 7 * Screen.pixelDensity
                        color: "black"
                        Text {
                            text: qsTr("text");
                            color: "white"
                        }
                    }
                }
//                DisassembleView {
//                    id: disassembleView
//                    anchors.fill: parent
//                }
            }
            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Rectangle {
                    id: infoRect
                    visible: true
                    anchors.fill: parent
                    radius: Screen.pixelDensity
                    color: backgroudColor
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
                    RegistersView {
                        anchors.fill: parent
                        anchors.topMargin: parent.border.width + Screen.pixelDensity
                        anchors.leftMargin: parent.border.width + Screen.pixelDensity
                        anchors.rightMargin: parent.border.width + Screen.pixelDensity
                        columnSpacing: Screen.pixelDensity
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
