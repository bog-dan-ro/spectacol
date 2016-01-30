/*
    Copyright (c) 2015, BogDan Vatra <bogdan@kde.org>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

import QtQuick 2.0
import QtQuick.Controls 1.4
import QtQuick.Window 2.0
import QtGamepad 1.0

Menu {
    id: fuseMenu
    MenuItem {
        visible: fuse.saveSnapshotEnabled
        text: "Quick &save snapshot"
        onTriggered: {
            menuBar.close();
            fuse.quickSaveSnapshot();
        }
    }
    MenuItem {
        text: "Quick &load snapshot"
        onTriggered: {
            menuBar.close();
            fuse.quickLoadSnapshot();
        }
    }

    Menu {
        title: "&File"
        MenuItem {
            text: "&Open .."
            onTriggered: {
                menuBar.close();
                pageLoader.source = "FileBrowserPage.qml";
            }
        }

        MenuItem {
            text: "&Save snapshot as ..."
            onTriggered: {
                menuBar.close();
                pageLoader.source = "SaveSnapshotPage.qml";
            }
        }

        Menu {
            title: qsTr("Recording")
            visible: false
            MenuItem {
                text: "Record"
                onTriggered: {
                    menuBar.close();
                    pageLoader.source = "StartRecordingPage.qml";
                }
            }
            MenuItem {
                text: "Record from snapshot"
                onTriggered: {
                    menuBar.close();
                    pageLoader.source = "RecordFromSnapshotRecordingPage.qml";
                }
            }
            MenuItem {
                text: "Continue recording"
                onTriggered: {
                    menuBar.close();
                    pageLoader.source = "ContinueRecordingPage.qml";
                }
            }
            MenuItem {
                text: "Insert snapshot"
                onTriggered: {
                    menuBar.close();
                    pageLoader.source = "InsertSnapshotRecordingPage.qml";
                }
            }
            MenuItem {
                text: "Rollback"
                onTriggered: {
                    menuBar.close();
                    pageLoader.source = "RollbackRecordingPage.qml";
                }
            }
            MenuItem {
                text: "Rollback to ..."
                onTriggered: {
                    menuBar.close();
                    pageLoader.source = "RollbackToRecordingPage.qml";
                }
            }
            MenuItem {
                text: "Play"
                onTriggered: {
                    menuBar.close();
                    pageLoader.source = "PlayRecordingPage.qml";
                }
            }
            MenuItem {
                text: "Stop"
                onTriggered: {
                    menuBar.close();
                    pageLoader.source = "StopRecordingPage.qml";
                }
            }
            MenuItem {
                text: "Finalise ..."
                onTriggered: {
                    menuBar.close();
                    pageLoader.source = "FinaliseRecordingPage.qml";
                }
            }
        }
    }

    Menu {
        title: "Joystick"
        visible: GamepadManager.connectedGamepads.length > 0
        MenuItem {
            text: "Joysticks"
            onTriggered: {
                menuBar.close();
                pageLoader.source = "Joysticks.qml";
            }
        }

        MenuItem {
            text: "Calibrate Gamepad"
            onTriggered: {
                menuBar.close();
                pageLoader.source = "CalibrateGamepad.qml";
            }
        }
    }

    Menu {
        title: "Screen"

        MenuItem {
            text: "Filter"
            onTriggered: {
                menuBar.close();
                pageLoader.source = "FiltersPage.qml";
            }
        }
        MenuItem {
            text: "Toggle Full Screen"
            onTriggered: {
                menuBar.close();
                fuseScreen.fullScreen = !fuseScreen.fullScreen;
            }
        }
    }

    Menu {
        title: qsTr("Machine")
        MenuItem {
            text: qsTr("Reset")
            onTriggered: {
                menuBar.close();
                fuse.reset();
            }
        }
        MenuItem {
            text: qsTr("Hard reset")
            onTriggered: {
                menuBar.close();
                fuse.hardReset();
            }
        }
        MenuItem {
            text: qsTr("Debugger")
            onTriggered: {
                menuBar.close();
                fuse.debuggerTrap();
            }
        }
        MenuItem {
            text: qsTr("Poke finder")
            onTriggered: {
                menuBar.close();
                pageLoader.source = "PokeFinderPage.qml";
            }
        }
        MenuItem {
            text: qsTr("Poke memory")
            onTriggered: {
                menuBar.close();
                //pageLoader.source = "PokeMemoryPage.qml";
                fuse.pokeMemory(0,0,0);
            }
        }
        MenuItem {
            text: qsTr("Memory Browser")
            onTriggered: {
                menuBar.close();
                pageLoader.source = "MemoryBrowserPage.qml";
            }
        }

        MenuItem {
            text: qsTr("Memory Browser")
            onTriggered: {
                menuBar.close();
                pageLoader.source = "MemoryBrowserPage.qml";
            }
        }

        Menu {
            visible: false
            title: qsTr("Profiler")
            MenuItem {
                text: qsTr("Start")
                onTriggered: {
                    menuBar.close();
                    fuse.startProfiler();
                }
            }
            MenuItem {
                text: qsTr("Stop")
                onTriggered: {
                    menuBar.close();
                    fuse.stopProfiler();
                }
            }
        }

        MenuItem {
            text: qsTr("NMI")
            onTriggered: {
                menuBar.close();
                fuse.nmi();
            }
        }

        MenuItem {
            visible: false
            text: qsTr("Didaktik SNAP")
            onTriggered: {
                menuBar.close();
                fuse.didaktikSNAP();
            }
        }
    }

    Menu {
        title: qsTr("Options")

        MenuItem {
            text: qsTr("General")
            onTriggered: {
                menuBar.close();
                pageLoader.source = "OptionsGeneralPage.qml";
            }
        }

        MenuItem {
            text: qsTr("Media")
            onTriggered: {
                menuBar.close();
                pageLoader.source = "OptionsMediaPage.qml";
            }
        }

        MenuItem {
            text: qsTr("Sound")
            onTriggered: {
                menuBar.close();
                pageLoader.source = "OptionsSoundPage.qml";
            }
        }

        Menu {
            title: qsTr("Machine")
            MenuItem {
                text: qsTr("Reset")
                onTriggered: {
                    menuBar.close();
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
                    menuBar.close();
                    pageLoader.source = "FileBrowserPage.qml";
                }
            }
            MenuItem {
                text: qsTr("Play")
                onTriggered: {
                    menuBar.close();
                    fuse.playTape();
                }
            }
            MenuItem {
                text: qsTr("Browse")
                onTriggered: {
                    menuBar.close();
                    fuse.browseTape();
                }
            }
            MenuItem {
                text: qsTr("Rewind")
                onTriggered: {
                    menuBar.close();
                    fuse.rewindTape();
                }
            }
            MenuItem {
                text: qsTr("Clear")
                onTriggered: {
                    menuBar.close();
                    fuse.clearTape();
                }
            }
            MenuItem {
                text: qsTr("Write")
                onTriggered: {
                    menuBar.close();
                    fuse.writeTape();
                }
            }
            MenuItem {
                text: qsTr("Record Start")
                onTriggered: {
                    menuBar.close();
                    fuse.recordStartTape();
                }
            }
            MenuItem {
                text: qsTr("Record Stop")
                onTriggered: {
                    menuBar.close();
                    fuse.recordStopTape();
                }
            }
        }
    }

    MenuItem {
        text: "About .."
        onTriggered: {
            menuBar.close();
            pageLoader.source = "AboutPage.qml";
        }
    }

    MenuItem {
        text: "Quit"
        onTriggered: quitDialog.open()
    }
}
