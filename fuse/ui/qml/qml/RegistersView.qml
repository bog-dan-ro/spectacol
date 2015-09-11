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

GridLayout {
    columns: 4
    FancyText {
        fontSize: 4
        horizontalAlignment: Text.AlignRight
        Layout.fillHeight: false
        Layout.fillWidth: true
        text: "PC"
    }
    FancyText {
        fontSize: 4
        Layout.fillHeight: false
        Layout.fillWidth: false
        font.family: "Monospace"
        text: fuse.PC
    }
    FancyText {
        fontSize: 4
        horizontalAlignment: Text.AlignRight
        Layout.fillHeight: false
        Layout.fillWidth: true
        text: "SP"
    }
    FancyText {
        fontSize: 4
        Layout.fillHeight: false
        Layout.fillWidth: false
        font.family: "Monospace"
        text: fuse.SP
    }

    FancyText {
        fontSize: 4
        horizontalAlignment: Text.AlignRight
        Layout.fillHeight: false
        Layout.fillWidth: true
        text: "AF"
    }
    FancyText {
        fontSize: 4
        Layout.fillHeight: false
        Layout.fillWidth: false
        font.family: "Monospace"
        text: fuse.AF
    }
    FancyText {
        fontSize: 4
        horizontalAlignment: Text.AlignRight
        Layout.fillHeight: false
        Layout.fillWidth: true
        text: "AF'"
    }
    FancyText {
        fontSize: 4
        Layout.fillHeight: false
        Layout.fillWidth: false
        font.family: "Monospace"
        text: fuse.AF_
    }

    FancyText {
        fontSize: 4
        horizontalAlignment: Text.AlignRight
        Layout.fillHeight: false
        Layout.fillWidth: true
        text: "BC"
    }
    FancyText {
        fontSize: 4
        Layout.fillHeight: false
        Layout.fillWidth: false
        font.family: "Monospace"
        text: fuse.BC
    }
    FancyText {
        fontSize: 4
        horizontalAlignment: Text.AlignRight
        Layout.fillHeight: false
        Layout.fillWidth: true
        text: "BC'"
    }
    FancyText {
        fontSize: 4
        Layout.fillHeight: false
        Layout.fillWidth: false
        font.family: "Monospace"
        text: fuse.BC_
    }

    FancyText {
        fontSize: 4
        horizontalAlignment: Text.AlignRight
        Layout.fillHeight: false
        Layout.fillWidth: true
        text: "DE"
    }
    FancyText {
        fontSize: 4
        Layout.fillHeight: false
        Layout.fillWidth: false
        font.family: "Monospace"
        text: fuse.DE
    }
    FancyText {
        fontSize: 4
        horizontalAlignment: Text.AlignRight
        Layout.fillHeight: false
        Layout.fillWidth: true
        text: "DE'"
    }
    FancyText {
        fontSize: 4
        Layout.fillHeight: false
        Layout.fillWidth: false
        font.family: "Monospace"
        text: fuse.DE_
    }

    FancyText {
        fontSize: 4
        horizontalAlignment: Text.AlignRight
        Layout.fillHeight: false
        Layout.fillWidth: true
        text: "HL"
    }
    FancyText {
        fontSize: 4
        Layout.fillHeight: false
        Layout.fillWidth: false
        font.family: "Monospace"
        text: fuse.HL
    }
    FancyText {
        fontSize: 4
        horizontalAlignment: Text.AlignRight
        Layout.fillHeight: false
        Layout.fillWidth: true
        text: "HL'"
    }
    FancyText {
        fontSize: 4
        Layout.fillHeight: false
        Layout.fillWidth: false
        font.family: "Monospace"
        text: fuse.HL_
    }

    FancyText {
        fontSize: 4
        horizontalAlignment: Text.AlignRight
        Layout.fillHeight: false
        Layout.fillWidth: true
        text: "IX"
    }
    FancyText {
        fontSize: 4
        Layout.fillHeight: false
        Layout.fillWidth: false
        font.family: "Monospace"
        text: fuse.IX
    }
    FancyText {
        fontSize: 4
        horizontalAlignment: Text.AlignRight
        Layout.fillHeight: false
        Layout.fillWidth: true
        text: "IY"
    }
    FancyText {
        fontSize: 4
        Layout.fillHeight: false
        Layout.fillWidth: false
        font.family: "Monospace"
        text: fuse.IY
    }

    FancyText {
        fontSize: 4
        horizontalAlignment: Text.AlignRight
        Layout.fillHeight: false
        Layout.fillWidth: true
        text: "I"
    }
    FancyText {
        fontSize: 4
        Layout.fillHeight: false
        Layout.fillWidth: false
        font.family: "Monospace"
        text: fuse.I
    }
    FancyText {
        fontSize: 4
        horizontalAlignment: Text.AlignRight
        Layout.fillHeight: false
        Layout.fillWidth: true
        text: "R"
    }
    FancyText {
        fontSize: 4
        Layout.fillHeight: false
        Layout.fillWidth: false
        font.family: "Monospace"
        text: fuse.R
    }

    FancyText {
        fontSize: 4
        horizontalAlignment: Text.AlignRight
        Layout.fillHeight: false
        Layout.fillWidth: true
        text: "IFF1"
    }
    FancyText {
        fontSize: 4
        Layout.fillHeight: false
        Layout.fillWidth: false
        font.family: "Monospace"
        text: fuse.IFF1
    }
    FancyText {
        fontSize: 4
        horizontalAlignment: Text.AlignRight
        Layout.fillHeight: false
        Layout.fillWidth: true
        text: "IFF2"
    }
    FancyText {
        fontSize: 4
        Layout.fillHeight: false
        Layout.fillWidth: false
        font.family: "Monospace"
        text: fuse.IFF2
    }

    FancyText {
        fontSize: 4
        horizontalAlignment: Text.AlignRight
        Layout.fillHeight: false
        Layout.fillWidth: true
        text: "R7"
    }
    FancyText {
        fontSize: 4
        Layout.fillHeight: false
        Layout.fillWidth: false
        font.family: "Monospace"
        text: fuse.R7
    }
    FancyText {
        fontSize: 4
        horizontalAlignment: Text.AlignRight
        Layout.fillHeight: false
        Layout.fillWidth: true
        text: "IM"
    }
    FancyText {
        fontSize: 4
        Layout.fillHeight: false
        Layout.fillWidth: false
        font.family: "Monospace"
        text: fuse.IM
    }

    Item {
        Layout.fillHeight: true
        Layout.fillWidth: true
    }
}
