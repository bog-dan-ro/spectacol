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
        Column {
            spacing: 2.5 * Screen.pixelDensity
            anchors.horizontalCenter: parent.horizontalCenter
            Label {
                text: qsTr("General Options")
            }

            Row {
                spacing: 2.5 * Screen.pixelDensity
                Label {
                    text: qsTr("Emulation Speed")
                    anchors.verticalCenter: parent.verticalCenter
                }
                SpinBox {
                    id: emulationSpeed
                    focus: true
                    Keys.onUpPressed: detectLoaders.forceActiveFocus(Qt.TabFocusReason)
                    Keys.onDownPressed: autoSave.forceActiveFocus(Qt.TabFocusReason)
                    Keys.onLeftPressed: decrease()
                    Keys.onRightPressed: increase()
                    from: 10
                    to: 10000
                    value: fuseSettings.emulationSpeed
                    onValueChanged: fuseSettings.emulationSpeed = value
                }
            }

            CheckBox {
                id: autoSave
                KeyNavigation.up: emulationSpeed
                KeyNavigation.down: restrictBrowse
                text: qsTr("Auto save snapshot on exit")
                checked: fuseSettings.autoSaveOnExit
                onCheckedChanged: fuseSettings.autoSaveOnExit = checked
            }

            CheckBox {
                id: restrictBrowse
                KeyNavigation.up: autoSave
                KeyNavigation.down: fuse.touchscreen ? swipeForMenu : autoLoad
                text: qsTr("Restrict browsing to Spectacol folder")
                checked: fuseSettings.restrictToSpectacol
                onCheckedChanged: fuseSettings.restrictToSpectacol = checked
            }

            CheckBox {
                id: swipeForMenu
                visible: fuse.touchscreen
                KeyNavigation.up: restrictBrowse
                KeyNavigation.down: autoLoad
                text: qsTr("Swipe to open the menu")
                checked: fuseSettings.swipe4menu
                onCheckedChanged: fuseSettings.swipe4menu = checked
            }

            CheckBox {
                id: autoLoad
                KeyNavigation.up: fuse.touchscreen ? swipeForMenu : restrictBrowse
                KeyNavigation.down: detectLoaders
                text: qsTr("Auto load media")
                checked: fuseSettings.autoLoad
                onCheckedChanged: fuseSettings.autoLoad = checked
            }

            CheckBox {
                id: detectLoaders
                KeyNavigation.up: autoLoad
                KeyNavigation.down: loaderAcceleration
                text: qsTr("Detect loaders")
                checked: fuseSettings.detectLoaders
                onCheckedChanged: fuseSettings.detectLoaders = checked
            }

            Row {
                spacing: 2.5 * Screen.pixelDensity
                Label {
                    text: qsTr("Loader acceleration")
                    anchors.verticalCenter: parent.verticalCenter
                }
                ComboBox {
                    id: loaderAcceleration
                    Keys.onUpPressed: {
                        if (popup.visible)
                            decrementCurrentIndex();
                        else
                            detectLoaders.forceActiveFocus(Qt.TabFocusReason);
                    }
                    Keys.onDownPressed: {
                        if (popup.visible)
                            incrementCurrentIndex();
                        else
                            emulationSpeed.forceActiveFocus(Qt.TabFocusReason);
                    }
                    Keys.onLeftPressed: if (!popup.visible)  decrementCurrentIndex()
                    Keys.onRightPressed: if (!popup.visible) incrementCurrentIndex()

                    model: ["None", "Safe", "Turbo"]
                    currentIndex: fuseSettings.loaderAcceleration
                    onCurrentIndexChanged: fuseSettings.loaderAcceleration = currentIndex
                }
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Close (B)")
                onClicked: pageLoader.source = ""
            }
        }
    }
}
