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

import QtQuick
import QtQuick.Window
import QtQuick.Controls
import "private"

Label {
    font.pixelSize: TextSizes.smallScreen ? TextSizes.scale12 : TextSizes.scale14
    font.family: Qt.platform.os === 'android' ? 'Droid Sans Mono' : 'Monospace'
    style: Text.Outline
    styleColor: "#66ff2200"
    horizontalAlignment: Text.AlignLeft
    verticalAlignment: Text.AlignVCenter
}
