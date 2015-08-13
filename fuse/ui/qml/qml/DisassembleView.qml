// @scope main.qml
import QtQuick 2.5
import QtQuick.Window 2.0

ListView {
    id: view
    clip: true
    focus: true
    PropertyAnimation on x {
        to: (pageLoader.width - 50 * Screen.pixelDensity) / 2
        easing.type: Easing.InOutBack
        easing.amplitude: 2.0
        easing.period: 1.5
        duration: 500
    }

    width: pageLoader.width - 50 * Screen.pixelDensity
    anchors.top: pageLoader.top
    anchors.bottom: pageLoader.bottom

    snapMode: ListView.SnapToItem
    highlightFollowsCurrentItem: true
    highlightRangeMode: ListView.ApplyRange

    Keys.onLeftPressed: decrementCurrentIndex()
    Keys.onRightPressed: incrementCurrentIndex()
    Keys.onEscapePressed: pageLoader.source = ""
    Keys.onReturnPressed: pageLoader.source = ""

    model: fuse.filtersModel
    currentIndex: fuse.selectedFilterIndex
    onCurrentIndexChanged: fuse.selectedFilterIndex = currentIndex

    highlight: Rectangle {
        width: 50 * Screen.pixelDensity
        color: "lightgreen"
        border.width: 0.5 * Screen.pixelDensity
        border.color: "green"
    }

    delegate: Rectangle {
        width: 50 * Screen.pixelDensity
        height: 7 * Screen.pixelDensity
        color: Qt.rgba(0, 0, 0, 0.75);

        FancyText {
            anchors.fill: parent
            anchors.leftMargin: 10 * Screen.pixelDensity
            fontSize: 4
            text: modelData
        }
        MouseArea {
            anchors.fill: parent
            onClicked: view.currentIndex = model.index
        }
    }

    footer: Button {
        text: qsTr("Close")
        onClicked: pageLoader.source = ""
    }
}
