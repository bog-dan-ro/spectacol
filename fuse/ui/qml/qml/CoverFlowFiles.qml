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

import QtQuick 2.2
import QtGamepad 1.0
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.1
import Fuse 1.0
import QtQuick.Controls 2.0
import "private" 1.0

Item
{
    id : fileList
    signal fileSelected(string filePath);
    property alias rootFolder: filesModel.rootFolder
    property alias folder: filesModel.folder

    GamepadKeyNavigation {
        gamepad: Gamepad { deviceId: fuse.gamepadId }
        buttonXKey: Qt.Key_X
    }

    transform: Rotation {
        angle : 0
        axis {x : 0; y: 1; z : 0}
        origin.x : width * 0.5
        origin.y : height * 0.5
    }

    FolderListModel {
        id: filesModel
        rootFolder: fuseSettings.restrictToSpectacol ? fuse.dataPath : "/"
        sortCriteria: folder.indexOf(fuse.snapshotsPath(), 0) == 0 ? FolderListModel.ByDateDesc : FolderListModel.ByName
        onFolderChanged: filesView.currentIndex = 0
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

    CoverFlow
    {
        visible: true
        id : filesView
        anchors.fill: parent
        model : filesModel

        onReturnPressed: {
            if (model.isDir(currentIndex))
                filesModel.folder = model.path(currentIndex);
            else
                fileSelected(model.path(currentIndex));
        }
        onEscapePressed: fileSelected("")

        onUpPressed: filesModel.cdUp()

        onDeletePressed: removeDialog.remove()

        delegate: Component {
            Image
            {
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

                FancyText {
                    font.pixelSize: TextSizes.scale20
                    font.bold: true
                    text : name
                    width : parent.width
                    elide: Text.ElideRight
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                    anchors
                    {
                        top : parent.bottom
                        horizontalCenter : parent.horizontalCenter
                    }
                }
                MouseArea
                {
                    anchors.fill: parent
                    onClicked: {
                        if (isCurrentItem) {
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
                text: qsTr("Close (B)")
                onClicked: pageLoader.source = ""
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
                if (artist_text_label.root)
                    pageLoader.source = "";
                else
                    filesModel.cdUp();
            }
        }
    }

    ShaderEffect
    {
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
