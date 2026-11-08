/*
    Copyright (c) 2015-2026, BogDan Vatra <bogdan@kde.org>

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
import QtQuick.Window
import Spectacol

ZxMenuItem {
    id: zxMenu
    ZxMenuItem {
        visible: ZxEmulator.saveSnapshotEnabled
        text: qsTr("Quick &save snapshot")
        onTriggered: {
            menuBar.close();
            ZxEmulator.quickSaveSnapshot();
        }
    }
    ZxMenuItem {
        text: qsTr("Quick &load snapshot")
        onTriggered: {
            menuBar.close();
            ZxEmulator.quickLoadSnapshot();
        }
    }

    ZxMenuItem {
        visible: ZxEmulator.recording.type !== ZxRecording.None
        text: ZxEmulator.recording.type === ZxRecording.Unsaved ? qsTr("Retry recording save")
              : qsTr("Stop ") + (ZxEmulator.recording.type === ZxRecording.Playback ? qsTr("Playback") : qsTr("Recording"))
        onTriggered: {
            menuBar.close();
            ZxEmulator.recording.stop();
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
            text: qsTr("&Save snapshot as ...")
            onTriggered: {
                menuBar.close();
                pageLoader.source = "SaveSnapshotPage.qml";
            }
        }

        ZxMenuItem {
            text: qsTr("Tape")
            ZxMenuItem {
                text: qsTr("New blank tape")
                onTriggered: {
                    menuBar.close();
                    pageLoader.source = "NewTapePage.qml";
                }
            }
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
                    ZxEmulator.tape.togglePlay();
                }
            }

            ZxMenuItem {
                text: qsTr("Rewind")
                onTriggered: {
                    menuBar.close();
                    ZxEmulator.tape.rewind();
                }
            }

            ZxMenuItem {
                visible: ZxEmulator.tape.hasTape;
                text: qsTr("Browse")
                onTriggered: {
                    menuBar.close();
                    pageLoader.source = "BrowseTapePage.qml";
                }
            }
        }

        ZxMenuItem {
            visible: ZxEmulator.disks.count > 0
            text: qsTr("Disks")
            ZxMenuItem {
                text: qsTr("Insert")
                onTriggered: {
                    menuBar.close();
                    pageLoader.source = "DisksInsertPage.qml";
                }
            }

            ZxMenuItem {
                text: qsTr("Eject")
                onTriggered: {
                    menuBar.close();
                    pageLoader.source = "DisksEjectPage.qml";
                }
            }
        }

        ZxMenuItem {
            visible: ZxEmulator.sdCard.inUse
            text: qsTr("SD card")
            onTriggered: {
                menuBar.close();
                pageLoader.source = "SdCardPage.qml";
            }
        }

        ZxMenuItem {
            text: qsTr("Recording")
            ZxMenuItem {
                visible: ZxEmulator.recording.type === ZxRecording.Recording
                text: qsTr("Stop Recording")
                onTriggered: {
                    menuBar.close();
                    ZxEmulator.recording.stop();
                }
            }
            ZxMenuItem {
                visible: ZxEmulator.recording.type === ZxRecording.None
                text: qsTr("Start recording")
                onTriggered: {
                    menuBar.close();
                    ZxEmulator.recording.start();
                }
            }
            ZxMenuItem {
                text: qsTr("Playback and recording controls…")
                onTriggered: {
                    menuBar.close();
                    pageLoader.source = "RzxControlsPage.qml";
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
        onAboutToShow: {
            multifaceButton.enabled = ZxEmulator.canNextNmi(true);
            divMmcButton.enabled = ZxEmulator.canNextNmi(false);
        }
        ZxMenuItem {
            text: qsTr("Choose machine ...")
            onTriggered: {
                menuBar.close();
                pageLoader.source = "MachineTypePage.qml";
            }
        }

        ZxMenuItem {
            text: qsTr("Reset")
            onTriggered: {
                menuBar.close();
                ZxEmulator.reset();
            }
        }

        ZxMenuItem {
            text: qsTr("Hard reset")
            onTriggered: {
                menuBar.close();
                ZxEmulator.hardReset();
            }
        }

        ZxMenuItem {
            text: qsTr("Debugger")
            onTriggered: {
                menuBar.close();
                ZxEmulator.debuggerTrap();
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
            visible: ZxEmulator.settings.currentMachine !== "ZX Spectrum Next"
            text: qsTr("NMI")
            onTriggered: {
                menuBar.close();
                ZxEmulator.nmi();
            }
        }
        ZxMenuItem {
            id: multifaceButton
            visible: ZxEmulator.settings.currentMachine === "ZX Spectrum Next"
            enabled: false
            text: qsTr("Multiface button")
            onTriggered: { menuBar.close(); ZxEmulator.nextNmi(true); }
        }
        ZxMenuItem {
            id: divMmcButton
            visible: ZxEmulator.settings.currentMachine === "ZX Spectrum Next"
            enabled: false
            text: qsTr("DivMMC drive button")
            onTriggered: { menuBar.close(); ZxEmulator.nextNmi(false); }
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
            text: qsTr("Borders")
            onTriggered: {
                menuBar.close();
                pageLoader.source = "OptionsBordersPage.qml";
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
                text: qsTr("Controller mapping")
                onTriggered: {
                    menuBar.close();
                    pageLoader.source = "CustomizeGamepadPage.qml";
                }
            }

            ZxMenuItem {
                text: qsTr("Controller profiles")
                onTriggered: { menuBar.close(); pageLoader.source = "ControllerProfilesPage.qml"; }
            }
            ZxMenuItem {
                text: qsTr("On-screen controls and fast-forward")
                onTriggered: { menuBar.close(); pageLoader.source = "OptionsControlsPage.qml"; }
            }

            ZxMenuItem {
                visible: ZxEmulator.gamepadId !== -1
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
