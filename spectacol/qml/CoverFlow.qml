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

import QtQuick 2.12

PathView
{
    signal returnPressed()
    signal upPressed()
    signal escapePressed();
    signal deletePressed();
    signal resetPressed();
    signal copyPressed();
    signal pastePressed();

    anchors.fill: parent
    pathItemCount: mainScreen.portrait ? 5 : 7
    snapMode : PathView.SnapToItem
    preferredHighlightBegin : 0.5
    preferredHighlightEnd : 0.5
    highlightRangeMode : PathView.StrictlyEnforceRange
    focus: true

    Keys.onPressed: {
        switch (event.key) {
        case Qt.Key_Left:
            decrementCurrentIndex();
            event.accepted = true;
            break;

        case Qt.Key_Right:
            incrementCurrentIndex();
            event.accepted = true;
            break;

        case Qt.Key_Enter:
        case Qt.Key_Return:
            returnPressed();
            event.accepted = true;
            break;

        case Qt.Key_Up:
            upPressed();
            event.accepted = true;
            break;

        case Qt.Key_Escape:
            escapePressed();
            event.accepted = true;
            break;

        case Qt.Key_Delete:
        case Qt.Key_X:
            deletePressed();
            event.accepted = true;
            break;

        case Qt.Key_Y:
            resetPressed();
            event.accepted = true;
            break;

        case Qt.Key_C:
            copyPressed();
            event.accepted = true;
            break;

        case Qt.Key_V:
        case Qt.Key_P:
            pastePressed();
            event.accepted = true;
            break;
        }
    }

    path : Path  {
        // LEFT
        startX : parent.x
        startY: (parent.height * 0.5)
        PathPercent {value : 0}
        PathAttribute {name : "delScale"; value : 0.5}
        PathAttribute {name : "delAngle"; value : 60}
        PathAttribute {name : "delZ"; value :0}

        PathLine {x : parent.width * 0.4 ; y : parent.height * 0.5}
        PathPercent {value : 0.45}
        PathAttribute {name : "delScale"; value : 0.7}
        PathAttribute {name : "delAngle"; value : 30}

//        // CENTER
        PathLine {x : parent.width * 0.5; y : parent.height * 0.5}
        PathPercent {value : 0.5}
        PathAttribute {name : "delScale"; value : 1}
        PathAttribute {name : "delAngle"; value : 0}
        PathAttribute {name : "delZ"; value : 100}

        PathLine {x : parent.width * 0.6 ; y : parent.height * 0.5}
        PathPercent {value : 0.55}
        PathAttribute {name : "delAngle"; value : -30}
        PathAttribute {name : "delScale"; value : 0.7}

//        //RIGHT
        PathLine {x : parent.width; y : parent.height * 0.5}
        PathAttribute {name : "delScale"; value : 0.5}
        PathAttribute {name : "delAngle"; value : -60}
        PathAttribute {name : "delZ"; value : 0}
        PathPercent {value : 1}
    }
}
