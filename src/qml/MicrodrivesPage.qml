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
import QtCore
import QtGamepadLegacy
import Spectacol

// @scope main.qml
FocusScope {
    id: page
    property int drive: 0
    function closeDialog(): bool {
        if (view.item && view.item.closeDialog()) return true;
        if (view.sourceComponent === browser) {
            view.sourceComponent = actions;
            return true;
        }
        return false;
    }

    Loader {
        id: view
        anchors.fill: parent
        focus: true
        sourceComponent: drives
    }

    Component {
        id: drives
        Sheet {
            title: qsTr("Microdrives")
            GamepadNavigation {
                gamepad: Gamepad { deviceId: ZxEmulator.gamepadId }
                buttonAKey: Qt.Key_Return
                buttonBKey: Qt.Key_Escape
            }
            OptionList {
                model: ZxEmulator.microdriveLabels()
                checkedIndex: page.drive
                onChosen: index => {
                    if (index >= 0) {
                        page.drive = index;
                        view.sourceComponent = actions;
                    }
                }
            }
        }
    }

    Component {
        id: actions
        Sheet {
            id: drivePage
            readonly property bool inserted: ZxEmulator.microdriveInserted(page.drive)
            crumb: qsTr("Microdrives")
            title: ZxEmulator.microdriveLabels()[page.drive] || ""
            closeAction: () => view.sourceComponent = drives
            function closeDialog(): bool {
                if (discard.visible) discard.reject();
                else back();
                return true;
            }

            GamepadNavigation {
                gamepad: Gamepad { deviceId: ZxEmulator.gamepadId }
                buttonAKey: Qt.Key_Return
                buttonBKey: Qt.Key_Escape
            }

            ConfirmDialog {
                id: discard
                title: qsTr("Eject cartridge?")
                text: qsTr("Changes to this cartridge have not been saved.")
                acceptText: qsTr("Eject")
                destructive: true
                onAccepted: {
                    ZxEmulator.ejectMicrodrive(page.drive);
                    view.sourceComponent = drives;
                }
            }

            SettingsList {
                SettingRow {
                    visible: !drivePage.inserted
                    label: qsTr("Insert cartridge")
                    kind: "action"
                    onClicked: view.sourceComponent = browser
                }
                SettingRow {
                    visible: !drivePage.inserted
                    label: qsTr("Insert blank cartridge")
                    kind: "action"
                    onClicked: {
                        if (ZxEmulator.newMicrodrive(page.drive))
                            view.sourceComponent = drives;
                    }
                }
                SettingRow {
                    visible: drivePage.inserted && ZxEmulator.microdriveCanSave(page.drive)
                    label: qsTr("Save cartridge")
                    kind: "action"
                    onClicked: ZxEmulator.saveMicrodrive(page.drive, "")
                }
                SettingRow {
                    visible: drivePage.inserted
                    label: qsTr("Save cartridge as…")
                    kind: "action"
                    onClicked: view.sourceComponent = exporter
                }
                SettingRow {
                    visible: drivePage.inserted
                    label: qsTr("Eject cartridge")
                    kind: "action"
                    onClicked: {
                        if (ZxEmulator.microdriveModified(page.drive)) {
                            discard.open();
                        } else {
                            ZxEmulator.ejectMicrodrive(page.drive);
                            view.sourceComponent = drives;
                        }
                    }
                }
            }
        }
    }

    Component {
        id: browser
        CoverFlowFiles {
            id: files
            folder: ZxEmulator.dataPath
            filterClass: FolderListModel.Microdrives
            Settings {
                category: "MicrodriveFileBrowser"
                property alias path: files.folder
            }
            onFileSelected: path => {
                if (!path || ZxEmulator.insertMicrodrive(page.drive, path))
                    view.sourceComponent = path ? drives : actions;
            }
        }
    }

    Component {
        id: exporter
        SaveFilePage {
            id: savePage
            crumb: qsTr("Microdrives")
            title: qsTr("Save cartridge as")
            formats: ["mdr"]
            fileName: qsTr("Cartridge-%1.mdr").arg(page.drive + 1)
            folder: ZxEmulator.savesPath()
            destinationPath: folder + "/"
                + fileName.trim().replace(/[\\/]/g, "_").replace(/\.mdr$/i, "") + ".mdr"
            onAccepted: path => {
                if (ZxEmulator.saveMicrodrive(page.drive, path))
                    view.sourceComponent = actions;
            }
            onCanceled: view.sourceComponent = actions
        }
    }
}
