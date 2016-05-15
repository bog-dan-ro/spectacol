/*
    Copyright (c) 2016, BogDan Vatra <bogdan@kde.org>

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

import QtQuick 2.6
import QtQuick.Window 2.2
import QtGamepad 1.0
import QtQuick.Controls 2.0
import "private"

// @scope main.qml

Flickable {
    contentHeight: pane.height
    Component.onCompleted: fuse.paused = true
    Component.onDestruction: fuse.paused = false
    anchors.margins: Screen.pixelDensity

    GamepadKeyNavigation {
        gamepad: Gamepad { deviceId: fuse.gamepadId }
        buttonAKey: Qt.Key_Space
        buttonBKey: Qt.Key_Escape
        buttonYKey: Qt.Key_Tab
        buttonXKey: Qt.Key_Backtab
    }

    Pane {
        id: pane
        focus: true
        background.opacity: 0.85
        PropertyAnimation on x {
            to: (pageLoader.width - pane.width) / 2
            easing.type: Easing.InOutBack
            easing.amplitude: 2.0
            easing.period: 1.5
            duration: 500
        }
        Column {
            spacing: 2.5 * Screen.pixelDensity
            anchors.horizontalCenter: parent.horizontalCenter
            Label {
                text: qsTr("Screen Options")
            }

            Row {
                spacing: 2.5 * Screen.pixelDensity
                Label {
                    text: qsTr("Screen filter")
                    anchors.verticalCenter: parent.verticalCenter
                }
                ComboBox {
                    focus: true
                    id: screenFilter
                    Keys.onUpPressed: {
                        if (popup.visible)
                            decrease();
                        else
                            smoothScaling.forceActiveFocus(Qt.TabFocusReason);
                    }
                    Keys.onDownPressed: {
                        if (popup.visible)
                            increase();
                        else
                            screenFillMode.forceActiveFocus(Qt.TabFocusReason);
                    }
                    Keys.onLeftPressed: if (!popup.visible) decrease();
                    Keys.onRightPressed: if (!popup.visible) increase();

                    model: fuse.filtersModel
                    currentIndex: fuse.selectedFilterIndex
                    onCurrentIndexChanged: fuse.selectedFilterIndex = currentIndex
                }
            }

            Row {
                spacing: 2.5 * Screen.pixelDensity
                Label {
                    text: qsTr("Fill mode")
                    anchors.verticalCenter: parent.verticalCenter
                }
                ComboBox {
                    id: screenFillMode
                    Keys.onUpPressed: {
                        if (popup.visible)
                            decrease();
                        else
                            screenFilter.forceActiveFocus(Qt.TabFocusReason);
                    }
                    Keys.onDownPressed: {
                        if (popup.visible) {
                            increase();
                        } else if (fuseSettings.showOrientationChooser) {
                            screenOrientation.forceActiveFocus(Qt.TabFocusReason);
                        } else {
                            smoothScaling.forceActiveFocus(Qt.TabFocusReason);
                        }
                    }
                    Keys.onLeftPressed: if (!popup.visible) decrease();
                    Keys.onRightPressed: if (!popup.visible) increase();

                    model: ["Preserve aspect fit", "Preserve aspect", "Stretch"]
                    currentIndex: fuseSettings.fillMode
                    onCurrentIndexChanged: {
                        fuseSettings.fillMode = currentIndex;
                        fuseScreen.updateFillMode();
                    }
                }
            }

            Row {
                visible: fuseSettings.showOrientationChooser
                spacing: 2.5 * Screen.pixelDensity
                Label {
                    text: qsTr("Screen orientation")
                    anchors.verticalCenter: parent.verticalCenter
                }
                ComboBox {
                    id: screenOrientation
                    Keys.onUpPressed: {
                        if (popup.visible)
                            decrease();
                        else
                            screenFillMode.forceActiveFocus(Qt.TabFocusReason);
                    }
                    Keys.onDownPressed: {
                        if (popup.visible)
                            increase();
                        else
                            screenFilter.forceActiveFocus(Qt.TabFocusReason);
                    }
                    Keys.onLeftPressed: if (!popup.visible) decrease()
                    Keys.onRightPressed: if (!popup.visible) increase()

                    model: ["Any", "Landscape", "Portrait"]
                    currentIndex: fuseSettings.screenOrientation
                    onCurrentIndexChanged: fuseSettings.screenOrientation = currentIndex
                }
            }

            CheckBox {
                id: smoothScaling
                KeyNavigation.up: fuseSettings.showOrientationChooser ? screenOrientation : screenFillMode
                KeyNavigation.down: TextSizes.smallScreen ? leftBorder : screenFilter
                text: qsTr("Smooth scaling")
                checked: fuseScreen.smoothScaling
                onCheckedChanged: fuseScreen.smoothScaling = checked
            }

            CheckBox {
                id: leftBorder
                visible: TextSizes.smallScreen
                KeyNavigation.up: smoothScaling
                KeyNavigation.down: screenFilter
                text: qsTr("Left boder")
                checked: fuseScreen.leftBorder
                onCheckedChanged: fuseScreen.leftBorder = checked
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Close (B)")
                onClicked: pageLoader.source = ""
            }
        }
    }
}
