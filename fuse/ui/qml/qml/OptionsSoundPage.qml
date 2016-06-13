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

// @scope main.qml

Flickable {
    contentHeight: pane.height
    Component.onCompleted: fuse.paused = true
    Component.onDestruction: fuse.paused = false
    anchors.margins: Screen.pixelDensity

    function getIndex(data, str) {
        for (var i = 0; i < data.length; ++i)
            if (data[i] === str)
                return i;
        return 0;
    }

    GamepadKeyNavigation {
        gamepad: Gamepad { deviceId: fuse.gamepadId }
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
                checked: fuseSettings.soundEnabled
                onCheckedChanged: fuseSettings.soundEnabled = checked
            }

            CheckBox {
                id: loadingSound
                KeyNavigation.up: soundEnabled
                KeyNavigation.down: soundForce8Bit
                text: qsTr("Loading sound")
                checked: fuseSettings.loadingSound
                onCheckedChanged: fuseSettings.loadingSound = checked
            }

            CheckBox {
                id: soundForce8Bit
                KeyNavigation.up: loadingSound
                KeyNavigation.down: ayStereoSeparation
                text: qsTr("Force 8 bit")
                checked: fuseSettings.soundForce8Bit
                onCheckedChanged: fuseSettings.soundForce8Bit = checked
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
                    currentIndex: getIndex(model, fuseSettings.AYStereoSeparation)
                    onCurrentTextChanged: fuseSettings.AYStereoSeparation = currentText
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
                    currentIndex: getIndex(model, fuseSettings.speakerType)
                    onCurrentTextChanged: fuseSettings.speakerType = currentText
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
