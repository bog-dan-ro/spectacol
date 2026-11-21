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
import QtQuick.Layouts
import Spectacol

FormDialog {
    id: dialog
    property bool directory: false
    property bool saveFile: false
    property string defaultSuffix
    property string path: ""
    property var submit: null
    property var nameFilters: [qsTr("All files (*)")]
    initialFocus: saveFile ? null : pathField
    acceptable: pathField.text.trim().length > 0
    commit: () => !submit || submit(pathField.text.trim())
    onOpened: {
        pathField.text = path;
        if (saveFile && acceptable) {
            saveButton.forceActiveFocus();
        } else {
            pathField.forceActiveFocus();
            pathField.selectAll();
        }
    }
    function closeNested(): bool {
        if (filePicker.visible) { filePicker.close(); browse.forceActiveFocus(); return true; }
        if (folderPicker.visible) { folderPicker.close(); browse.forceActiveFocus(); return true; }
        if (visible) { reject(); return true; }
        return false;
    }
    // A typed path as the pickers' URL; URLs (file://, content://) stay as they are
    function pathUrl(text: string): string {
        if (text.length === 0 || text.indexOf("://") > 0)
            return text;
        return (text.startsWith("/") ? "file://" : "file:///") + text;
    }
    Label {
        Layout.fillWidth: true
        text: dialog.directory ? qsTr("Folder path") : qsTr("File path")
        color: Theme.text
    }
    FancyTextField {
        id: pathField
        objectName: "hostPath"
        Layout.fillWidth: true
        inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText
        onAccepted: dialog.save()
    }
    DialogButton {
        id: browse
        objectName: dialog.directory ? "hostFolderBrowse" : "hostFileBrowse"
        text: qsTr("Browse…")
        onClicked: {
            // Start the picker on the current path; a save picker also proposes its name
            const url = dialog.pathUrl(pathField.text.trim());
            if (dialog.directory) {
                if (url)
                    folderPicker.selectedFolder = url;
                folderPicker.open();
            } else {
                if (url)
                    filePicker.selectedFile = url;
                filePicker.open();
            }
        }
    }
    FileDialog {
        id: filePicker
        options: Qt.platform.os === "android" ? 0 : FileDialog.DontUseNativeDialog
        title: dialog.title
        fileMode: dialog.saveFile ? FileDialog.SaveFile : FileDialog.OpenFile
        defaultSuffix: dialog.defaultSuffix
        nameFilters: dialog.nameFilters
        onAccepted: {
            pathField.text = selectedFile.toString();
            (dialog.saveFile ? dialog.saveButton : pathField).forceActiveFocus();
        }
        onRejected: browse.forceActiveFocus()
    }
    FolderDialog {
        id: folderPicker
        options: Qt.platform.os === "android" ? 0 : FolderDialog.DontUseNativeDialog
        title: dialog.title
        onAccepted: { pathField.text = selectedFolder.toString(); pathField.forceActiveFocus(); }
        onRejected: browse.forceActiveFocus()
    }
}
