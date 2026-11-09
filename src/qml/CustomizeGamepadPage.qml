/*
    Copyright (c) 2015-2026, BogDan Vatra <bogdan@kde.org>

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
import QtQuick.Controls
import QtQuick.Layouts
import QtGamepadLegacy
import Spectacol

// @scope main.qml
Sheet {
    id: page
    crumb: qsTr("Options / Gamepad")
    title: qsTr("Controller mapping")
    property int revision: 0
    property int direction: -1
    property int buttonCode: -1
    property var returnFocus: null
    Component.onCompleted: ZxEmulator.processInputEvents = false
    Component.onDestruction: ZxEmulator.processInputEvents = true
    readonly property var directionLabels: [qsTr("Up"), qsTr("Down"), qsTr("Left"), qsTr("Right"), qsTr("Fire")]
    readonly property var keyChoices: {
        let keys = [
            {
                code: Qt.Key_Space,
                name: qsTr("Space")
            },
            {
                code: Qt.Key_Return,
                name: qsTr("Enter")
            },
            {
                code: Qt.Key_Shift,
                name: qsTr("Caps Shift")
            },
            {
                code: Qt.Key_Control,
                name: qsTr("Symbol Shift")
            }
        ];
        for (let code = Qt.Key_0; code <= Qt.Key_9; ++code)
            keys.push({
                code: code,
                name: String.fromCharCode(code)
            });
        for (let code = Qt.Key_A; code <= Qt.Key_Z; ++code)
            keys.push({
                code: code,
                name: String.fromCharCode(code)
            });
        return keys;
    }
    readonly property var actionChoices: [
        {
            code: ZxSettings.JoystickFire,
            name: qsTr("Joystick / mapped fire")
        },
        {
            code: ZxSettings.ToggleKeyboard,
            name: qsTr("Show keyboard")
        },
        {
            code: ZxSettings.ToggleCursorJoystick,
            name: qsTr("Show numeric keypad")
        },
        {
            code: ZxSettings.TogglePause,
            name: qsTr("Pause / resume")
        },
        {
            code: ZxSettings.ToggleEmulationSpeed,
            name: qsTr("Fast-forward")
        },
        {
            code: ZxSettings.QuickSaveSnapshot,
            name: qsTr("Quick save")
        },
        {
            code: ZxSettings.QuickLoadSnapshot,
            name: qsTr("Quick load")
        }
    ].concat(keyChoices)
    readonly property var controllerButtons: [
        {
            code: GamepadManager.ButtonA,
            name: "A"
        },
        {
            code: GamepadManager.ButtonB,
            name: "B"
        },
        {
            code: GamepadManager.ButtonX,
            name: "X"
        },
        {
            code: GamepadManager.ButtonY,
            name: "Y"
        },
        {
            code: GamepadManager.ButtonL1,
            name: "L1"
        },
        {
            code: GamepadManager.ButtonR1,
            name: "R1"
        },
        {
            code: GamepadManager.ButtonL2,
            name: "L2"
        },
        {
            code: GamepadManager.ButtonR2,
            name: "R2"
        },
        {
            code: GamepadManager.ButtonSelect,
            name: qsTr("Select")
        },
        {
            code: GamepadManager.ButtonL3,
            name: qsTr("Left stick click")
        },
        {
            code: GamepadManager.ButtonR3,
            name: qsTr("Right stick click")
        }
    ]
    hints: [
        {
            pad: "A",
            key: "Enter",
            text: qsTr("Change")
        },
        {
            pad: "B",
            key: "Esc",
            text: qsTr("Back"),
            action: () => pageLoader.source = ""
        }
    ]
    GamepadNavigation {
        gamepad: Gamepad {
            deviceId: ZxEmulator.gamepadId
        }
        buttonAKey: Qt.Key_Return
        buttonBKey: Qt.Key_Escape
    }
    Connections {
        target: ZxEmulator.settings
        function onControllerMappingChanged() {
            ++page.revision;
        }
    }
    function actionName(button) {
        const changed = revision;
        let code = ZxEmulator.settings.gamepadAction(button);
        if (code === Qt.Key_Enter)
            code = Qt.Key_Return;
        return actionChoices.find(item => item.code === code)?.name || qsTr("Unknown mapping");
    }
    function choose(direction, button, row) {
        page.direction = direction;
        page.buttonCode = button;
        returnFocus = row;
        chooser.open();
    }
    SettingsList {
        id: settings
        SettingRow {
            label: qsTr("Controller profiles")
            description: ZxEmulator.controllerProfiles.gameProfile.length
                ? qsTr("This game: %1. Save profile edits to keep them.").arg(ZxEmulator.controllerProfiles.gameProfile)
                : qsTr("Save mappings and choose one for this game")
            kind: "action"
            onClicked: pageLoader.source = "ControllerProfilesPage.qml"
        }
        SettingRow {
            label: qsTr("Directions")
            description: ZxEmulator.settings.directionMapping === ZxSettings.QaopDirections ? qsTr("Q up · A down · O left · P right · Space fire") : qsTr("Both sticks, D-pad and the touch joystick")
            kind: "choice"
            model: [qsTr("Selected joystick"), qsTr("QAOP + Space"), qsTr("Custom keys")]
            currentIndex: ZxEmulator.settings.directionMapping
            onActivated: index => ZxEmulator.settings.directionMapping = index
        }
        Repeater {
            model: page.directionLabels
            delegate: SettingRow {
                id: directionRow
                required property int index
                required property string modelData
                visible: ZxEmulator.settings.directionMapping === ZxSettings.CustomDirections
                label: modelData
                kind: "action"
                actionText: ZxEmulator.settings.directionKeyNames[index]
                onClicked: page.choose(index, -1, directionRow)
            }
        }
        Text {
            width: parent.width
            padding: Theme.sp(3)
            text: qsTr("Buttons")
            font.pixelSize: Theme.title
            color: Theme.text
        }
        Repeater {
            model: page.controllerButtons
            delegate: SettingRow {
                id: buttonRow
                required property var modelData
                label: modelData.name
                kind: "action"
                actionText: page.actionName(modelData.code)
                onClicked: page.choose(-1, modelData.code, buttonRow)
            }
        }
        Text {
            width: parent.width
            padding: Theme.sp(3)
            text: qsTr("Start opens the menu. QAOP uses Q/A for up/down, O/P for left/right, and Space for fire.")
                + (ZxEmulator.controllerProfiles.gameProfile.length
                    ? qsTr(" Save your edits in Controller profiles to keep them for this game.")
                    : qsTr(" Mappings are remembered as your defaults."))
            wrapMode: Text.Wrap
            font.pixelSize: Theme.caption
            color: Theme.textMuted
        }
    }
    Dialog {
        id: chooser
        parent: Overlay.overlay
        x: Math.round((parent.width - width) / 2)
        y: Math.round((parent.height - height) / 2)
        width: Math.min(page.width - Theme.sp(8), Theme.sp(120))
        height: Math.min(page.height - Theme.sp(8), Theme.sp(140))
        modal: true
        focus: true
        title: page.direction >= 0 ? page.directionLabels[page.direction] : qsTr("Button action")
        Overlay.modal: Rectangle {
            color: Theme.scrim
        }
        background: Rectangle {
            color: Theme.raised
            radius: Theme.radius
            border.width: Theme.line
            border.color: Theme.outline
        }
        onOpened: {
            options.currentIndex = Math.max(0, options.checkedIndex);
            options.positionViewAtIndex(options.currentIndex, ListView.Contain);
            options.forceActiveFocus();
        }
        onClosed: {
            if (page.returnFocus) {
                page.returnFocus.forceActiveFocus();
                settings.ensureVisible(page.returnFocus);
            }
        }
        contentItem: Item {
            OptionList {
                id: options
                model: page.direction >= 0 ? page.keyChoices : page.actionChoices
                textOf: item => item.name
                checkedIndex: {
                    const changed = page.revision;
                    let code = page.direction >= 0 ? ZxEmulator.settings.directionKey(page.direction) : ZxEmulator.settings.gamepadAction(page.buttonCode);
                    if (code === Qt.Key_Enter)
                        code = Qt.Key_Return;
                    return model.findIndex(item => item.code === code);
                }
                onChosen: index => {
                    if (index < 0 || index >= model.length)
                        return;
                    if (page.direction >= 0)
                        ZxEmulator.settings.setDirectionKey(page.direction, model[index].code);
                    else
                        ZxEmulator.settings.setGamepadAction(page.buttonCode, model[index].code);
                    chooser.close();
                }
            }
        }
        footer: HintBar {
            hints: [
                {
                    pad: "A",
                    key: "Enter",
                    text: qsTr("Choose"),
                    action: () => options.chosen(options.currentIndex)
                },
                {
                    pad: "B",
                    key: "Esc",
                    text: qsTr("Back"),
                    action: () => chooser.close()
                }
            ]
        }
    }
}
