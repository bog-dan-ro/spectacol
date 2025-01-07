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
import QtQuick.Layouts
import QtQuick.Window
import Spectacol
import "private"

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
            text: "PC :" + FuseEmulator.PC
        }
        FancyText {
            font.pixelSize: TextSizes.scale12
            text: "SP :" + FuseEmulator.SP
        }
        FancyText {
            font.pixelSize: TextSizes.scale12
            text: "AF :" + FuseEmulator.AF
        }
        FancyText {
            font.pixelSize: TextSizes.scale12
            text: "AF' :" + FuseEmulator.AF_
        }
        FancyText {
            font.pixelSize: TextSizes.scale12
            text: "BC :" + FuseEmulator.BC
        }
        FancyText {
            font.pixelSize: TextSizes.scale12
            text: "BC' :" + FuseEmulator.BC_
        }
        FancyText {
            font.pixelSize: TextSizes.scale12
            text: "DE :" + FuseEmulator.DE
        }
        FancyText {
            font.pixelSize: TextSizes.scale12
            text: "DE' :" + FuseEmulator.DE_
        }
        FancyText {
            font.pixelSize: TextSizes.scale12
            text: "HL :" + FuseEmulator.HL
        }
        FancyText {
            font.pixelSize: TextSizes.scale12
            text: "HL' :" + FuseEmulator.HL_
        }

        FancyText {
            font.pixelSize: TextSizes.scale12
            text: "IX :" + FuseEmulator.IX
        }

        FancyText {
            font.pixelSize: TextSizes.scale12
            text: "IY :" + FuseEmulator.IY
        }
        FancyText {
            font.pixelSize: TextSizes.scale12
            text: "I :" + FuseEmulator.I
        }
        FancyText {
            font.pixelSize: TextSizes.scale12
            text: "R :" + FuseEmulator.R
        }
        FancyText {
            font.pixelSize: TextSizes.scale12
            text: "IFF1 :" + FuseEmulator.IFF1
        }
        FancyText {
            font.pixelSize: TextSizes.scale12
            text: "IFF2 :" + FuseEmulator.IFF2
        }
        FancyText {
            font.pixelSize: TextSizes.scale12
            text: "R7 :" + FuseEmulator.R7
        }
        FancyText {
            font.pixelSize: TextSizes.scale12
            text: "IM :" + FuseEmulator.IM
        }
    }
}
