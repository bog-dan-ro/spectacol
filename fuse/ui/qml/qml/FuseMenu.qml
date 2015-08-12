import QtQuick 2.0
import QtQuick.Controls 1.4
import QtQuick.Window 2.0

Menu {
    id: fuseMenu
    MenuItem {
        visible: fuse.saveSnapshotEnabled
        text: "Quick save snapshot"
        onTriggered: {
            menuBar.open = false;
            fuse.quickSaveSnapshot();
        }
    }
    MenuItem {
        text: "Quick load snapshot"
        onTriggered: {
            menuBar.open = false;
            fuse.quickLoadSnapshot();
        }
    }

    Menu {
        title: "&File"
        MenuItem {
            text: "Open .."
            onTriggered: {
                menuBar.open = false;
                pageLoader.source = "FileBrowserPage.qml";
            }
        }
        MenuItem {
            text: "Save snapshot as ..."
            onTriggered: {
                menuBar.open = false;
                pageLoader.source = "SaveSnapshotPage.qml";
            }
        }
    }

    Menu {
        title: "Screen"
        MenuItem {
            text: "Toggle Full Screen"
            onTriggered: {
                menuBar.open = false;
                fuse.fullScreen = !fuse.fullScreen;
            }
        }
    }

    MenuItem {
        text: "About .."
        onTriggered: {
            menuBar.open = false;
//            pageLoader.source = "AboutPage.qml";
        }
    }

    MenuItem {
        text: "Quit"
        onTriggered: quitDialog.open()
    }
}
