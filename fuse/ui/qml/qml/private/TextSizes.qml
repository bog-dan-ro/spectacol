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

pragma Singleton

import QtQuick.Window 2.0
import QtQuick.Controls 2.0

Label {
    id: _label
    property bool smallScreen: Math.min(Screen.height, Screen.width) / Screen.pixelDensity < 80
    property double scale12: smallScreen ? _label.font.pixelSize / (80 / (Math.min(Screen.height, Screen.width) / Screen.pixelDensity)) : _label.font.pixelSize
    property double scale14: scale12 * 14 / 12
    property double scale16: scale12 * 16 / 12
    property double scale20: scale12 * 20 / 12
    property double scale24: scale12 * 2 // 24 / 12
    property double scale34: scale12 * 20 / 12
    property double scaleImage: smallScreen ? Screen.devicePixelRatio : 1
}
