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

import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import Spectacol

// @scope main.qml

FocusScope
{
    id : fileList
    signal fileSelected(string filePath);
    // full screen, with a hint bar: the messages go above it
    readonly property real coverSize: mainScreen.portrait ? filesView.width * 0.8 : filesView.height * 0.65
    readonly property int coverPixels: Math.max(1, Math.ceil(coverSize * mainScreen.devicePixelRatio))
    readonly property real panelWidth: width
    readonly property real hintBarHeight: hintBar.visible ? hintBar.height : 0
    property alias rootFolder: filesModel.rootFolder
    property alias folder: filesModel.folder
    property alias filterClass: filesModel.filterClass
    property alias viewMode: browserPrefs.mode
    readonly property var deviceStorage: ZxEmulator.deviceStorage
    // Android: folders of the shared storage the user granted, listed in the Spectacol folder
    readonly property bool deviceFolders: deviceStorage.available && filterClass !== FolderListModel.HostFiles
    Settings {
        id: browserPrefs
        category: "LocalFilesView"
        property int mode: 0
    }
    function focusFiles() {
        if (viewMode === 0) filesView.forceActiveFocus();
        else compactView.forceActiveFocus();
    }
    function openRow(row: int) {
        if (!filesModel.path(row)) return;
        filesModel.currentIndex = row;
        filesModel.rememberSelection(row);
        if (filesModel.isDir(row)) filesModel.folder = filesModel.path(row);
        else fileSelected(filesModel.path(row));
    }
    function closeDialog(): bool {
        if (fileActions.close()) return true;
        if (modePicker.popup.visible) { modePicker.popup.close(); return true; }
        if (deviceFile.visible) { deviceFile.close(); focusFiles(); return true; }
        if (forgetDialog.visible) { forgetDialog.reject(); return true; }
        if (copyDialog.visible) { copyDialog.reject(); return true; }
        if (removeDialog.visible) { removeDialog.reject(); return true; }
        if (renameDialog.visible) { renameDialog.reject(); return true; }
        return false;
    }
    function renameCurrent() {
        const row = filesModel.currentIndex;
        const path = filesModel.path(row);
        if (!path || filesModel.isDeviceFolder(row))
            return;
        renameDialog.filePath = path;
        renameField.text = filesModel.entryName(row);
        renameDialog.open();
    }
    Component.onCompleted: {
        ZxEmulator.processInputEvents = false;
        Qt.callLater(focusFiles);
    }
    Component.onDestruction: ZxEmulator.processInputEvents = true
    onViewModeChanged: Qt.callLater(focusFiles)
    Keys.onPressed: event => {
        if (event.key === Qt.Key_F2) viewMode = (viewMode + 1) % 3;
        else if (event.key === Qt.Key_F3) search.forceActiveFocus();
        else if (event.key === Qt.Key_F6) renameCurrent();
        else if (event.key === Qt.Key_Backspace) filesModel.cdUp();
        else if (event.key === Qt.Key_Home) folder = ZxEmulator.dataPath;
        else if (event.key === Qt.Key_Escape) {
            if (!closeDialog()) fileSelected("");
        } else if ([Qt.Key_Delete, Qt.Key_X].includes(event.key) && viewMode !== 0) removeDialog.remove();
        else if (event.key === Qt.Key_C && viewMode !== 0) filesView.copyPressed();
        else if (event.key === Qt.Key_P && viewMode !== 0) copyDialog.copy();
        else if (event.key === Qt.Key_Y && viewMode !== 0) folder = ZxEmulator.dataPath;
        else if (event.key === Qt.Key_F4 && deviceFolders) deviceStorage.addFolder();
        else return;
        event.accepted = true;
    }

    FileDialog {
        id: deviceFile
        title: qsTr("Open a file from device storage")
        fileMode: FileDialog.OpenFile
        nameFilters: [qsTr("All files (*)")]
        onAccepted: {
            ZxEmulator.importFile(selectedFile, fileList, path => {
                if (path) fileList.fileSelected(path);
            });
        }
    }

    DebuggerGamepad {
        buttonXKey: Qt.Key_Delete
        buttonYKey: Qt.Key_F3
        buttonStartKey: Qt.Key_F2
        leftShoulderKey: Qt.Key_C
        rightShoulderKey: Qt.Key_P
        leftTriggerKey: Qt.Key_F6
        rightTriggerKey: fileList.deviceFolders ? Qt.Key_F4 : Qt.Key_unknown
    }

    Connections {
        target: fileList.deviceStorage
        function onFolderAdded(path) {
            fileList.folder = path;
            fileList.focusFiles();
        }
    }

    function copyCurrent() {
        const row = filesModel.currentIndex;
        if (filesModel.isDeviceFolder(row)) {
            messagePage.showMessage(ZxEmulator.Info, qsTr("Open the device folder and copy the games inside it."));
            return;
        }
        filesView.copyPath = filesModel.path(row);
    }


    transform: Rotation {
        angle : 0
        axis {x : 0; y: 1; z : 0}
        origin.x : width * 0.5
        origin.y : height * 0.5
    }


    // The covers need a calm background, not the game
    Rectangle {
        anchors.fill: parent
        z: -10
        color: Theme.raised
    }

    ConfirmDialog {
        id: copyDialog
        text: qsTr("Replace \"%1\" in this folder?").arg(ZxEmulator.fileName(filesView.copyPath))
        acceptText: qsTr("Replace")
        rejectText: qsTr("Cancel")
        destructive: true
        onClosed: fileList.focusFiles()
        onAccepted: {
            ZxEmulator.copy(filesView.copyPath, fileList.folder, true);
            filesView.copyPath = "";
            filesModel.refresh();
            filesView.currentIndex = 0;
        }

        function copy() {
            if (!filesView.canCopy)
                return;

            if (!ZxEmulator.copy(filesView.copyPath, fileList.folder)) {
                copyDialog.open();
                return;
            }
            filesView.copyPath = "";
            filesModel.refresh();
            filesView.currentIndex = 0;
        }
    }

    FolderListModel {
        id: filesModel
        rootFolder: ZxEmulator.settings.restrictToSpectacol ? ZxEmulator.dataPath : "/"
        homeFolder: fileList.deviceFolders ? ZxEmulator.dataPath : ""
        deviceFolders: fileList.deviceFolders ? fileList.deviceStorage.folders : []
        sortCriteria: folder.indexOf(ZxEmulator.snapshotsPath(), 0) === 0 ? FolderListModel.ByDateDesc : folder.indexOf(ZxEmulator.recordingsPath(), 0) === 0 ? FolderListModel.ByDateDesc : FolderListModel.ByName
    }

    ConfirmDialog {
        id: removeDialog
        text: isDirectory ? qsTr("Delete \"%1\" and everything in it?").arg(fileName)
                          : qsTr("Delete \"%1\"?").arg(fileName)
        acceptText: qsTr("Delete")
        rejectText: qsTr("Cancel")
        destructive: true
        onClosed: fileList.focusFiles()
        property string filePath
        property string fileName
        property bool isDirectory
        onAccepted: {
            const error = filesModel.removeEntry(filePath);
            if (error)
                messagePage.showMessage(ZxEmulator.Error, error);
        }
        function remove() {
            const row = filesModel.currentIndex;
            const path = filesModel.path(row);
            if (!path)
                return;
            if (filesModel.isDeviceFolder(row)) {
                forgetDialog.filePath = path;
                forgetDialog.fileName = filesModel.entryName(row);
                forgetDialog.open();
                return;
            }
            removeDialog.filePath = path;
            removeDialog.fileName = filesModel.entryName(row);
            removeDialog.isDirectory = filesModel.isDir(row);
            removeDialog.open();
        }
    }

    FormDialog {
        id: renameDialog
        objectName: "browserRename"
        property string filePath
        title: qsTr("Rename")
        acceptText: qsTr("Rename")
        initialFocus: renameField
        acceptable: renameField.text.trim().length > 0
        onOpened: renameField.selectAll()
        onClosed: fileList.focusFiles()
        commit: () => {
            errorMessage = filesModel.renameEntry(filePath, renameField.text);
            return errorMessage === "";
        }
        FancyTextField {
            id: renameField
            objectName: "browserRenameName"
            Layout.fillWidth: true
            Accessible.name: qsTr("New name")
            onAccepted: renameDialog.save()
        }
    }

    // Removing a device folder only gives back Spectacol's access to it
    ConfirmDialog {
        id: forgetDialog
        property string filePath
        property string fileName
        text: qsTr("Stop showing \"%1\"? Its files stay on the device.").arg(fileName)
        acceptText: qsTr("Stop showing")
        rejectText: qsTr("Cancel")
        onClosed: fileList.focusFiles()
        onAccepted: fileList.deviceStorage.forgetFolder(filePath)
    }

    CoverFlow {
        id : filesView

        anchors.fill: undefined
        anchors { left: parent.left; right: parent.right; top: artist_text_label.bottom; bottom: hintBar.top }
        clip: true
        visible: fileList.viewMode === 0
        activeFocusOnTab: true
        model: visible ? filesModel : null
        onCurrentIndexChanged: if (visible && activeFocus) filesModel.currentIndex = currentIndex
        // Follow the model's remembered index for each folder. This is not a binding
        // because tapping a cover, copy and remove assign currentIndex directly.
        Component.onCompleted: currentIndex = filesModel.currentIndex
        Connections {
            target: filesModel
            function onCurrentIndexChanged() {
                filesView.currentIndex = filesModel.currentIndex;
                compactView.currentIndex = filesModel.currentIndex;
            }
        }
        property string copyPath: ""
        onCopyPathChanged: {
            if (copyPath.length)
                messagePage.showMessage(ZxEmulator.Info, "\"" + ZxEmulator.fileName(copyPath) + qsTr("\" copied to clipboard. Use <b>Paste</b> to paste it to another folder."))
        }
        property bool canCopy: {
            if (!copyPath.length)
                return false;
            var srcPath = ZxEmulator.folderName(copyPath);
            if (ZxEmulator.isFolder(copyPath))
                return fileList.folder !== srcPath && !fileList.folder.startsWith(copyPath);
            return fileList.folder !== srcPath;
        }

        onReturnPressed: fileList.openRow(currentIndex)
        onEscapePressed: fileSelected("")

        onUpPressed: {
            filesModel.currentIndex = currentIndex;
            filesModel.cdUp();
        }

        onDeletePressed: removeDialog.remove()
        onResetPressed: fileList.folder = ZxEmulator.dataPath

        onCopyPressed: fileList.copyCurrent()
        onPastePressed: copyDialog.copy()

        delegate: Component {
            Image {
                id : album_delegate
                property real rotAngle : PathView.onPath ? PathView.delAngle : 0
                property real delScale : PathView.onPath ? PathView.delScale : 0.5
                property bool isCurrentItem : index === filesView.currentIndex;

                width : fileList.coverSize
                height : width
                fillMode: Image.PreserveAspectFit
                scale : delScale
                z : PathView.onPath ? PathView.delZ : -1
                Behavior on rotAngle {SpringAnimation {spring : 5; damping: 0.7; epsilon: 0.025}}
                Behavior on delScale {SpringAnimation {spring : 5; damping: 1; epsilon: 0.005}}
                onStatusChanged: if (status === Image.Ready) scale_anim.start();
                sourceSize: Qt.size(fileList.coverPixels, fileList.coverPixels)
                source: coverSource
                smooth : false
                // The file's kind until its screen is there
                Image {
                    anchors.fill: parent
                    visible: album_delegate.status !== Image.Ready
                    source: visible ? typeIcon : ""
                    sourceSize: album_delegate.sourceSize
                    fillMode: Image.PreserveAspectFit
                    asynchronous: true
                }

                NumberAnimation {id : scale_anim; target : album_delegate; property: "scale"; from : 0; to : delScale; duration : 500; easing.type: Easing.InOutQuad}
                transform: [Rotation {id : rotation; angle : rotAngle; axis {x : 0; y: 1; z : 0} origin.x : width * 0.5; origin.y : height * 0.5}]

                Rectangle {
                    width : parent.width
                    height: coverLabel.implicitHeight + Theme.sp(3)
                    anchors {
                        bottom : parent.bottom
                        horizontalCenter : parent.horizontalCenter
                    }
                    radius: Theme.radiusSmall
                    color: Theme.surface
                    border.width: isCurrentItem ? Theme.focusLine : Theme.line
                    border.color: isCurrentItem ? Theme.accent : Theme.outline
                    Text {
                        id: coverLabel
                        anchors.centerIn: parent
                        width: parent.width - Theme.sp(4)
                        font.pixelSize: Theme.body
                        font.weight: Font.DemiBold
                        color: Theme.text
                        text : name
                        elide: Text.ElideRight
                        maximumLineCount: 2
                        wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                        horizontalAlignment: Text.AlignHCenter
                    }
                }
                MouseArea {
                    anchors.fill: parent
                    property bool contextOnRelease: false
                    onPressed: contextOnRelease = false
                    onPressAndHold: contextOnRelease = true
                    onReleased: {
                        if (!contextOnRelease)
                            return;
                        filesView.forceActiveFocus();
                        filesView.currentIndex = index;
                        Qt.callLater(fileActions.openMenu);
                    }
                    onClicked: {
                        if (contextOnRelease)
                            return;
                        // A click also gives the keys back to the covers, as in the list
                        filesView.forceActiveFocus();
                        if (isCurrentItem) {
                            fileList.openRow(index);
                        } else {
                            filesView.currentIndex = index;
                        }
                    }
                }
            }
        }
    }
    HintBar {
        id: hintBar
        actionsFocusable: false
        visible: Theme.showHintBar
        height: visible ? implicitHeight : 0
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        hints: [
            { pad: "A", key: "Enter", text: qsTr("Open"), action: () => fileList.openRow(filesModel.currentIndex) },
            { pad: "B", key: "Esc", text: qsTr("Close"), action: () => fileList.fileSelected("") },
            { pad: "X", key: "Del", text: filesModel.isDeviceFolder(filesModel.currentIndex) ? qsTr("Stop showing") : qsTr("Delete"), action: () => removeDialog.remove() },
            { pad: "L1", key: "C", text: qsTr("Copy"), action: () => fileList.copyCurrent() },
            { pad: "R1", key: "P", text: filesView.canCopy ? qsTr("Paste") : qsTr("Paste (copy first)"), action: () => copyDialog.copy() },
            { pad: "Y", key: "F3", text: qsTr("Search"), action: () => search.forceActiveFocus() },
            { pad: "Start", key: "F2", text: qsTr("View"), action: () => fileList.viewMode = (fileList.viewMode + 1) % 3 },
            { pad: "", key: "Home", text: qsTr("Spectacol folder"), action: () => fileList.folder = ZxEmulator.dataPath }
        ].concat(filesModel.currentIndex >= 0 && !filesModel.isDeviceFolder(filesModel.currentIndex) ? [
            { pad: "L2", key: "F6", text: qsTr("Rename"), action: fileList.renameCurrent }
        ] : []).concat(fileList.deviceFolders ? [
            { pad: "R2", key: "F4", text: qsTr("Add device folder…"), action: () => fileList.deviceStorage.addFolder() }
        ] : [])
    }

    CompactFilesView {
        id: compactView
        objectName: "compactFiles"
        visible: fileList.viewMode !== 0
        grid: fileList.viewMode === 2
        anchors { left: parent.left; right: parent.right; top: artist_text_label.bottom; bottom: hintBar.top; margins: Theme.sp(2) }
        model: filesModel
        onCurrentIndexChanged: if (visible && activeFocus) filesModel.currentIndex = currentIndex
        onOpenRequested: row => fileList.openRow(row)
        onContextRequested: row => {
            filesModel.currentIndex = row;
            fileActions.openMenu();
        }
    }
    // A device folder is being listed
    BusyIndicator {
        objectName: "filesBusy"
        anchors.centerIn: filesView
        running: filesModel.busy
        visible: running
        z: 10
    }
    // Where we are; a click (or Up) goes to the parent folder, or closes at the top
    Rectangle {
        id : artist_text_label
        property bool root: filesModel.rootFolder === filesModel.folder
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: headerColumn.implicitHeight + Theme.sp(Theme.touchScreenUi ? 2 : 8)
        color: Theme.surface

        Column {
            id: headerColumn
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.leftMargin: Theme.sp(Theme.touchScreenUi ? 1 : 6)
            anchors.rightMargin: Theme.sp(Theme.touchScreenUi ? 1 : 6)
            spacing: Theme.sp(1)
            RowLayout {
                visible: Theme.touchScreenUi
                width: parent.width
                spacing: Theme.sp(2)
                ToolButton {
                    objectName: "filesClose"
                    implicitWidth: Theme.rowHeight
                    implicitHeight: Theme.rowHeight
                    text: qsTr("Close files")
                    icon.source: "qrc:/images/back.svg"
                    icon.color: Theme.text
                    icon.width: Theme.sp(6)
                    icon.height: Theme.sp(6)
                    display: AbstractButton.IconOnly
                    onClicked: fileList.fileSelected("")
                }
                Label {
                    Layout.fillWidth: true
                    text: artist_text_label.root ? qsTr("Open a file") : filesModel.folderName
                    elide: Text.ElideMiddle
                    font.pixelSize: Theme.title
                    color: Theme.text
                }
                PageActions {
                    id: fileActions
                    actions: hintBar.hints.concat(Qt.platform.os === "android" ? [
                        { text: qsTr("Device files…"), action: () => deviceFile.open() }
                    ] : [])
                    onDismissed: fileList.focusFiles()
                }
            }
            RowLayout {
                width: parent.width
                Button {
                    objectName: "filesUp"
                    text: qsTr("Up")
                    icon.source: Theme.touchScreenUi ? "qrc:/images/up.svg" : ""
                    icon.color: Theme.text
                    icon.width: Theme.sp(6)
                    icon.height: Theme.sp(6)
                    display: Theme.touchScreenUi ? AbstractButton.IconOnly : AbstractButton.TextOnly
                    Accessible.name: qsTr("Up a folder")
                    Layout.minimumWidth: Theme.rowHeight
                    Layout.minimumHeight: Theme.rowHeight
                    enabled: !artist_text_label.root
                    // Give the keys back to the files: at the top the button disables itself, focus and all
                    onClicked: {
                        filesModel.cdUp();
                        fileList.focusFiles();
                    }
                    Keys.onDownPressed: fileList.focusFiles()
                }
                NavigationComboBox {
                    id: modePicker
                    objectName: "filesViewMode"
                    model: [qsTr("Covers"), qsTr("List"), qsTr("Grid")]
                    currentIndex: fileList.viewMode
                    onActivated: {
                        fileList.viewMode = currentIndex;
                        fileList.focusFiles();
                    }
                }
                FancyTextField {
                    id: search
                    objectName: "filesSearch"
                    Layout.fillWidth: true
                    placeholderText: Theme.keyboardHints ? qsTr("Search filenames (F3)") : qsTr("Search filenames")
                    Layout.minimumWidth: 0
                    text: filesModel.search
                    onTextEdited: filesModel.search = text
                    onAccepted: fileList.focusFiles()
                    Keys.onDownPressed: fileList.focusFiles()
                    Keys.onEscapePressed: {
                        if (text.length) { clear(); filesModel.search = ""; }
                        else fileList.focusFiles();
                    }
                }
            }
            Text {
                visible: !Theme.touchScreenUi
                text: (artist_text_label.root ? qsTr("Open a file") : qsTr("‹ Up a folder")).toUpperCase()
                font.pixelSize: Theme.caption
                font.letterSpacing: Theme.uiScale
                color: Theme.textMuted
            }
            Text {
                visible: !Theme.touchScreenUi
                width: parent.width
                text: filesModel.folderTitle
                elide: Text.ElideMiddle
                font.pixelSize: Theme.title
                font.weight: Font.DemiBold
                color: Theme.text
            }
        }
        Rectangle {
            anchors.bottom: parent.bottom
            width: parent.width
            height: Theme.line
            color: Theme.outline
        }

    }

    ShaderEffect {
        visible: fileList.viewMode === 0 && !Theme.touchScreenUi
        smooth: true
        property variant source : ShaderEffectSource {
            sourceItem: filesView
            live: fileList.viewMode === 0 && !Theme.touchScreenUi
            width : fileList.width
            height : fileList.height
        }
        z : -5
        anchors
        {
            left : parent.left
            right : parent.right
            top : parent.verticalCenter
        }
        height : parent.height

        vertexShader: "qrc:///shaders/mirror.vert.qsb"
        fragmentShader: "qrc:///shaders/mirror.frag.qsb"
    }
}
