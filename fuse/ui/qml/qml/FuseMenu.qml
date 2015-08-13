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

        Menu {
            title: qsTr("Recording")
            MenuItem {
                text: "Record"
                onTriggered: {
                    menuBar.open = false;
                    pageLoader.source = "StartRecordingPage.qml";
                }
            }
            MenuItem {
                text: "Record from snapshot"
                onTriggered: {
                    menuBar.open = false;
                    pageLoader.source = "StartRecordingPage.qml";
                }
            }
            MenuItem {
                text: "Continue recording"
                onTriggered: {
                    menuBar.open = false;
                    pageLoader.source = "StartRecordingPage.qml";
                }
            }
            MenuItem {
                text: "Insert snapshot"
                onTriggered: {
                    menuBar.open = false;
                    pageLoader.source = "StartRecordingPage.qml";
                }
            }
            MenuItem {
                text: "Rollback"
                onTriggered: {
                    menuBar.open = false;
                    pageLoader.source = "StartRecordingPage.qml";
                }
            }
            MenuItem {
                text: "Rollback to ..."
                onTriggered: {
                    menuBar.open = false;
                    pageLoader.source = "StartRecordingPage.qml";
                }
            }
            MenuItem {
                text: "Play"
                onTriggered: {
                    menuBar.open = false;
                    pageLoader.source = "StartRecordingPage.qml";
                }
            }
            MenuItem {
                text: "Stop"
                onTriggered: {
                    menuBar.open = false;
                    pageLoader.source = "StartRecordingPage.qml";
                }
            }
            MenuItem {
                text: "Finalise ..."
                onTriggered: {
                    menuBar.open = false;
                    pageLoader.source = "StartRecordingPage.qml";
                }
            }
        }
    }
    Menu {
        title: qsTr("Input")
    }

    Menu {
        title: "Screen"

        MenuItem {
            text: "Filter"
            onTriggered: {
                menuBar.open = false;
                pageLoader.source = "FiltersPage.qml";
            }
        }
        MenuItem {
            text: "Toggle Full Screen"
            onTriggered: {
                menuBar.open = false;
                fuse.fullScreen = !fuse.fullScreen;
            }
        }
    }

    Menu {
        title: "Advanced"
        MenuItem {
            text: "Debugger"
            onTriggered: {
                menuBar.open = false;
                pageLoader.source = "DebuggerPage.qml";
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
