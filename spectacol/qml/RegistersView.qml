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
import QtQuick.Window
import Spectacol
import "private"

Flickable {
    id: view
    activeFocusOnTab: true
    Keys.onUpPressed: contentY = Math.max(0, contentY - Theme.rowHeight)
    Keys.onDownPressed: contentY = Math.min(Math.max(0, contentHeight - height), contentY + Theme.rowHeight)
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
            mono: true
            font.pixelSize: TextSizes.scale12
            text: "PC :" + ZxEmulator.PC
        }
        FancyText {
            mono: true
            font.pixelSize: TextSizes.scale12
            text: "SP :" + ZxEmulator.SP
        }
        FancyText {
            mono: true
            font.pixelSize: TextSizes.scale12
            text: "AF :" + ZxEmulator.AF
        }
        FancyText {
            mono: true
            font.pixelSize: TextSizes.scale12
            text: "AF' :" + ZxEmulator.AF_
        }
        FancyText {
            mono: true
            font.pixelSize: TextSizes.scale12
            text: "BC :" + ZxEmulator.BC
        }
        FancyText {
            mono: true
            font.pixelSize: TextSizes.scale12
            text: "BC' :" + ZxEmulator.BC_
        }
        FancyText {
            mono: true
            font.pixelSize: TextSizes.scale12
            text: "DE :" + ZxEmulator.DE
        }
        FancyText {
            mono: true
            font.pixelSize: TextSizes.scale12
            text: "DE' :" + ZxEmulator.DE_
        }
        FancyText {
            mono: true
            font.pixelSize: TextSizes.scale12
            text: "HL :" + ZxEmulator.HL
        }
        FancyText {
            mono: true
            font.pixelSize: TextSizes.scale12
            text: "HL' :" + ZxEmulator.HL_
        }

        FancyText {
            mono: true
            font.pixelSize: TextSizes.scale12
            text: "IX :" + ZxEmulator.IX
        }

        FancyText {
            mono: true
            font.pixelSize: TextSizes.scale12
            text: "IY :" + ZxEmulator.IY
        }
        FancyText {
            mono: true
            font.pixelSize: TextSizes.scale12
            text: "I :" + ZxEmulator.I
        }
        FancyText {
            mono: true
            font.pixelSize: TextSizes.scale12
            text: "R :" + ZxEmulator.R
        }
        FancyText {
            mono: true
            font.pixelSize: TextSizes.scale12
            text: "IFF1 :" + ZxEmulator.IFF1
        }
        FancyText {
            mono: true
            font.pixelSize: TextSizes.scale12
            text: "IFF2 :" + ZxEmulator.IFF2
        }
        FancyText {
            mono: true
            font.pixelSize: TextSizes.scale12
            text: "R7 :" + ZxEmulator.R7
        }
        FancyText {
            mono: true
            font.pixelSize: TextSizes.scale12
            text: "IM :" + ZxEmulator.IM
        }
    }
}
