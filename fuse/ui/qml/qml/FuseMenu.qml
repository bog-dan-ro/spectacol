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
import Fuse 1.0

Menu {
    id: fuseMenu
    MenuItem {
        visible: fuse.saveSnapshotEnabled
        text: qsTr("Quick &save snapshot")
        onTriggered: {
            menuBar.close();
            fuse.quickSaveSnapshot();
        }
    }
    MenuItem {
        text: qsTr("Quick &load snapshot")
        onTriggered: {
            menuBar.close();
            fuse.quickLoadSnapshot();
        }
    }

    MenuItem {
        visible: fuse.recording.type !== FuseRecording.None
        text: qsTr("Stop ") + (fuse.recording.type === FuseRecording.Playback ? qsTr("Playback") : qsTr("Recording"))
        onTriggered: {
            menuBar.close();
            fuse.recording.stop();
        }
    }

    Menu {
        title: qsTr("Media")
        MenuItem {
            text: qsTr("&Quick Open ..")
            onTriggered: {
                menuBar.close();
                pageLoader.source = "FileBrowserPage.qml";
            }
        }

        MenuItem {
            visible: false
            text: qsTr("&Save snapshot as ...")
            onTriggered: {
                menuBar.close();
                pageLoader.source = "SaveSnapshotPage.qml";
            }
        }

        MenuItem {
            text: qsTr("&Search online ..")
            onTriggered: {
                menuBar.close();
                pageLoader.source = "SearchOnlinePage.qml";
            }
        }

        Menu {
            title: qsTr("Tape")
            MenuItem {
                text: "Open"
                onTriggered: {
                    menuBar.close();
                    pageLoader.source = "TapesFileBrowserPage.qml";
                }
            }

            MenuItem {
                text: qsTr("Play")
                onTriggered: {
                    menuBar.close();
                    fuse.tape.togglePlay();
                }
            }

            MenuItem {
                text: qsTr("Rewind")
                onTriggered: {
                    menuBar.close();
                    fuse.tape.rewind();
                }
            }

            MenuItem {
                text: qsTr("Clear")
                onTriggered: {
                    menuBar.close();
                    fuse.tape.clear();
                }
            }

            MenuItem {
                visible: fuse.tape.hasTape;
                text: qsTr("Browse")
                onTriggered: {
                    menuBar.close();
                    pageLoader.source = "BrowseTapePage.qml";
                }
            }
        }

        Menu {
            title: qsTr("Recording")
            MenuItem {
                text: "Start recording"
                onTriggered: {
                    menuBar.close();
                    fuse.recording.start();
                }
            }
            MenuItem {
                text: "Continue recording"
                onTriggered: {
                    menuBar.close();
                    fuse.recording.continueRecording();
                }
            }
            MenuItem {
                visible: fuse.recording.type === FuseRecording.Recording
                text: "Rollback"
                onTriggered: {
                    menuBar.close();
                    fuse.recording.rollback();
                }
            }
            MenuItem {
                visible: fuse.recording.type === FuseRecording.Recording
                text: "Rollback to ..."
                onTriggered: {
                    menuBar.close();
                    fuse.recording.rollbackTo();
                }
            }
            MenuItem {
                text: "Finalise ..."
                onTriggered: {
                    menuBar.close();
                    fuse.recording.finalise();
                }
            }
        }
    }


    MenuItem {
        text: "Joystick type"
        onTriggered: {
            menuBar.close();
            pageLoader.source = "Joysticks.qml";
        }
    }

    Menu {
        title: qsTr("Machine")
        MenuItem {
            text: qsTr("Select ... ") + fuseSettings.currentMachine
            onTriggered: {
                menuBar.close();
                pageLoader.source = "MachineTypePage.qml";
            }
        }

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
                pageLoader.source = "PokeManagerPage.qml";
            }
        }

        MenuItem {
            text: qsTr("NMI")
            onTriggered: {
                menuBar.close();
                fuse.nmi();
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
            text: qsTr("Screen")
            onTriggered: {
                menuBar.close();
                pageLoader.source = "OptionsScreenPage.qml";
            }
        }

        MenuItem {
            text: qsTr("Sound")
            onTriggered: {
                menuBar.close();
                pageLoader.source = "OptionsSoundPage.qml";
            }
        }

        MenuItem {
            text: qsTr("Peripherals")
            onTriggered: {
                menuBar.close();
                pageLoader.source = "OptionsPeripheralsPage.qml";
            }
        }

        MenuItem {
            visible: false
            text: qsTr("Media")
            onTriggered: {
                menuBar.close();
                pageLoader.source = "OptionsMediaPage.qml";
            }
        }
        Menu {
            title: qsTr("(Virtual) Gamepad")
            MenuItem {
                text: qsTr("Customize Gamepad Buttons")
                onTriggered: {
                    menuBar.close();
                    pageLoader.source = "CustomizeGamepadPage.qml";
                }
            }

            MenuItem {
                visible: fuse.gamepadId != -1
                text: qsTr("Calibrate Gamepad")
                onTriggered: {
                    menuBar.close();
                    pageLoader.source = "CalibrateGamepad.qml";
                }
            }
        }
    }

    Menu {
        title: qsTr("Help")

        MenuItem {
            text: "About .."
            onTriggered: {
                menuBar.close();
                pageLoader.source = "AboutPage.qml";
            }
        }

        MenuItem {
            text: "Features and bugs"
            onTriggered:  {
                menuBar.close();
                Qt.openUrlExternally("https://github.com/bog-dan-ro/spectacol/issues");
            }
        }
    }


    MenuItem {
        text: "Quit"
        onTriggered: quitDialog.open()
    }
}
