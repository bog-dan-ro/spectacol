import QtQuick 2.6

PathView
{
    signal returnPressed()
    signal upPressed()
    signal escapePressed();
    signal deletePressed();

    anchors.fill: parent
    pathItemCount: mainScreen.portrait ? 5 : 10
    cacheItemCount: pathItemCount * 2
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
