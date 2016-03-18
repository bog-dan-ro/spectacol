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

import QtQuick 2.5
import QtQuick.Layouts 1.1
import "private" 1.0

GridLayout {
    columns: 4
    FancyText {
        font.pixelSize: TextSizes.scale12
        horizontalAlignment: Text.AlignRight
        Layout.fillHeight: false
        Layout.fillWidth: true
        text: "PC"
    }
    FancyText {
        font.pixelSize: TextSizes.scale12
        Layout.fillHeight: false
        Layout.fillWidth: false
        text: fuse.PC
    }
    FancyText {
        font.pixelSize: TextSizes.scale12
        horizontalAlignment: Text.AlignRight
        Layout.fillHeight: false
        Layout.fillWidth: true
        text: "SP"
    }
    FancyText {
        font.pixelSize: TextSizes.scale12
        Layout.fillHeight: false
        Layout.fillWidth: false
        text: fuse.SP
    }

    FancyText {
        font.pixelSize: TextSizes.scale12
        horizontalAlignment: Text.AlignRight
        Layout.fillHeight: false
        Layout.fillWidth: true
        text: "AF"
    }
    FancyText {
        font.pixelSize: TextSizes.scale12
        Layout.fillHeight: false
        Layout.fillWidth: false
        text: fuse.AF
    }
    FancyText {
        font.pixelSize: TextSizes.scale12
        horizontalAlignment: Text.AlignRight
        Layout.fillHeight: false
        Layout.fillWidth: true
        text: "AF'"
    }
    FancyText {
        font.pixelSize: TextSizes.scale12
        Layout.fillHeight: false
        Layout.fillWidth: false
        text: fuse.AF_
    }

    FancyText {
        font.pixelSize: TextSizes.scale12
        horizontalAlignment: Text.AlignRight
        Layout.fillHeight: false
        Layout.fillWidth: true
        text: "BC"
    }
    FancyText {
        font.pixelSize: TextSizes.scale12
        Layout.fillHeight: false
        Layout.fillWidth: false
        text: fuse.BC
    }
    FancyText {
        font.pixelSize: TextSizes.scale12
        horizontalAlignment: Text.AlignRight
        Layout.fillHeight: false
        Layout.fillWidth: true
        text: "BC'"
    }
    FancyText {
        font.pixelSize: TextSizes.scale12
        Layout.fillHeight: false
        Layout.fillWidth: false
        text: fuse.BC_
    }

    FancyText {
        font.pixelSize: TextSizes.scale12
        horizontalAlignment: Text.AlignRight
        Layout.fillHeight: false
        Layout.fillWidth: true
        text: "DE"
    }
    FancyText {
        font.pixelSize: TextSizes.scale12
        Layout.fillHeight: false
        Layout.fillWidth: false
        text: fuse.DE
    }
    FancyText {
        font.pixelSize: TextSizes.scale12
        horizontalAlignment: Text.AlignRight
        Layout.fillHeight: false
        Layout.fillWidth: true
        text: "DE'"
    }
    FancyText {
        font.pixelSize: TextSizes.scale12
        Layout.fillHeight: false
        Layout.fillWidth: false
        text: fuse.DE_
    }

    FancyText {
        font.pixelSize: TextSizes.scale12
        horizontalAlignment: Text.AlignRight
        Layout.fillHeight: false
        Layout.fillWidth: true
        text: "HL"
    }
    FancyText {
        font.pixelSize: TextSizes.scale12
        Layout.fillHeight: false
        Layout.fillWidth: false
        text: fuse.HL
    }
    FancyText {
        font.pixelSize: TextSizes.scale12
        horizontalAlignment: Text.AlignRight
        Layout.fillHeight: false
        Layout.fillWidth: true
        text: "HL'"
    }
    FancyText {
        font.pixelSize: TextSizes.scale12
        Layout.fillHeight: false
        Layout.fillWidth: false
        text: fuse.HL_
    }

    FancyText {
        font.pixelSize: TextSizes.scale12
        horizontalAlignment: Text.AlignRight
        Layout.fillHeight: false
        Layout.fillWidth: true
        text: "IX"
    }
    FancyText {
        font.pixelSize: TextSizes.scale12
        Layout.fillHeight: false
        Layout.fillWidth: false
        text: fuse.IX
    }
    FancyText {
        font.pixelSize: TextSizes.scale12
        horizontalAlignment: Text.AlignRight
        Layout.fillHeight: false
        Layout.fillWidth: true
        text: "IY"
    }
    FancyText {
        font.pixelSize: TextSizes.scale12
        Layout.fillHeight: false
        Layout.fillWidth: false
        text: fuse.IY
    }

    FancyText {
        font.pixelSize: TextSizes.scale12
        horizontalAlignment: Text.AlignRight
        Layout.fillHeight: false
        Layout.fillWidth: true
        text: "I"
    }
    FancyText {
        font.pixelSize: TextSizes.scale12
        Layout.fillHeight: false
        Layout.fillWidth: false
        text: fuse.I
    }
    FancyText {
        font.pixelSize: TextSizes.scale12
        horizontalAlignment: Text.AlignRight
        Layout.fillHeight: false
        Layout.fillWidth: true
        text: "R"
    }
    FancyText {
        font.pixelSize: TextSizes.scale12
        Layout.fillHeight: false
        Layout.fillWidth: false
        text: fuse.R
    }

    FancyText {
        font.pixelSize: TextSizes.scale12
        horizontalAlignment: Text.AlignRight
        Layout.fillHeight: false
        Layout.fillWidth: true
        text: "IFF1"
    }
    FancyText {
        font.pixelSize: TextSizes.scale12
        Layout.fillHeight: false
        Layout.fillWidth: false
        text: fuse.IFF1
    }
    FancyText {
        font.pixelSize: TextSizes.scale12
        horizontalAlignment: Text.AlignRight
        Layout.fillHeight: false
        Layout.fillWidth: true
        text: "IFF2"
    }
    FancyText {
        font.pixelSize: TextSizes.scale12
        Layout.fillHeight: false
        Layout.fillWidth: false
        text: fuse.IFF2
    }

    FancyText {
        font.pixelSize: TextSizes.scale12
        horizontalAlignment: Text.AlignRight
        Layout.fillHeight: false
        Layout.fillWidth: true
        text: "R7"
    }
    FancyText {
        font.pixelSize: TextSizes.scale12
        Layout.fillHeight: false
        Layout.fillWidth: false
        text: fuse.R7
    }
    FancyText {
        font.pixelSize: TextSizes.scale12
        horizontalAlignment: Text.AlignRight
        Layout.fillHeight: false
        Layout.fillWidth: true
        text: "IM"
    }
    FancyText {
        font.pixelSize: TextSizes.scale12
        Layout.fillHeight: false
        Layout.fillWidth: false
        text: fuse.IM
    }

    Item {
        Layout.fillHeight: true
        Layout.fillWidth: true
    }
}
