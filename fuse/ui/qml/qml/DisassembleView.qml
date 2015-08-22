// @scope main.qml
import QtQuick 2.5
import QtQuick.Layouts 1.1
import QtQuick.Window 2.0

ListView {
    id: view
    clip: true

    snapMode: ListView.SnapToItem
    highlightFollowsCurrentItem: true
    highlightRangeMode: ListView.ApplyRange

    Keys.onLeftPressed: decrementCurrentIndex()
    Keys.onRightPressed: incrementCurrentIndex()

    model: fuse.disassambleModel()
//    currentIndex: fuse.selectedFilterIndex
//    onCurrentIndexChanged: fuse.selectedFilterIndex = currentIndex

    delegate: Rectangle {
        width: view.width
        height: 7 * Screen.pixelDensity
        color: backgroud
        RowLayout {
            anchors.fill: parent
            FancyText {
                Layout.fillHeight: true
                horizontalAlignment: Text.AlignRight
                fontSize: 4
                color: foreground
                text: addressText
            }
            FancyText {
                Layout.fillHeight: true
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignLeft
                fontSize: 4
                color: foreground
                text: disassable
            }
        }
        MouseArea {
            anchors.fill: parent
            onClicked: view.currentIndex = model.index
        }
    }
}
