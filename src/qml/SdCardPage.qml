/*
    Copyright (c) 2026, BogDan Vatra <bogdan@kde.org>

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

// @scope main.qml
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import QtQuick.Window
import QtGamepadLegacy
import Spectacol

// The ZX Next's SD card: its files, copying host files onto it, another image
Sheet {
    id: page
    crumb: qsTr("Media")
    title: qsTr("SD card")
    preferredWidth: Theme.sp(180)
    hints: [
        { pad: "A", key: "Enter", text: qsTr("Open folder"), action: () => page.openCurrent() },
        { pad: "B", key: "Esc", text: qsTr("Close"), action: () => pageLoader.source = "" },
        { pad: "X", key: "Ins", text: qsTr("Add file"), action: () => { if (page.writable) pageLoader.setSource("FilePickerPage.qml",{purpose:"sdCopy"}); } },
        { pad: "Y", key: "Del", text: qsTr("Remove"), action: () => page.removeCurrent() }
    ]

    readonly property SdCardFiles card: ZxEmulator.sdCard
    readonly property var currentEntry: filesView.currentIndex >= 0 ? card.entries[filesView.currentIndex] : undefined
    readonly property bool writable: card.available && !card.readOnly

    function sizeText(bytes) {
        if (bytes >= 1024 * 1024 * 1024)
            return qsTr("%1 GB").arg((bytes / (1024 * 1024 * 1024)).toFixed(1));
        if (bytes >= 1024 * 1024)
            return qsTr("%1 MB").arg((bytes / (1024 * 1024)).toFixed(1));
        if (bytes >= 1024)
            return qsTr("%1 KB").arg(Math.round(bytes / 1024));
        return qsTr("%1 bytes").arg(bytes);
    }

    function openCurrent() {
        if (currentEntry && currentEntry.isDir) {
            card.openFolder(currentEntry.name);
            filesView.currentIndex = 0;
        }
    }

    function removeCurrent() {
        if (!writable || !currentEntry || currentEntry.name === "..")
            return;
        removeDialog.name = currentEntry.name;
        removeDialog.open();
    }

    Component.onCompleted: {
        card.refresh();
        filesView.forceActiveFocus();
    }

    GamepadNavigation {
        gamepad: Gamepad { deviceId: ZxEmulator.gamepadId }
        buttonBKey: Qt.Key_Escape
        buttonXKey: Qt.Key_Insert
        buttonYKey: Qt.Key_Delete
    }


    // In the window, and Return (the gamepad's A) confirms
    Dialog {
        id: updateDialog
        title: qsTr("Update downloaded image")
        anchors.centerIn: parent
        width: Math.min(page.width-Theme.sp(12),Theme.sp(140))
        modal: true
        focus: true
        standardButtons: Dialog.Ok | Dialog.Cancel
        onAccepted: {
            NextSdCard.updateDistro();
            pageLoader.setSource("NextSdCardPage.qml",{returnToCard:true});
        }
        onClosed: filesView.forceActiveFocus()
        Label {
            width: parent.width
            wrapMode: Text.WordWrap
            text: qsTr("Download the latest official emulator image? The previous downloaded image will be kept as a backup. A custom image is kept unchanged.")
            font.pixelSize: Theme.body
            color: Theme.text
        }
    }

    Dialog {
        id: removeDialog
        property string name
        title: qsTr("Remove")
        anchors.centerIn: parent
        width: Math.min(page.width * 0.8, Theme.body * 36)
        modal: true
        focus: true
        standardButtons: Dialog.Yes | Dialog.No
        onOpened: removeQuestion.forceActiveFocus()
        onAccepted: page.card.remove(name)
        onClosed: filesView.forceActiveFocus()
        FancyText {
            id: removeQuestion
            width: removeDialog.availableWidth
            wrapMode: Text.WordWrap
            text: qsTr("Remove '%1' from the SD card?").arg(removeDialog.name)
            Keys.onReturnPressed: removeDialog.accept()
        }
    }

    Dialog {
        id: folderDialog
        title: qsTr("New folder")
        anchors.centerIn: parent
        modal: true
        focus: true
        standardButtons: Dialog.Ok | Dialog.Cancel
        onOpened: {
            folderName.text = "";
            folderName.forceActiveFocus();
        }
        onAccepted: page.card.makeFolder(folderName.text)
        onClosed: filesView.forceActiveFocus()
        FancyTextField {
            id: folderName
            implicitWidth: page.width / 3
            placeholderText: qsTr("Folder name")
            onAccepted: folderDialog.accept()
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.sp(6)
        anchors.topMargin: Theme.sp(4)
        spacing: Theme.sp(1)

        FancyText {
            Layout.fillWidth: true
            color: Theme.textMuted
            elide: Text.ElideMiddle
            text: page.card.customImage ? page.card.imagePath : qsTr("%1 (downloaded)").arg(page.card.imagePath)
        }

        FancyText {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            color: Theme.textMuted
            text: {
                if (!page.card.available)
                    return qsTr("No SD card image, or it has no FAT file system.");
                let text = qsTr("%1 free of %2").arg(page.sizeText(page.card.freeBytes)).arg(page.sizeText(page.card.totalBytes));
                if (page.card.readOnly)
                    text += qsTr(", read only");
                if (page.card.inUse)
                    text += qsTr(". Changes restart the machine.");
                return text;
            }
        }

        FancyText {
            Layout.fillWidth: true
            elide: Text.ElideMiddle
            visible: page.card.available
            Layout.topMargin: Theme.sp(3)
            text: page.card.folder
            font.weight: Font.DemiBold
        }

        ListView {
            id: filesView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            focus: true
            model: page.card.entries
            highlightFollowsCurrentItem: true
            highlightMoveDuration: 0
            highlight: Rectangle {
                radius: Theme.radiusSmall
                color: Theme.accentFill
                border.width: Theme.focusLine
                border.color: Theme.accent
            }

            delegate: Item {
                id: entry
                required property var modelData
                required property int index
                width: filesView.width
                height: Theme.rowHeight

                FancyText {
                    anchors.left: parent.left
                    anchors.right: sizeLabel.left
                    anchors.leftMargin: Theme.sp(3)
                    anchors.verticalCenter: parent.verticalCenter
                    elide: Text.ElideRight
                    text: entry.modelData.isDir ? entry.modelData.name + "/" : entry.modelData.name
                }
                FancyText {
                    id: sizeLabel
                    anchors.right: parent.right
                    anchors.rightMargin: Theme.sp(3)
                    anchors.verticalCenter: parent.verticalCenter
                    text: entry.modelData.isDir ? "" : page.sizeText(entry.modelData.size)
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: filesView.currentIndex = entry.index
                    onDoubleClicked: page.openCurrent()
                }
            }

            Keys.onReturnPressed: page.openCurrent()
            Keys.onPressed: (event) => {
                if (event.key === Qt.Key_Insert && page.writable) {
                    pageLoader.setSource("FilePickerPage.qml",{purpose:"sdCopy"});
                    event.accepted = true;
                } else if (event.key === Qt.Key_Delete) {
                    page.removeCurrent();
                    event.accepted = true;
                }
            }
        }

        Flow {
            Layout.fillWidth: true
            spacing: Theme.sp(2)
            Button {
                text: qsTr("New folder")
                enabled: page.writable
                flat: true
                onClicked: folderDialog.open()
            }
            Button {
                text: qsTr("Other image")
                flat: true
                onClicked: pageLoader.setSource("FilePickerPage.qml",{purpose:"sdImage"})
            }
            Button {
                text: qsTr("Update downloaded image")
                flat: true
                onClicked: updateDialog.open()
            }
            Button {
                text: qsTr("Downloaded image")
                visible: page.card.customImage
                flat: true
                onClicked: page.card.useImage("")
            }
        }
    }
}
