// @scope main.qml
import QtQuick 2.5
import QtQuick.Layouts 1.1
import QtQuick.Window 2.0

ListView {
    id: view
    clip: true

    snapMode: ListView.SnapToItem
    highlightFollowsCurrentItem: true

    currentIndex: disassambleModel.delta

    Keys.onUpPressed: {
        if (view.currentIndex > 1)
            decrementCurrentIndex();
        else {
            fuse.disassambleFetchUp(10);
            decrementCurrentIndex();
        }
    }

    Keys.onDownPressed: incrementCurrentIndex()

    model: disassambleModel

    delegate: Rectangle {
        property color paper: view.currentIndex !== index ? background : selectedBackground
        property color ink: view.currentIndex !== index ? foreground : selectedForeground

        width: view.width
        height: 7 * Screen.pixelDensity
        color: paper
        RowLayout {
            anchors.fill: parent
            FancyText {
                Layout.fillHeight: true
                Layout.fillWidth: false
                horizontalAlignment: Text.AlignRight
                fontSize: 4
                style: Text.Normal
                Layout.preferredWidth: 14 * Screen.pixelDensity
                color: ink
                text: model.addressText
            }

            Item { Layout.fillWidth: false;width: 1.5 * Screen.pixelDensity }

            FancyText {
                Layout.fillHeight: true
                Layout.fillWidth: false
                Layout.preferredWidth: 15 * Screen.pixelDensity
                horizontalAlignment: Text.AlignHCenter
                fontSize: 2.5
                style: Text.Normal
                color: ink
                wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                text: model.bytes
            }

            Item { Layout.fillWidth: false;width: 1.5 * Screen.pixelDensity }

            FancyText {
                Layout.fillHeight: true
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignLeft
                fontSize: 4
                color: ink
                style: Text.Normal
                text: model.disassable
            }
        }
        MouseArea {
            anchors.fill: parent
            onClicked: view.currentIndex = model.index
        }
    }
}
