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
import QtQuick.Layouts 1.1
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
                    anchors.verticalCenter: parent.verticalCenter
                }
                ComboBox {
                    focus: true
                    id: screenFilter
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

                    model: fuse.filtersModel
                    currentIndex: fuse.selectedFilterIndex
                    onCurrentIndexChanged: fuse.selectedFilterIndex = currentIndex
                }
            }

            RowLayout {
                spacing: 2.5 * Screen.pixelDensity
                Layout.fillWidth: true
                Label {
                    text: qsTr("Fill mode")
                    anchors.verticalCenter: parent.verticalCenter
                }
                ComboBox {
                    id: screenFillMode
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
                        } else if (fuseSettings.showOrientationChooser) {
                            screenOrientation.forceActiveFocus(Qt.TabFocusReason);
                        } else {
                            smoothScaling.forceActiveFocus(Qt.TabFocusReason);
                        }
                    }
                    Keys.onLeftPressed: if (!popup.visible) decrementCurrentIndex();
                    Keys.onRightPressed: if (!popup.visible) incrementCurrentIndex();

                    model: ["Preserve aspect fit", "Preserve aspect", "Stretch"]
                    currentIndex: fuseSettings.fillMode
                    onCurrentIndexChanged: {
                        fuseSettings.fillMode = currentIndex;
                        fuseScreen.updateFillMode();
                    }
                }
            }

            RowLayout {
                visible: fuseSettings.showOrientationChooser
                spacing: 2.5 * Screen.pixelDensity
                Layout.fillWidth: true
                Label {
                    text: qsTr("Screen orientation")
                    anchors.verticalCenter: parent.verticalCenter
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
                            screenFilter.forceActiveFocus(Qt.TabFocusReason);
                    }
                    Keys.onLeftPressed: if (!popup.visible) decrementCurrentIndex()
                    Keys.onRightPressed: if (!popup.visible) incrementCurrentIndex()

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
                checked: fuseSettings.leftMargin
                onCheckedChanged: fuseSettings.leftMargin = checked
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Close (B)")
                onClicked: pageLoader.source = ""
            }
        }
    }
}
