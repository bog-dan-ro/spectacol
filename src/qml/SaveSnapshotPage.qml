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

// @scope main.qml

import QtQuick
import QtCore
import Spectacol

SaveFilePage {
    id: saveSnapshotPage

    title: qsTr("Save snapshot")
    fileName: ZxEmulator.snapshotFileName()
    formats: ZxEmulator.snapshotFormats()
    folder: ZxEmulator.dataPath + "/Snapshots/"
    destinationPath: selectedPath || ZxEmulator.snapshotFilePath(folder,
        fileName.replace(/\.(szx|sna|z80|nex)$/i, "") + "." + selectedFormat)
    Component.onCompleted: {
        if (Qt.platform.os === "android" && folder !== ZxEmulator.dataPath
                && !folder.startsWith(ZxEmulator.dataPath + "/"))
            folder = ZxEmulator.snapshotsPath();
    }
    onAccepted: {
        if (ZxEmulator.save(destinationPath))
            pageLoader.source = "";
    }

    onCanceled: pageLoader.source = "";

    Settings {
        category: "snapshot"
        property alias folder: saveSnapshotPage.folder
    }
}
