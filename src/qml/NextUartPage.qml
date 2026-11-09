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

Sheet {
    id: page
    crumb: qsTr("Options")
    title: qsTr("Next UART devices")
    hints: [{pad:"A",key:"Enter",text:qsTr("Change")},
            {pad:"B",key:"Esc",text:qsTr("Back"),action:()=>pageLoader.source="OptionsPeripheralsPage.qml"}]
    readonly property NextUartHooks hooks: ZxEmulator.uartHooks
    GamepadNavigation {
        gamepad: Gamepad { deviceId: ZxEmulator.gamepadId }
        buttonBKey: Qt.Key_Escape
    }
    function edit(kind, value) { editor.kind=kind; editor.value=value; editor.open(); }
    Dialog {
        id: editor
        property string kind
        property string value
        anchors.centerIn: parent
        width: Math.min(page.width-Theme.sp(12),Theme.sp(140))
        title: kind==="host" ? qsTr("Pi service host") : kind==="port" ? qsTr("Pi service port") : qsTr("Local script path or HTTPS URL")
        modal: true
        focus: true
        standardButtons: Dialog.Ok | Dialog.Cancel
        onOpened: { field.text=value; field.forceActiveFocus(); field.selectAll(); }
        onClosed: settings.forceActiveFocus()
        onAccepted: {
            if(kind==="host") page.hooks.piHost=field.text.trim();
            else if(kind==="port") page.hooks.piPort=Number(field.text);
            else if(field.text.startsWith("https://")) page.hooks.downloadScript(kind==="esp" ? 0 : 1,field.text);
            else if(kind==="esp") page.hooks.espScript=field.text.trim();
            else page.hooks.piScript=field.text.trim();
        }
        TextField {
            id: field
            width: parent.width
            font.pixelSize: Theme.body
            onAccepted: editor.accept()
        }
    }
    SettingsList {
        id: settings
        SettingRow {
            label: qsTr("ESP Wi-Fi module")
            description: page.hooks.espScript || qsTr("Bundled ESP AT commands with host networking")
            kind: "action"
            onClicked: page.edit("esp",page.hooks.espScript)
        }
        SettingRow {
            label: qsTr("Choose ESP script")
            kind: "action"
            onClicked: pageLoader.setSource("FilePickerPage.qml",{purpose:"uartEsp",returnPage:"NextUartPage.qml"})
        }
        SettingRow {
            label: qsTr("Pi module")
            description: page.hooks.piScript || qsTr("Bundled raw TCP bridge")
            kind: "action"
            onClicked: page.edit("pi",page.hooks.piScript)
        }
        SettingRow {
            label: qsTr("Choose Pi script")
            kind: "action"
            onClicked: pageLoader.setSource("FilePickerPage.qml",{purpose:"uartPi",returnPage:"NextUartPage.qml"})
        }
        SettingRow {
            label: qsTr("Pi service host")
            description: page.hooks.piHost
            kind: "action"
            onClicked: page.edit("host",page.hooks.piHost)
        }
        SettingRow {
            label: qsTr("Pi service port")
            description: page.hooks.piPort ? String(page.hooks.piPort) : qsTr("Disconnected (port 0)")
            kind: "action"
            onClicked: page.edit("port",String(page.hooks.piPort))
        }
        SettingRow {
            label: qsTr("Reload scripts")
            description: qsTr("Apply script edits and reconnect UART devices")
            kind: "action"
            onClicked: page.hooks.reload()
        }
        SettingRow {
            label: qsTr("Restore bundled scripts")
            kind: "action"
            onClicked: { page.hooks.useBundled(0); page.hooks.useBundled(1); }
        }
        FancyText {
            width: parent.width
            wrapMode: Text.WordWrap
            text: page.hooks.downloading ? qsTr("Downloading script…") : page.hooks.errorString
            color: Theme.textMuted
        }
    }
}
