import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.1
import QtQuick.Window 2.0
import Fuse 1.0

ApplicationWindow {
    id: mainScreen
    visible: true

    width: 320 * 3
    height: 256 * 3

    title: qsTr("Hello World")

    property int dpiMultiplier: 1
    property bool portrait: false

    NavigationDrawer {
        id: menuBar
        color: Qt.rgba(0, 0, 0, 0.5)
        width: parent.width / 4
        ColumnLayout {
            Button {
                text: "Toggle Full screen"
                onClicked: {
                    if (mainScreen.visibility === Window.FullScreen)
                        mainScreen.visibility = Window.AutomaticVisibility
                    else
                        mainScreen.visibility = Window.FullScreen
                    menuBar.open = false;
                }
            }
            Button {
                text: "Browse files"
                onClicked: {
                    pageLoader.sourceComponent = fileBrowser;
                    menuBar.open = false;
                }
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        color: "black"
        FuseScreen {
            id: fuse
            focus: true
            anchors.fill: parent
            onScreenChanged: mainScreen.visibility = fullScreen ? Window.FullScreen : Window.AutomaticVisibility;

            Loader {
                id: pageLoader
                anchors.fill: parent
                asynchronous: true
                visible: status === Loader.Ready
                focus: visible
                enabled: visible
                onSourceComponentChanged: fuse.paused = (status === Loader.Ready)
            }
        }
    }
    Component {
        id: fileBrowser
        CoverFlowFiles {
            anchors.fill: parent
            onFileSelected: {
                if (filePath)
                    fuse.load(filePath);
                pageLoader.sourceComponent = null
            }
        }
    }
}
