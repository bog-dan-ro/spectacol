import QtQuick 2.5
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import QtQuick.Window 2.0

TextField {
    property real fontSize : 5

    focus: true
    font.pixelSize: fontSize * Screen.pixelDensity
    font.bold: true
    horizontalAlignment: Text.AlignLeft
    verticalAlignment: Text.AlignVCenter
    inputMethodHints: Qt.ImhNoPredictiveText
//    style: TextFieldStyle {
//        textColor: "black"
//        background: Rectangle {
//            radius: 2
//            implicitWidth: 100
//            implicitHeight: 24
//            border.color: "#333"
//            border.width: 1
//        }
//    }
}
