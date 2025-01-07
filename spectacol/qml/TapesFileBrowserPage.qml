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

CoverFlowFiles {
    id: browser
    Component.onCompleted: FuseEmulator.paused = true
    Component.onDestruction: FuseEmulator.paused = false

    folder: FuseEmulator.dataPath
    filterClass: FolderListModel.Tapes

    Settings {
        category: "TapeFileBrowser"
        property alias path: browser.folder
    }

    onFileSelected: {
        if (filePath)
            FuseEmulator.tape.open(filePath);
        pageLoader.source = "";
    }
}
