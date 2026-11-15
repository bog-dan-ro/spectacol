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

// One of Spectacol's icons (images/NAME.svg), rasterised at the size it is shown. The UI draws
// its symbols with these, not with font characters: Android's fonts lack many of them.
// An Item around the Image: an Image's implicit size is its picture's, it can't have a default.
Item {
    id: icon
    property string name
    implicitWidth: Theme.sp(6)
    implicitHeight: Theme.sp(6)
    Accessible.ignored: true
    Image {
        anchors.fill: parent
        source: icon.name ? "qrc:/images/" + icon.name + ".svg" : ""
        sourceSize: Qt.size(width, height)
        fillMode: Image.PreserveAspectFit
    }
}
