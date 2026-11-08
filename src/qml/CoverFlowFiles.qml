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

Item
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
        if (copyDialog.visible) { copyDialog.close(); focusFiles(); return true; }
        if (removeDialog.visible) { removeDialog.close(); focusFiles(); return true; }
        return false;
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
        else if (event.key === Qt.Key_Backspace) filesModel.cdUp();
        else if (event.key === Qt.Key_Home) folder = ZxEmulator.dataPath;
        else if (event.key === Qt.Key_Escape) {
            if (!closeDialog()) fileSelected("");
        } else if ([Qt.Key_Delete, Qt.Key_X].includes(event.key) && viewMode !== 0) removeDialog.remove();
        else if (event.key === Qt.Key_C && viewMode !== 0) filesView.copyPressed();
        else if (event.key === Qt.Key_P && viewMode !== 0) copyDialog.copy();
        else if (event.key === Qt.Key_Y && viewMode !== 0) folder = ZxEmulator.dataPath;
        else return;
        event.accepted = true;
    }

    FileDialog {
        id: deviceFile
        title: qsTr("Open a file from device storage")
        fileMode: FileDialog.OpenFile
        nameFilters: [qsTr("All files (*)")]
        onAccepted: {
            const path = ZxEmulator.importFile(selectedFile);
            if (path) fileList.fileSelected(path);
        }
    }

    DebuggerGamepad {
        buttonXKey: Qt.Key_Delete
        buttonYKey: Qt.Key_F3
        buttonStartKey: Qt.Key_F2
        leftShoulderKey: Qt.Key_C
        rightShoulderKey: Qt.Key_P
        leftTriggerKey: Qt.Key_Home
        rightTriggerKey: Qt.Key_unknown
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

    MessageDialog {
        id: copyDialog
        title: "Spectacol"
        text: qsTr("Override \"") + ZxEmulator.fileName(filesView.copyPath) + "\" ?";
        buttons: MessageDialog.Yes | MessageDialog.No
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
        sortCriteria: folder.indexOf(ZxEmulator.snapshotsPath(), 0) === 0 ? FolderListModel.ByDateDesc : folder.indexOf(ZxEmulator.recordingsPath(), 0) === 0 ? FolderListModel.ByDateDesc : FolderListModel.ByName
    }

    MessageDialog {
        id: removeDialog
        title: "Spectacol"
        text: qsTr("Remove \"") + filePath + "\" ?";
        buttons: MessageDialog.Yes | MessageDialog.No
        property string filePath
        onAccepted: {
            ZxEmulator.remove(filePath)
            filesModel.refresh();
            filesView.currentIndex = 0;
        }
        function remove() {
            removeDialog.filePath = filesModel.path(filesView.currentIndex);
            removeDialog.open();
        }
    }

    CoverFlow {
        id : filesView

        anchors.fill: undefined
        anchors { left: parent.left; right: parent.right; top: artist_text_label.bottom; bottom: hintBar.top }
        clip: true
        visible: fileList.viewMode === 0
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
                messagePage.showMessage(ZxEmulator.Info, "\"" + copyPath + qsTr("\" copied to clipboard. Use <b>Paste</b> to paste it to another folder."))
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

        onCopyPressed: copyPath = filesModel.path(filesView.currentIndex)
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
                source : "image://spectrum/" + path
                smooth : false

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
                    onClicked: {
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
        visible: Theme.showHintBar
        height: visible ? implicitHeight : 0
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        hints: [
            { pad: "A", key: "Enter", text: qsTr("Open"), action: () => fileList.openRow(filesModel.currentIndex) },
            { pad: "B", key: "Esc", text: qsTr("Close"), action: () => fileList.fileSelected("") },
            { pad: "X", key: "Del", text: qsTr("Remove"), action: () => removeDialog.remove() },
            { pad: "L1", key: "C", text: qsTr("Copy"), action: () => filesView.copyPath = filesModel.path(filesModel.currentIndex) },
            { pad: "R1", key: "P", text: filesView.canCopy ? qsTr("Paste") : qsTr("Paste (copy first)"), action: () => copyDialog.copy() },
            { pad: "Y", key: "F3", text: qsTr("Search"), action: () => search.forceActiveFocus() },
            { pad: "Start", key: "F2", text: qsTr("View"), action: () => fileList.viewMode = (fileList.viewMode + 1) % 3 },
            { pad: "L2", key: "Home", text: qsTr("Spectacol folder"), action: () => fileList.folder = ZxEmulator.dataPath }
        ]
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
                    text: "‹"
                    font.pixelSize: Theme.pageTitle
                    Accessible.name: qsTr("Close files")
                    onClicked: fileList.fileSelected("")
                }
                Label {
                    Layout.fillWidth: true
                    text: artist_text_label.root ? qsTr("Open a file") : ZxEmulator.fileName(filesModel.folder)
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
                    text: Theme.touchScreenUi ? "↑" : qsTr("Up")
                    Accessible.name: qsTr("Up a folder")
                    Layout.minimumWidth: Theme.rowHeight
                    Layout.minimumHeight: Theme.rowHeight
                    enabled: !artist_text_label.root
                    onClicked: filesModel.cdUp()
                }
                NavigationComboBox {
                    id: modePicker
                    objectName: "filesViewMode"
                    model: [qsTr("Covers"), qsTr("List"), qsTr("Grid")]
                    currentIndex: fileList.viewMode
                    onActivated: fileList.viewMode = currentIndex
                }
                TextField {
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
                text: filesModel.folder
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
