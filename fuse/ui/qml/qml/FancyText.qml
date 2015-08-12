import QtQuick 2.5
import QtQuick.Window 2.2

Text {
    property real fontSize : 5
    font.pixelSize: fontSize * Screen.pixelDensity
    style: Text.Outline
    styleColor: "#66ff2200"
    color : "white"
    horizontalAlignment: Text.AlignLeft
    verticalAlignment: Text.AlignVCenter
}
