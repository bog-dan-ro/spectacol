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
    property string path: ""
    property var submit: null
    property var nameFilters: [qsTr("All files (*)")]
    initialFocus: pathField
    acceptable: pathField.text.trim().length > 0
    commit: () => !submit || submit(pathField.text.trim())
    onOpened: {
        pathField.text = path;
        pathField.forceActiveFocus();
        pathField.selectAll();
    }
    function closeNested(): bool {
        if (filePicker.visible) { filePicker.close(); browse.forceActiveFocus(); return true; }
        if (folderPicker.visible) { folderPicker.close(); browse.forceActiveFocus(); return true; }
        if (visible) { reject(); return true; }
        return false;
    }
    Label {
        Layout.fillWidth: true
        text: dialog.directory ? qsTr("Folder path") : qsTr("File path")
        color: Theme.text
    }
    TextField {
        id: pathField
        objectName: "hostPath"
        Layout.fillWidth: true
        inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText
        onAccepted: dialog.save()
    }
    Button {
        id: browse
        objectName: dialog.directory ? "hostFolderBrowse" : "hostFileBrowse"
        text: qsTr("Browse…")
        onClicked: dialog.directory ? folderPicker.open() : filePicker.open()
    }
    FileDialog {
        id: filePicker
        options: Qt.platform.os === "android" ? 0 : FileDialog.DontUseNativeDialog
        title: dialog.title
        nameFilters: dialog.nameFilters
        onAccepted: { pathField.text = selectedFile.toString(); pathField.forceActiveFocus(); }
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
