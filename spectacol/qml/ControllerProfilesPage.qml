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

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtGamepadLegacy
import Spectacol

// @scope Main.qml
Sheet {
    id: page
    crumb: qsTr("Options / Gamepad")
    title: qsTr("Controller profiles")
    readonly property var profiles: ZxEmulator.controllerProfiles
    property string selectedName: profiles.names.length ? profiles.names[0] : ""
    onCloseRequested: pageLoader.source = "CustomizeGamepadPage.qml"
    Component.onCompleted: ZxEmulator.processInputEvents = false
    Component.onDestruction: ZxEmulator.processInputEvents = true
    hints: [
        { pad: "A", key: "Enter", text: qsTr("Choose") },
        { pad: "B", key: "Esc", text: qsTr("Mapping"), action: () => page.closeRequested() }
    ]
    GamepadKeyNavigation {
        gamepad: Gamepad { deviceId: ZxEmulator.gamepadId }
        buttonAKey: Qt.Key_Return
        buttonBKey: Qt.Key_Escape
    }
    SettingsList {
        SettingRow {
            label: qsTr("Saved profile")
            description: page.selectedName.length > 16 ? page.selectedName : ""
            choiceMaximumWidth: Math.min(Theme.sp(28), page.width * 0.25)
            kind: "choice"
            model: page.profiles.names
            currentIndex: page.profiles.names.indexOf(page.selectedName)
            enabled: page.profiles.names.length > 0
            onActivated: index => page.selectedName = page.profiles.names[index]
        }
        SettingRow {
            label: qsTr("Load profile")
            description: qsTr("Apply its directions, buttons, joystick, dead zone and fast-forward mode")
            kind: "action"
            enabled: page.profiles.names.indexOf(page.selectedName) >= 0
            onClicked: ZxEmulator.loadControllerProfile(page.selectedName)
        }
        SettingRow {
            label: qsTr("Save current mapping")
            description: qsTr("Create a named profile or update one you already saved")
            kind: "action"
            onClicked: {
                nameEdit.text = page.selectedName || page.profiles.gameName;
                nameDialog.open();
            }
        }
        SettingRow {
            label: qsTr("Profile for this game")
            choiceMaximumWidth: Math.min(Theme.sp(28), page.width * 0.25)
            description: page.profiles.hasGame
                ? page.profiles.gameName + (page.profiles.gameProfile.length > 16 ? " — " + page.profiles.gameProfile : "")
                    + qsTr(" — matched by filename across folders") : qsTr("Load a game first")
            kind: "choice"
            enabled: page.profiles.hasGame
            model: [qsTr("Use default mapping")].concat(page.profiles.names)
            currentIndex: page.profiles.names.indexOf(page.profiles.gameProfile) + 1
            onActivated: index => page.profiles.assignGame(index === 0 ? "" : page.profiles.names[index - 1])
        }
        SettingRow {
            label: qsTr("Edit mapping")
            kind: "action"
            onClicked: pageLoader.source = "CustomizeGamepadPage.qml"
        }
        SettingRow {
            label: qsTr("Import profile…")
            kind: "action"
            onClicked: importDialog.open()
        }
        SettingRow {
            label: qsTr("Export profile…")
            kind: "action"
            enabled: page.profiles.names.indexOf(page.selectedName) >= 0
            onClicked: exportDialog.open()
        }
        SettingRow {
            label: qsTr("Delete profile…")
            kind: "action"
            enabled: page.profiles.names.indexOf(page.selectedName) >= 0
            onClicked: deleteDialog.open()
        }
        Text {
            width: parent.width
            padding: Theme.sp(3)
            text: qsTr("A game profile is temporary: other games keep your default mapping. A supported joystick declared by a snapshot takes priority when loading it.")
            wrapMode: Text.Wrap
            font.pixelSize: Theme.caption
            color: Theme.textMuted
        }
        Text {
            width: parent.width
            padding: Theme.sp(3)
            visible: text.length > 0
            text: page.profiles.lastError
            wrapMode: Text.Wrap
            font.pixelSize: Theme.body
            color: Theme.warning
        }
    }
    Dialog {
        id: nameDialog
        parent: Overlay.overlay
        title: qsTr("Save controller profile")
        modal: true
        focus: true
        x: Math.round((parent.width - width) / 2)
        y: Math.round((parent.height - height) / 2)
        width: Math.min(page.width - Theme.sp(8), Theme.sp(110))
        standardButtons: Dialog.Save | Dialog.Cancel
        onOpened: { nameEdit.forceActiveFocus(); nameEdit.selectAll(); }
        onAccepted: {
            if (ZxEmulator.saveControllerProfile(nameEdit.text))
                page.selectedName = nameEdit.text.trim();
        }
        contentItem: FancyTextField {
            id: nameEdit
            objectName: "profileName"
            maximumLength: 64
            placeholderText: qsTr("Profile name")
            selectByMouse: true
            onAccepted: nameDialog.accept()
        }
    }
    Dialog {
        id: deleteDialog
        parent: Overlay.overlay
        title: qsTr("Delete profile “%1”?").arg(page.selectedName)
        modal: true
        focus: true
        x: Math.round((parent.width - width) / 2)
        y: Math.round((parent.height - height) / 2)
        standardButtons: Dialog.Yes | Dialog.No
        onAccepted: {
            page.profiles.remove(page.selectedName);
            page.selectedName = page.profiles.names.length ? page.profiles.names[0] : "";
        }
    }
    FileDialog {
        id: importDialog
        title: qsTr("Import controller profile")
        nameFilters: [qsTr("Controller profiles (*.json)")]
        fileMode: FileDialog.OpenFile
        onAccepted: {
            const name = page.profiles.importProfile(selectedFile);
            if (name.length) page.selectedName = name;
        }
    }
    FileDialog {
        id: exportDialog
        title: qsTr("Export controller profile")
        nameFilters: [qsTr("Controller profiles (*.json)")]
        defaultSuffix: "json"
        fileMode: FileDialog.SaveFile
        currentFolder: "file://" + ZxEmulator.dataPath
        onAccepted: page.profiles.exportProfile(page.selectedName, selectedFile)
    }
}
