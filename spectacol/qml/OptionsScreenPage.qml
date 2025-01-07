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

import QtQuick
import QtQuick.Window
import QtGamepadLegacy
import QtQuick.Controls
import QtQuick.Layouts
import Spectacol
import "private"

// @scope main.qml

Flickable {
    contentHeight: pane.height
    Component.onCompleted: FuseEmulator.paused = true
    Component.onDestruction: FuseEmulator.paused = false
    anchors.margins: Screen.pixelDensity

    GamepadKeyNavigation {
        gamepad: Gamepad { deviceId: FuseEmulator.gamepadId }
        buttonAKey: Qt.Key_Space
        buttonBKey: Qt.Key_Escape
        buttonYKey: Qt.Key_Tab
        buttonXKey: Qt.Key_Backtab
    }

    FancyPane {
        id: pane
        ColumnLayout {
            spacing: 2.5 * Screen.pixelDensity
            anchors.horizontalCenter: parent.horizontalCenter
            Label {
                text: qsTr("Screen Options")
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 2.5 * Screen.pixelDensity
                Label {
                    text: qsTr("Screen filter")
                }
                ComboBox {
                    focus: true
                    id: screenFilter
                    Layout.minimumWidth: 50 * Screen.pixelDensity
                    Layout.fillWidth: true
                    Keys.onUpPressed: {
                        if (popup.visible)
                            decrementCurrentIndex();
                        else
                            smoothScaling.forceActiveFocus(Qt.TabFocusReason);
                    }
                    Keys.onDownPressed: {
                        if (popup.visible)
                            incrementCurrentIndex();
                        else
                            screenFillMode.forceActiveFocus(Qt.TabFocusReason);
                    }
                    Keys.onLeftPressed: if (!popup.visible) decrementCurrentIndex();
                    Keys.onRightPressed: if (!popup.visible) incrementCurrentIndex();

                    model: FuseEmulator.filtersModel
                    currentIndex: FuseEmulator.selectedFilterIndex
                    onCurrentIndexChanged: FuseEmulator.selectedFilterIndex = currentIndex
                }
            }

            RowLayout {
                spacing: 2.5 * Screen.pixelDensity
                Layout.fillWidth: true
                Label {
                    text: qsTr("Fill mode")
                }
                ComboBox {
                    id: screenFillMode
                    Layout.minimumWidth: 50 * Screen.pixelDensity
                    Layout.fillWidth: true
                    Keys.onUpPressed: {
                        if (popup.visible)
                            decrementCurrentIndex();
                        else
                            screenFilter.forceActiveFocus(Qt.TabFocusReason);
                    }
                    Keys.onDownPressed: {
                        if (popup.visible) {
                            incrementCurrentIndex();
                        } else if (FuseEmulator.settings.showOrientationChooser) {
                            screenOrientation.forceActiveFocus(Qt.TabFocusReason);
                        } else {
                            smoothScaling.forceActiveFocus(Qt.TabFocusReason);
                        }
                    }
                    Keys.onLeftPressed: if (!popup.visible) decrementCurrentIndex();
                    Keys.onRightPressed: if (!popup.visible) incrementCurrentIndex();

                    model: ["Preserve aspect fit", "Preserve aspect", "Stretch"]
                    currentIndex: FuseEmulator.settings.fillMode
                    onCurrentIndexChanged: {
                        FuseEmulator.settings.fillMode = currentIndex;
                        zxScreen.updateFillMode();
                    }
                }
            }

            RowLayout {
                visible: FuseEmulator.settings.showOrientationChooser
                spacing: 2.5 * Screen.pixelDensity
                Layout.fillWidth: true
                Label {
                    text: qsTr("Screen orientation")
                }
                ComboBox {
                    id: screenOrientation
                    Layout.fillWidth: true
                    Keys.onUpPressed: {
                        if (popup.visible)
                            decrementCurrentIndex();
                        else
                            screenFillMode.forceActiveFocus(Qt.TabFocusReason);
                    }
                    Keys.onDownPressed: {
                        if (popup.visible)
                            incrementCurrentIndex();
                        else
                            smoothScaling.forceActiveFocus(Qt.TabFocusReason);
                    }
                    Keys.onLeftPressed: if (!popup.visible) decrementCurrentIndex()
                    Keys.onRightPressed: if (!popup.visible) incrementCurrentIndex()

                    model: ["Any", "Landscape", "Portrait"]
                    currentIndex: FuseEmulator.settings.screenOrientation
                    onCurrentIndexChanged: FuseEmulator.settings.screenOrientation = currentIndex
                }
            }

            CheckBox {
                id: smoothScaling
                KeyNavigation.up: FuseEmulator.settings.showOrientationChooser ? screenOrientation : screenFillMode
                KeyNavigation.down: TextSizes.smallScreen ? leftBorder : screenFilter
                text: qsTr("Smooth scaling")
                checked: zxScreen.smoothScaling
                onCheckedChanged: zxScreen.smoothScaling = checked
            }

            CheckBox {
                id: leftBorder
                visible: TextSizes.smallScreen
                KeyNavigation.up: smoothScaling
                KeyNavigation.down: screenFilter
                text: qsTr("Left boder")
                checked: FuseEmulator.settings.leftMargin
                onCheckedChanged: FuseEmulator.settings.leftMargin = checked
            }

            Button {
                Layout.alignment: Qt.AlignHCenter
                text: qsTr("Close (B)")
                onClicked: pageLoader.source = ""
            }
        }
    }
}
