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

import QtGamepadLegacy
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
    readonly property real panelWidth: width
    readonly property real hintBarHeight: hintBar.height
    property alias rootFolder: filesModel.rootFolder
    property alias folder: filesModel.folder
    property alias filterClass: filesModel.filterClass

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

    GamepadKeyNavigation {
        gamepad: Gamepad { deviceId: ZxEmulator.gamepadId }
        buttonXKey: Qt.Key_X
        buttonYKey: Qt.Key_Y
        buttonL1Key: Qt.Key_C
        buttonR1Key: Qt.Key_P
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
        color: "#0d0f13"
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

        anchors.fill: parent
        model : filesModel
        // Follow the model's remembered index for each folder. This is not a binding
        // because tapping a cover, copy and remove assign currentIndex directly.
        Component.onCompleted: currentIndex = filesModel.currentIndex
        Connections {
            target: filesModel
            function onCurrentIndexChanged() { filesView.currentIndex = filesModel.currentIndex; }
        }
        property string copyPath: ""
        onCopyPathChanged: {
            if (copyPath.length)
                messagePage.showMessage(ZxEmulator.Info, "\"" + copyPath + qsTr("\" copied to clipboard. Use <b>Paste (R1)</b> button to paste it to another folder."))
        }
        property bool canCopy: {
            if (!copyPath.length)
                return false;
            var srcPath = ZxEmulator.folderName(copyPath);
            if (ZxEmulator.isFolder(copyPath))
                return fileList.folder !== srcPath && !fileList.folder.startsWith(copyPath);
            return fileList.folder !== srcPath;
        }

        onReturnPressed: {
            filesModel.currentIndex = currentIndex;
            if (model.isDir(currentIndex))
                filesModel.folder = model.path(currentIndex);
            else
                fileSelected(model.path(currentIndex));
        }
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

                width : mainScreen.portrait ? filesView.width * 0.8 : filesView.height * 0.65
                height : width
                fillMode: Image.PreserveAspectFit
                scale : delScale
                z : PathView.onPath ? PathView.delZ : -1
                Behavior on rotAngle {SpringAnimation {spring : 5; damping: 0.7; epsilon: 0.025}}
                Behavior on delScale {SpringAnimation {spring : 5; damping: 1; epsilon: 0.005}}
                onStatusChanged: if (status === Image.Ready) scale_anim.start();
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
                            filesModel.currentIndex = index;
                            if (isDir)
                                filesModel.folder = path
                            else
                                fileSelected(path);
                        } else {
                            filesView.currentIndex = index;
                        }
                    }
                }
            }
        }
        HintBar {
            id: hintBar
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            hints: [
                { pad: "A", key: "Enter", text: qsTr("Open"), action: () => filesView.returnPressed() },
                { pad: "B", key: "Esc", text: qsTr("Close"), action: () => fileList.fileSelected("") },
                { pad: "X", key: "Del", text: qsTr("Remove"), action: () => removeDialog.remove() },
                { pad: "L1", key: "C", text: qsTr("Copy"), action: () => filesView.copyPath = filesModel.path(filesView.currentIndex) },
                { pad: "R1", key: "P", text: filesView.canCopy ? qsTr("Paste") : qsTr("Paste (copy first)"), action: () => copyDialog.copy() },
                { pad: "Y", key: "Y", text: qsTr("Spectacol folder"), action: () => fileList.folder = ZxEmulator.dataPath }
            ]
        }
    }
    // Where we are; a click (or Up) goes to the parent folder, or closes at the top
    Rectangle {
        id : artist_text_label
        property bool root: filesModel.rootFolder === filesModel.folder
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: headerColumn.implicitHeight + Theme.sp(8)
        color: Theme.surface

        Column {
            id: headerColumn
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.leftMargin: Theme.sp(6)
            anchors.rightMargin: Theme.sp(6)
            spacing: Theme.sp(1)
            Text {
                text: (artist_text_label.root ? qsTr("Open a file") : qsTr("‹ Up a folder")).toUpperCase()
                font.pixelSize: Theme.caption
                font.letterSpacing: Theme.uiScale
                color: Theme.textMuted
            }
            Text {
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
        MouseArea {
            anchors.fill: parent
            onClicked: {
                filesModel.currentIndex = filesView.currentIndex;
                if (artist_text_label.root)
                    fileSelected("");
                else
                    filesModel.cdUp();
            }
        }
        Button {
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: Theme.sp(3)
            visible: Qt.platform.os === "android"
            text: qsTr("Device files…")
            onClicked: deviceFile.open()
        }
    }

    ShaderEffect {
        smooth: true
        property variant source : ShaderEffectSource {
            sourceItem: filesView
            live : true
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
