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
import QtQuick.Controls
import QtQuick.Layouts
import QtGamepadLegacy
import Spectacol

// @scope main.qml

Flickable {
    contentHeight: pane.height
    Component.onCompleted: FuseEmulator.paused = true
    Component.onDestruction: FuseEmulator.paused = false
    anchors.margins: Screen.pixelDensity

    function getIndex(data, str) {
        for (var i = 0; i < data.length; ++i)
            if (data[i] === str)
                return i;
        return 0;
    }

    GamepadKeyNavigation {
        gamepad: Gamepad { deviceId: FuseEmulator.gamepadId }
        buttonBKey: Qt.Key_Escape
        buttonAKey: Qt.Key_Space
    }

    FancyPane {
        id: pane
        ColumnLayout {
            spacing: 2.5 * Screen.pixelDensity
            anchors.horizontalCenter: parent.horizontalCenter
            Label {
                text: qsTr("Sound Options")
            }


            CheckBox {
                id: soundEnabled
                KeyNavigation.up: speakerType
                KeyNavigation.down: loadingSound
                focus: true
                text: qsTr("Sound enabled")
                checked: FuseEmulator.settings.soundEnabled
                onCheckedChanged: FuseEmulator.settings.soundEnabled = checked
            }

            CheckBox {
                id: loadingSound
                KeyNavigation.up: soundEnabled
                KeyNavigation.down: soundForce8Bit
                text: qsTr("Loading sound")
                checked: FuseEmulator.settings.loadingSound
                onCheckedChanged: FuseEmulator.settings.loadingSound = checked
            }

            CheckBox {
                id: soundForce8Bit
                KeyNavigation.up: loadingSound
                KeyNavigation.down: ayStereoSeparation
                text: qsTr("Force 8 bit")
                checked: FuseEmulator.settings.soundForce8Bit
                onCheckedChanged: FuseEmulator.settings.soundForce8Bit = checked
            }

            RowLayout {
                spacing: 2.5 * Screen.pixelDensity
                Layout.fillWidth: true
                Label {
                    text: qsTr("AY stereo separation")
                    anchors.verticalCenter: parent.verticalCenter
                }

                ComboBox {
                    id: ayStereoSeparation
                    Layout.fillWidth: true
                    Keys.onUpPressed: {
                        if (popup.visible)
                            decrementCurrentIndex();
                        else
                            soundForce8Bit.forceActiveFocus(Qt.TabFocusReason);
                    }
                    Keys.onDownPressed: {
                        if (popup.visible)
                            incrementCurrentIndex();
                        else
                            speakerType.forceActiveFocus(Qt.TabFocusReason);
                    }
                    Keys.onLeftPressed: if (!popup.visible) decrementCurrentIndex();
                    Keys.onRightPressed: if (!popup.visible) incrementCurrentIndex();
                    model: ["None", "ACB", "ABC"]
                    currentIndex: getIndex(model, FuseEmulator.settings.AYStereoSeparation)
                    onCurrentTextChanged: FuseEmulator.settings.AYStereoSeparation = currentText
                }
            }

            RowLayout {
                spacing: 2.5 * Screen.pixelDensity
                Layout.fillWidth: true
                Label {
                    text: qsTr("Speaker type")
                    anchors.verticalCenter: parent.verticalCenter
                }

                ComboBox {
                    id: speakerType
                    Layout.fillWidth: true
                    Keys.onUpPressed: {
                        if (popup.visible)
                            decrementCurrentIndex();
                        else
                            ayStereoSeparation.forceActiveFocus(Qt.TabFocusReason);
                    }
                    Keys.onDownPressed: {
                        if (popup.visible)
                            incrementCurrentIndex();
                        else
                            soundEnabled.forceActiveFocus(Qt.TabFocusReason);
                    }
                    Keys.onLeftPressed: if (!popup.visible) decrementCurrentIndex();
                    Keys.onRightPressed: if (!popup.visible) incrementCurrentIndex();
                    model: ["TV speaker", "Beeper", "Unfiltered"]
                    currentIndex: getIndex(model, FuseEmulator.settings.speakerType)
                    onCurrentTextChanged: FuseEmulator.settings.speakerType = currentText
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
