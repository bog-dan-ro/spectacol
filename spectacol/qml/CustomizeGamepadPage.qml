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
import QtQuick.Layouts
import QtGamepadLegacy
import QtQuick.Controls
import Spectacol

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

    ListModel {
        id: buttonsModel
        ListElement { code: GamepadManager.ButtonA; name: "A" }
        ListElement { code: GamepadManager.ButtonB; name: "B" }
        ListElement { code: GamepadManager.ButtonX; name: "X" }
        ListElement { code: GamepadManager.ButtonY; name: "Y" }
        ListElement { code: GamepadManager.ButtonL1; name: "L1" }
        ListElement { code: GamepadManager.ButtonR1; name: "R1" }
        ListElement { code: GamepadManager.ButtonL2; name: "L2" }
        ListElement { code: GamepadManager.ButtonR2; name: "R2" }
    }

    ListModel {
        id: actionsModel
        ListElement { code: FuseSettings.JoystickFire; name: "Joystick fire" }

        // Actions shortcuts
        ListElement { code: FuseSettings.ToggleKeyboard; name: "Toggle Keyboard"; section: "Actions" }
        ListElement { code: FuseSettings.ToggleCursorJoystick; name: "Toggle Joystick"; section: "Actions" }
        ListElement { code: FuseSettings.TogglePause; name: "Toggle Pause"; section: "Actions" }
        ListElement { code: FuseSettings.ToggleEmulationSpeed; name: "Toggle Emulation Speed"; section: "Actions" }
        ListElement { code: FuseSettings.QuickSaveSnapshot; name : "Save Snapshot"; section: "Actions" }
        ListElement { code: FuseSettings.QuickLoadSnapshot; name: "Load Snapshot"; section: "Actions" }

        // Keys shortcuts
        ListElement { code: Qt.Key_Enter; name:"Enter"; section: "Keys" }
        ListElement { code: Qt.Key_Shift; name:"Caps Shift"; section: "Keys" }
        ListElement { code: Qt.Key_Control; name:"Symbol Shift"; buttonColor:"red"; section: "Keys" }
        ListElement { code: Qt.Key_Space; name:"Break Space"; section: "Keys" }

        ListElement { code: Qt.Key_0; name: "0"; section: "Keys" }
        ListElement { code: Qt.Key_1; name: "1"; section: "Keys" }
        ListElement { code: Qt.Key_2; name: "2"; section: "Keys" }
        ListElement { code: Qt.Key_3; name: "3"; section: "Keys" }
        ListElement { code: Qt.Key_4; name: "4"; section: "Keys" }
        ListElement { code: Qt.Key_5; name: "5"; section: "Keys" }
        ListElement { code: Qt.Key_6; name: "6"; section: "Keys" }
        ListElement { code: Qt.Key_7; name: "7"; section: "Keys" }
        ListElement { code: Qt.Key_8; name: "8"; section: "Keys" }
        ListElement { code: Qt.Key_9; name: "9"; section: "Keys" }

        ListElement { code: Qt.Key_A; name: "A"; section: "Keys" }
        ListElement { code: Qt.Key_B; name: "B"; section: "Keys" }
        ListElement { code: Qt.Key_C; name: "C"; section: "Keys" }
        ListElement { code: Qt.Key_D; name: "D"; section: "Keys" }
        ListElement { code: Qt.Key_E; name: "E"; section: "Keys" }
        ListElement { code: Qt.Key_F; name: "F"; section: "Keys" }
        ListElement { code: Qt.Key_G; name: "G"; section: "Keys" }
        ListElement { code: Qt.Key_H; name: "H"; section: "Keys" }
        ListElement { code: Qt.Key_I; name: "I"; section: "Keys" }
        ListElement { code: Qt.Key_J; name: "J"; section: "Keys" }
        ListElement { code: Qt.Key_K; name: "K"; section: "Keys" }
        ListElement { code: Qt.Key_L; name: "L"; section: "Keys" }
        ListElement { code: Qt.Key_M; name: "M"; section: "Keys" }
        ListElement { code: Qt.Key_N; name: "N"; section: "Keys" }
        ListElement { code: Qt.Key_O; name: "O"; section: "Keys" }
        ListElement { code: Qt.Key_P; name: "P"; section: "Keys" }
        ListElement { code: Qt.Key_Q; name: "Q"; section: "Keys" }
        ListElement { code: Qt.Key_R; name: "R"; section: "Keys" }
        ListElement { code: Qt.Key_S; name: "S"; section: "Keys" }
        ListElement { code: Qt.Key_T; name: "T"; section: "Keys" }
        ListElement { code: Qt.Key_U; name: "U"; section: "Keys" }
        ListElement { code: Qt.Key_V; name: "V"; section: "Keys" }
        ListElement { code: Qt.Key_W; name: "W"; section: "Keys" }
        ListElement { code: Qt.Key_X; name: "X"; section: "Keys" }
        ListElement { code: Qt.Key_Y; name: "Y"; section: "Keys" }
        ListElement { code: Qt.Key_Z; name: "Z"; section: "Keys" }
    }

    function actionIndex(action)
    {
        for (var i = 0; i < actionsModel.count; ++i) {
            if (actionsModel.get(i).code === action)
                return i;
        }
        return -1;
    }

    Component {
        id: sectionHeading
        FancyText {
            horizontalAlignment: Text.AlignHCenter
            text: section
        }
    }

    FancyPane {
        id: pane
        ColumnLayout {
            GridLayout {
                columns: 2
                FancyText {
                    Layout.alignment:Qt.AlignRight
                    text: qsTr("Gampad button")
                }
                ComboBox {
                    id: gamepadButton
                    Layout.fillWidth: true
                    focus: true
                    model: buttonsModel
                    textRole: "name"
                    Keys.onUpPressed: {
                        if (popup.visible)
                             decrementCurrentIndex();
                        else
                            shortcut.forceActiveFocus(Qt.TabFocusReason);
                    }
                    Keys.onDownPressed: {
                        if (popup.visible)
                            incrementCurrentIndex();
                        else
                            shortcut.forceActiveFocus(Qt.TabFocusReason);
                    }
                    Keys.onLeftPressed: if (!popup.visible) decrementCurrentIndex()
                    Keys.onRightPressed: if (!popup.visible) incrementCurrentIndex()

                }

                FancyText {
                    Layout.alignment:Qt.AlignRight
                    text: qsTr("Shortcut")
                }
                ComboBox {
                    id: shortcut
                    Layout.fillWidth: true
                    model: actionsModel
                    currentIndex: actionIndex(FuseEmulator.settings.gamepadAction(buttonsModel.get(gamepadButton.currentIndex).code))
                    onCurrentIndexChanged: FuseEmulator.settings.setGamepadAction(buttonsModel.get(gamepadButton.currentIndex).code, actionsModel.get(currentIndex).code)
                    textRole: "name"
                    Keys.onUpPressed: {
                        if (shortcut.popup.visible)
                             decrementCurrentIndex();
                        else
                            gamepadButton.forceActiveFocus(Qt.TabFocusReason);
                    }
                    Keys.onDownPressed: {
                        if (shortcut.popup.visible)
                            shortcut.incrementCurrentIndex();
                        else
                            gamepadButton.forceActiveFocus(Qt.TabFocusReason);
                    }
                    Keys.onLeftPressed: if (!popup.visible) decrementCurrentIndex()
                    Keys.onRightPressed: if (!popup.visible) incrementCurrentIndex()

                    Connections {
                        target: shortcut.popup
                        onOpened: {
                            shortcut.popup.contentItem.section.property = "section";
                            shortcut.popup.contentItem.section.delegate = sectionHeading;
                            shortcut.popup.contentItem.section.criteria = ViewSection.FullString;
                        }
                    }
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
