/*
    Copyright (c) 2015-2026, BogDan Vatra <bogdan@kde.org>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtGamepadLegacy
import Spectacol

// @scope main.qml
// Spectacol 4 kept its files in shared storage, which Android no longer lets the app read:
// the user picks that folder once and its files are copied into the app's own storage.
Sheet {
    id: page
    readonly property var storage: ZxEmulator.deviceStorage
    readonly property bool finished: !storage.importing && storage.importSummary !== ""
    crumb: qsTr("Options")
    title: qsTr("Files from Spectacol 3")
    hints: storage.importing || finished ? [
        { pad: "B", key: "Esc", text: qsTr("Close"), action: () => mainScreen.closePage() }
    ] : [
        { pad: "A", key: "Enter", text: qsTr("Choose") },
        { pad: "B", key: "Esc", text: qsTr("Later"), action: () => page.later() }
    ]
    function later() {
        storage.postponeLegacyImport(false);
        mainScreen.closePage();
    }
    GamepadNavigation {
        gamepad: Gamepad { deviceId: ZxEmulator.gamepadId }
        buttonAKey: Qt.Key_Return
        buttonBKey: Qt.Key_Escape
    }
    Flickable {
        anchors.fill: parent
        contentHeight: column.implicitHeight + 2 * Theme.sp(6)
        clip: true
        ColumnLayout {
            id: column
            x: Theme.sp(6)
            y: Theme.sp(6)
            width: parent.width - 2 * Theme.sp(6)
            spacing: Theme.sp(5)
            Text {
                Layout.fillWidth: true
                text: qsTr("Spectacol 3 kept your games, snapshots and recordings in “%1”. Android lets Spectacol read that folder only after you choose it.").arg(page.storage.legacyFolder)
                wrapMode: Text.Wrap
                font.pixelSize: Theme.body
                color: Theme.text
            }
            Text {
                Layout.fillWidth: true
                text: qsTr("Choose the folder, then “Use this folder” and “Allow”. Spectacol copies its files into its own folder and leaves the originals where they are.")
                wrapMode: Text.Wrap
                font.pixelSize: Theme.body
                color: Theme.textMuted
            }
            Text {
                objectName: "importStatus"
                Layout.fillWidth: true
                visible: text !== ""
                text: page.storage.importing ? qsTr("Copying… %n file(s) so far", "", page.storage.importedFiles)
                                             : page.storage.importSummary
                wrapMode: Text.Wrap
                font.pixelSize: Theme.body
                font.weight: Font.DemiBold
                color: Theme.accent
            }
            Button {
                id: chooseButton
                objectName: "importChoose"
                visible: !page.finished
                enabled: !page.storage.importing
                focus: true
                text: qsTr("Choose the old folder")
                onClicked: page.storage.importLegacyFolder()
                Keys.onReturnPressed: clicked()
                KeyNavigation.down: laterButton
            }
            Button {
                id: laterButton
                visible: !page.storage.importing && !page.finished
                text: qsTr("Ask me later")
                onClicked: page.later()
                Keys.onReturnPressed: clicked()
                KeyNavigation.up: chooseButton
                KeyNavigation.down: neverButton
            }
            Button {
                id: neverButton
                visible: !page.storage.importing && !page.finished
                text: qsTr("Don't ask again")
                onClicked: {
                    page.storage.postponeLegacyImport(true);
                    pageLoader.source = "";
                }
                Keys.onReturnPressed: clicked()
                KeyNavigation.up: laterButton
            }
            Button {
                id: doneButton
                visible: page.finished
                text: qsTr("Done")
                onClicked: pageLoader.source = ""
                Keys.onReturnPressed: clicked()
                onVisibleChanged: if (visible) forceActiveFocus()
            }
        }
    }
}
