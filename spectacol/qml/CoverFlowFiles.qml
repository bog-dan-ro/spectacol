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

import QtQuick 2.12
import QtGamepad 1.0
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.12
import Fuse 1.0
import QtQuick.Controls 2.12
import "private" 1.0

// @scope main.qml

Item
{
    id : fileList
    signal fileSelected(string filePath);
    property alias rootFolder: filesModel.rootFolder
    property alias folder: filesModel.folder
    property alias filterClass: filesModel.filterClass

    GamepadKeyNavigation {
        gamepad: Gamepad { deviceId: fuse.gamepadId }
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


    MessageDialog {
        id: copyDialog
        icon: StandardIcon.Question
        title: "Spectacol"
        text: qsTr("Override \"") + fuse.fileName(filesView.copyPath) + "\" ?";
        standardButtons: StandardButton.Yes | StandardButton.No
        onYes: {
            fuse.copy(filesView.copyPath, fileList.folder, true);
            filesView.copyPath = "";
            filesModel.refresh();
            filesView.currentIndex = 0;
        }

        function copy() {
            if (!filesView.canCopy)
                return;

            if (!fuse.copy(filesView.copyPath, fileList.folder)) {
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
        rootFolder: fuseSettings.restrictToSpectacol ? fuse.dataPath : "/"
        sortCriteria: folder.indexOf(fuse.snapshotsPath(), 0) == 0 ? FolderListModel.ByDateDesc : folder.indexOf(fuse.recordingsPath(), 0) == 0 ? FolderListModel.ByDateDesc : FolderListModel.ByName
    }

    MessageDialog {
        id: removeDialog
        icon: StandardIcon.Question
        title: "Spectacol"
        text: qsTr("Remove \"") + filePath + "\" ?";
        standardButtons: StandardButton.Yes | StandardButton.No
        property string filePath
        onYes: {
            fuse.remove(filePath)
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
        currentIndex: filesModel.currentIndex
        property string copyPath: ""
        onCopyPathChanged: {
            if (copyPath.length)
                messagePage.showMessage(FuseEmulator.Info, "\"" + copyPath + qsTr("\" copied to clipboard. Use <b>Paste (R1)</b> button to paste it to another folder."))
        }
        property bool canCopy: {
            if (!copyPath.length)
                return false;
            var srcPath = fuse.folderName(copyPath);
            if (fuse.isFolder(copyPath))
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
        onResetPressed: fileList.folder = fuse.dataPath

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
                    height: TextSizes.scale14 * 3
                    anchors {
                        bottom : parent.bottom
                        horizontalCenter : parent.horizontalCenter
                    }
                    color: Qt.rgba(0, 0, 0, 0.75)
                    border.color: "gray"
                    FancyText {
                        anchors.fill: parent
                        font.pixelSize: TextSizes.scale14
                        font.bold: true
                        text : name
                        elide: Text.ElideRight
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
        RowLayout {
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            Button {
                Layout.fillWidth: true
                text: qsTr("Remove (X)")
                onClicked: removeDialog.remove()
            }
            Button {
                Layout.fillWidth: true
                text: qsTr("Copy (L1)")
                onClicked: filesView.copyPath = filesModel.path(filesView.currentIndex)
            }
            Button {
                Layout.fillWidth: true
                enabled: filesView.canCopy
                text: qsTr("Paste (R1)")
                onClicked: copyDialog.copy()
            }
            Button {
                Layout.fillWidth: true
                text: qsTr("Go to <b>Spectrum</b> folder (Y)")
                onClicked: fileList.folder = fuse.dataPath
            }
            Button {
                Layout.fillWidth: true
                text: qsTr("Close (B)")
                onClicked: fileSelected("")
            }
        }
    }
    FancyText {
        id : artist_text_label
        anchors
        {
            horizontalCenter : parent.horizontalCenter
            top : parent.top
            topMargin : TextSizes.scale20 / 2
        }
        background: Rectangle {
            color: "gray"
            opacity: 0.85
        }
        property bool root: filesModel.rootFolder == filesModel.folder
        padding: TextSizes.scale20 / 2
        font.pixelSize: TextSizes.scale20
        text : filesModel.folder + (root ? " X" : "  \u2191")
        font.bold: true
        elide: Text.ElideRight
        wrapMode: Text.WrapAtWordBoundaryOrAnywhere
        width : parent.width
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        MouseArea {
            anchors.fill: parent
            onClicked: {
                filesModel.currentIndex = filesView.currentIndex;
                if (artist_text_label.root)
                    pageLoader.source = "";
                else
                    filesModel.cdUp();
            }
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

        vertexShader: "
                uniform highp mat4 qt_Matrix;
                attribute highp vec4 qt_Vertex;
                attribute highp vec2 qt_MultiTexCoord0;
                varying highp vec2 coord;
                uniform highp float width;

                void main()
                {
                    coord = qt_MultiTexCoord0;
                    gl_Position = qt_Matrix * qt_Vertex;
                }
            "

        fragmentShader: "
                varying highp vec2 coord;
                uniform sampler2D source;
                uniform lowp float qt_Opacity;

                void main()
                {
                    gl_FragColor = texture2D(source, vec2(coord.x, 1.0 - coord.y)) * (0.6 - coord.y) * sin(3.14 * coord.x);
                }
            "
    }
}
