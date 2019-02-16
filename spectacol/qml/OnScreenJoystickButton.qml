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
import QtQuick.Window 2.12

import "private"

Rectangle {
    property alias text: label.text
    anchors.bottomMargin: (TextSizes.smallScreen ? 3.5 : 7) * Screen.pixelDensity // 0.7, 1.5 cm
    anchors.right: parent.right
    color: Qt.lighter(border.color, 1.5)
    opacity: 0.65
    width: (TextSizes.smallScreen ? 7 : 15) * Screen.pixelDensity // 0.7, 1.5 cm
    height: width
    radius: width / 2
    FancyText {
        id: label
        anchors.centerIn: parent
        style: Text.Outline
        styleColor: "white"
        color: parent.border.color
        font.bold: true
        font.pixelSize: TextSizes.smallScreen ? TextSizes.scale16 : TextSizes.scale34
    }
}
