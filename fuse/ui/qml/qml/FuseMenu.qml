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
            visible: false
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
                    pageLoader.source = "RecordFromSnapshotRecordingPage.qml";
                }
            }
            MenuItem {
                text: "Continue recording"
                onTriggered: {
                    menuBar.open = false;
                    pageLoader.source = "ContinueRecordingPage.qml";
                }
            }
            MenuItem {
                text: "Insert snapshot"
                onTriggered: {
                    menuBar.open = false;
                    pageLoader.source = "InsertSnapshotRecordingPage.qml";
                }
            }
            MenuItem {
                text: "Rollback"
                onTriggered: {
                    menuBar.open = false;
                    pageLoader.source = "RollbackRecordingPage.qml";
                }
            }
            MenuItem {
                text: "Rollback to ..."
                onTriggered: {
                    menuBar.open = false;
                    pageLoader.source = "RollbackToRecordingPage.qml";
                }
            }
            MenuItem {
                text: "Play"
                onTriggered: {
                    menuBar.open = false;
                    pageLoader.source = "PlayRecordingPage.qml";
                }
            }
            MenuItem {
                text: "Stop"
                onTriggered: {
                    menuBar.open = false;
                    pageLoader.source = "StopRecordingPage.qml";
                }
            }
            MenuItem {
                text: "Finalise ..."
                onTriggered: {
                    menuBar.open = false;
                    pageLoader.source = "FinaliseRecordingPage.qml";
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
                fuseScreen.fullScreen = !fuseScreen.fullScreen;
            }
        }
    }

    Menu {
        title: qsTr("Machine")
        MenuItem {
            text: qsTr("Reset")
            onTriggered: {
                menuBar.open = false;
                fuse.reset();
            }
        }
        MenuItem {
            text: qsTr("Hard reset")
            onTriggered: {
                menuBar.open = false;
                fuse.hardReset();
            }
        }
        MenuItem {
            text: qsTr("Debugger")
            onTriggered: {
                menuBar.open = false;
                fuse.debuggerTrap();
            }
        }
        MenuItem {
            text: qsTr("Poke finder")
            onTriggered: {
                menuBar.open = false;
                pageLoader.source = "PokeFinderPage.qml";
            }
        }
        MenuItem {
            text: qsTr("Poke memory")
            onTriggered: {
                menuBar.open = false;
                pageLoader.source = "PokeMemoryPage.qml";
            }
        }
        MenuItem {
            text: qsTr("Memory Browser")
            onTriggered: {
                menuBar.open = false;
                pageLoader.source = "MemoryBrowserPage.qml";
            }
        }

        MenuItem {
            text: qsTr("Memory Browser")
            onTriggered: {
                menuBar.open = false;
                pageLoader.source = "MemoryBrowserPage.qml";
            }
        }

        Menu {
            visible: false
            title: qsTr("Profiler")
            MenuItem {
                text: qsTr("Start")
                onTriggered: {
                    menuBar.open = false;
                    fuse.startProfiler();
                }
            }
            MenuItem {
                text: qsTr("Stop")
                onTriggered: {
                    menuBar.open = false;
                    fuse.stopProfiler();
                }
            }
        }

        MenuItem {
            text: qsTr("NMI")
            onTriggered: {
                menuBar.open = false;
                fuse.nmi();
            }
        }

        MenuItem {
            visible: false
            text: qsTr("Didaktik SNAP")
            onTriggered: {
                menuBar.open = false;
                fuse.didaktikSNAP();
            }
        }
    }

    Menu {
        title: qsTr("Options")

        MenuItem {
            text: qsTr("General")
            onTriggered: {
                menuBar.open = false;
                pageLoader.source = "OptionsGeneralPage.qml";
            }
        }

        MenuItem {
            text: qsTr("Media")
            onTriggered: {
                menuBar.open = false;
                pageLoader.source = "OptionsMediaPage.qml";
            }
        }

        MenuItem {
            text: qsTr("Sound")
            onTriggered: {
                menuBar.open = false;
                pageLoader.source = "OptionsSoundPage.qml";
            }
        }

        Menu {
            title: qsTr("Machine")
            MenuItem {
                text: qsTr("Reset")
                onTriggered: {
                    menuBar.open = false;
                    fuse.reset();
                }
            }
        }
    }

    Menu {
        title: qsTr("Media")
        Menu {
            title: qsTr("Tape")
            MenuItem {
                text: qsTr("Open")
                onTriggered: {
                    menuBar.open = false;
                    pageLoader.source = "FileBrowserPage.qml";
                }
            }
            MenuItem {
                text: qsTr("Play")
                onTriggered: {
                    menuBar.open = false;
                    fuse.playTape();
                }
            }
            MenuItem {
                text: qsTr("Browse")
                onTriggered: {
                    menuBar.open = false;
                    fuse.browseTape();
                }
            }
            MenuItem {
                text: qsTr("Rewind")
                onTriggered: {
                    menuBar.open = false;
                    fuse.rewindTape();
                }
            }
            MenuItem {
                text: qsTr("Clear")
                onTriggered: {
                    menuBar.open = false;
                    fuse.clearTape();
                }
            }
            MenuItem {
                text: qsTr("Write")
                onTriggered: {
                    menuBar.open = false;
                    fuse.writeTape();
                }
            }
            MenuItem {
                text: qsTr("Record Start")
                onTriggered: {
                    menuBar.open = false;
                    fuse.recordStartTape();
                }
            }
            MenuItem {
                text: qsTr("Record Stop")
                onTriggered: {
                    menuBar.open = false;
                    fuse.recordStopTape();
                }
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
