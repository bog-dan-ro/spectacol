/*
    Copyright (c) 2015-2025, BogDan Vatra <bogdan@kde.org>

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

Item {
    id: page
    anchors.fill: parent

    // With a single drive there is nothing to choose
    property int drive: ZxEmulator.disks.count > 1 ? -1 : 0

    DrivesList {
        visible: page.drive < 0
        focus: visible
        title: qsTr("Insert a disk into")
        onDriveSelected: (drive) => page.drive = drive
    }

    Loader {
        anchors.fill: parent
        active: page.drive >= 0
        focus: active
        sourceComponent: CoverFlowFiles {
            id: browser
            folder: ZxEmulator.dataPath
            filterClass: FolderListModel.Disks

            Settings {
                category: "DiskFileBrowser"
                property alias path: browser.folder
            }

            onFileSelected: (filePath) => {
                if (filePath)
                    ZxEmulator.disks.insertDisk(page.drive, filePath);
                pageLoader.source = "";
            }
        }
    }
}
