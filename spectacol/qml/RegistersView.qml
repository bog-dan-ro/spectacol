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

import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Window 2.12
import "private" 1.0

Flickable {
    anchors.margins: Screen.pixelDensity * 3
    contentHeight: grid.height
    contentWidth: grid.width
    clip: true
    Grid {
        id: grid
        columns: 2
        columnSpacing: Screen.pixelDensity * 3
        rowSpacing: Screen.pixelDensity
        horizontalItemAlignment: Grid.AlignRight
        FancyText {
            font.pixelSize: TextSizes.scale12
            text: "PC :" + fuse.PC
        }
        FancyText {
            font.pixelSize: TextSizes.scale12
            text: "SP :" + fuse.SP
        }
        FancyText {
            font.pixelSize: TextSizes.scale12
            text: "AF :" + fuse.AF
        }
        FancyText {
            font.pixelSize: TextSizes.scale12
            text: "AF' :" + fuse.AF_
        }
        FancyText {
            font.pixelSize: TextSizes.scale12
            text: "BC :" + fuse.BC
        }
        FancyText {
            font.pixelSize: TextSizes.scale12
            text: "BC' :" + fuse.BC_
        }
        FancyText {
            font.pixelSize: TextSizes.scale12
            text: "DE :" + fuse.DE
        }
        FancyText {
            font.pixelSize: TextSizes.scale12
            text: "DE' :" + fuse.DE_
        }
        FancyText {
            font.pixelSize: TextSizes.scale12
            text: "HL :" + fuse.HL
        }
        FancyText {
            font.pixelSize: TextSizes.scale12
            text: "HL' :" + fuse.HL_
        }

        FancyText {
            font.pixelSize: TextSizes.scale12
            text: "IX :" + fuse.IX
        }

        FancyText {
            font.pixelSize: TextSizes.scale12
            text: "IY :" + fuse.IY
        }
        FancyText {
            font.pixelSize: TextSizes.scale12
            text: "I :" + fuse.I
        }
        FancyText {
            font.pixelSize: TextSizes.scale12
            text: "R :" + fuse.R
        }
        FancyText {
            font.pixelSize: TextSizes.scale12
            text: "IFF1 :" + fuse.IFF1
        }
        FancyText {
            font.pixelSize: TextSizes.scale12
            text: "IFF2 :" + fuse.IFF2
        }
        FancyText {
            font.pixelSize: TextSizes.scale12
            text: "R7 :" + fuse.R7
        }
        FancyText {
            font.pixelSize: TextSizes.scale12
            text: "IM :" + fuse.IM
        }
    }
}
