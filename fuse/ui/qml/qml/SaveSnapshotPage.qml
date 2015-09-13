/*
    Copyright (c) 2015, BogDan Vatra <bogdan@kde.org>

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

import QtQuick 2.5
import QtQuick.Controls 1.2
import QtQuick.Layouts 1.1
import Qt.labs.settings 1.0

SaveFilePage {
    id: saveSnapshotPage
    Component.onCompleted: fuse.paused = true
    Component.onDestruction: fuse.paused = false

    title: qsTr("Snapshot name:")
    folder: fuse.dataPath + "/Snapshots/"
    onAccpeted: {
        pageLoader.source = "";
    }
    onCanceled: pageLoader.source = "";

    Settings {
        category: "snapshot"
        property alias folder: saveSnapshotPage.folder
    }
}
