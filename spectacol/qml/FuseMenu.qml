/*
    Copyright (c) 2015-2025, BogDan Vatra <bogdan@kde.org>

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

import QtQuick
import QtQuick.Controls
import QtQuick.Window
import QtGamepadLegacy
import Spectacol

ZxMenuItem {
    id: fuseMenu
    ZxMenuItem {
        visible: FuseEmulator.saveSnapshotEnabled
        text: qsTr("Quick &save snapshot")
        onTriggered: {
            menuBar.close();
            FuseEmulator.quickSaveSnapshot();
        }
    }
    ZxMenuItem {
        text: qsTr("Quick &load snapshot")
        onTriggered: {
            menuBar.close();
            FuseEmulator.quickLoadSnapshot();
        }
    }

    ZxMenuItem {
        visible: FuseEmulator.recording.type !== FuseRecording.None
        text: qsTr("Stop ") + (FuseEmulator.recording.type === FuseRecording.Playback ? qsTr("Playback") : qsTr("Recording"))
        onTriggered: {
            menuBar.close();
            FuseEmulator.recording.stop();
        }
    }

    ZxMenuItem {
        text: qsTr("&Quick Open ..")
        onTriggered: {
            menuBar.close();
            pageLoader.source = "FileBrowserPage.qml";
        }
    }

    ZxMenuItem {
        text: qsTr("&Search online ..")
        onTriggered: {
            menuBar.close();
            pageLoader.source = "SearchOnlinePage.qml";
        }
    }

    ZxMenuItem {
        text: qsTr("Media")

        ZxMenuItem {
            visible: false
            text: qsTr("&Save snapshot as ...")
            onTriggered: {
                menuBar.close();
                pageLoader.source = "SaveSnapshotPage.qml";
            }
        }

        ZxMenuItem {
            text: qsTr("Tape")
            ZxMenuItem {
                text: "Open"
                onTriggered: {
                    menuBar.close();
                    pageLoader.source = "TapesFileBrowserPage.qml";
                }
            }

            ZxMenuItem {
                text: "Save"
                onTriggered: {
                    menuBar.close();
                    pageLoader.source = "SaveTapePage.qml";
                }
            }

            ZxMenuItem {
                text: qsTr("Play")
                onTriggered: {
                    menuBar.close();
                    FuseEmulator.tape.togglePlay();
                }
            }

            ZxMenuItem {
                text: qsTr("Rewind")
                onTriggered: {
                    menuBar.close();
                    FuseEmulator.tape.rewind();
                }
            }

            ZxMenuItem {
                text: qsTr("Clear")
                onTriggered: {
                    menuBar.close();
                    FuseEmulator.tape.clear();
                }
            }

            ZxMenuItem {
                visible: FuseEmulator.tape.hasTape;
                text: qsTr("Browse")
                onTriggered: {
                    menuBar.close();
                    pageLoader.source = "BrowseTapePage.qml";
                }
            }
        }

        ZxMenuItem {
            text: qsTr("Recording")
            ZxMenuItem {
                visible: FuseEmulator.recording.type === FuseRecording.Recording
                text: qsTr("Stop Recording")
                onTriggered: {
                    menuBar.close();
                    FuseEmulator.recording.stop();
                }
            }
            ZxMenuItem {
                visible: FuseEmulator.recording.type === FuseRecording.None
                text: "Start recording"
                onTriggered: {
                    menuBar.close();
                    FuseEmulator.recording.start();
                }
            }
            ZxMenuItem {
                visible: FuseEmulator.recording.type === FuseRecording.None
                text: "Continue recording"
                onTriggered: {
                    menuBar.close();
                    FuseEmulator.recording.continueRecording();
                }
            }
            ZxMenuItem {
                visible: FuseEmulator.recording.type === FuseRecording.Recording
                text: "Rollback"
                onTriggered: {
                    menuBar.close();
                    FuseEmulator.recording.rollback();
                }
            }
            ZxMenuItem {
                visible: FuseEmulator.recording.type === FuseRecording.Recording
                text: "Rollback to ..."
                onTriggered: {
                    menuBar.close();
                    FuseEmulator.recording.rollbackTo();
                }
            }
            ZxMenuItem {
                visible: FuseEmulator.recording.type === FuseRecording.None
                text: "Finalise ..."
                onTriggered: {
                    menuBar.close();
                    FuseEmulator.recording.finalise();
                }
            }
        }
    }


    ZxMenuItem {
        text: "Joystick type"
        onTriggered: {
            menuBar.close();
            pageLoader.source = "Joysticks.qml";
        }
    }

    ZxMenuItem {
        text: qsTr("Machine")
        ZxMenuItem {
            text: qsTr("Select ... ") + FuseEmulator.settings.currentMachine
            onTriggered: {
                menuBar.close();
                pageLoader.source = "MachineTypePage.qml";
            }
        }

        ZxMenuItem {
            text: qsTr("Reset")
            onTriggered: {
                menuBar.close();
                FuseEmulator.reset();
            }
        }

        ZxMenuItem {
            text: qsTr("Hard reset")
            onTriggered: {
                menuBar.close();
                FuseEmulator.hardReset();
            }
        }

        ZxMenuItem {
            text: qsTr("Debugger")
            onTriggered: {
                menuBar.close();
                FuseEmulator.debuggerTrap();
            }
        }

        ZxMenuItem {
            text: qsTr("Poke finder")
            onTriggered: {
                menuBar.close();
                pageLoader.source = "PokeFinderPage.qml";
            }
        }

        ZxMenuItem {
            text: qsTr("Poke memory")
            onTriggered: {
                menuBar.close();
                pageLoader.source = "PokeManagerPage.qml";
            }
        }

        ZxMenuItem {
            text: qsTr("NMI")
            onTriggered: {
                menuBar.close();
                FuseEmulator.nmi();
            }
        }
    }

    ZxMenuItem {
        text: qsTr("Options")

        ZxMenuItem {
            text: qsTr("General")
            onTriggered: {
                menuBar.close();
                pageLoader.source = "OptionsGeneralPage.qml";
            }
        }

        ZxMenuItem {
            text: qsTr("Screen")
            onTriggered: {
                menuBar.close();
                pageLoader.source = "OptionsScreenPage.qml";
            }
        }

        ZxMenuItem {
            text: qsTr("Sound")
            onTriggered: {
                menuBar.close();
                pageLoader.source = "OptionsSoundPage.qml";
            }
        }

        ZxMenuItem {
            text: qsTr("Peripherals")
            onTriggered: {
                menuBar.close();
                pageLoader.source = "OptionsPeripheralsPage.qml";
            }
        }

        ZxMenuItem {
            visible: false
            text: qsTr("Media")
            onTriggered: {
                menuBar.close();
                pageLoader.source = "OptionsMediaPage.qml";
            }
        }
        ZxMenuItem {
            text: qsTr("(Virtual) Gamepad")
            ZxMenuItem {
                text: qsTr("Customize Gamepad Buttons")
                onTriggered: {
                    menuBar.close();
                    pageLoader.source = "CustomizeGamepadPage.qml";
                }
            }

            ZxMenuItem {
                visible: FuseEmulator.gamepadId != -1
                text: qsTr("Calibrate Gamepad")
                onTriggered: {
                    menuBar.close();
                    pageLoader.source = "CalibrateGamepad.qml";
                }
            }
        }
    }

    ZxMenuItem {
        text: qsTr("Help")

        ZxMenuItem {
            text: "About .."
            onTriggered: {
                menuBar.close();
                pageLoader.source = "AboutPage.qml";
            }
        }

        ZxMenuItem {
            text: "Features and bugs"
            onTriggered:  {
                menuBar.close();
                Qt.openUrlExternally("https://github.com/bog-dan-ro/spectacol/issues");
            }
        }
    }


    ZxMenuItem {
        text: "Quit"
        onTriggered: quitDialog.open()
    }
}
