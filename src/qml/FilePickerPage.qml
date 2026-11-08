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

import QtQuick
import Spectacol

// Reuse the cover flow's gamepad/touch navigation for every kind of host file.
CoverFlowFiles {
    id: picker
    property string purpose
    property string returnPage: "SdCardPage.qml"
    folder: ZxEmulator.dataPath
    filterClass: FolderListModel.HostFiles
    onFileSelected: (path) => {
        if(path) {
            if(purpose==="sdCopy") ZxEmulator.sdCard.addFile(path);
            else if(purpose==="sdImage") ZxEmulator.sdCard.useImage(path);
            else if(purpose==="uartEsp") ZxEmulator.uartHooks.espScript=path;
            else if(purpose==="uartPi") ZxEmulator.uartHooks.piScript=path;
        }
        pageLoader.source=returnPage;
    }
}
