import QtQuick 2.6
import QtQuick.Layouts 1.1
import QtQuick.Window 2.2
import QtGamepad 1.0
import Qt.labs.controls 1.0

// @scope main.qml
Pane {
    anchors.fill: parent
    background.opacity: 0.85
    Component.onCompleted: fuse.paused = true
    Component.onDestruction: fuse.paused = false

    GamepadKeyNavigation {
        gamepad: Gamepad { deviceId: fuse.gamepadId }
        buttonBKey: Qt.Key_Escape
    }
    focus: true
    Keys.onLeftPressed: tabbar.currentIndex--
    Keys.onRightPressed: tabbar.currentIndex++

    ColumnLayout {
        anchors.fill: parent
        TabBar {
            id: tabbar
            currentIndex: view.currentIndex
            Layout.fillWidth: true
            TabButton {
                text: qsTr("About")
            }
            TabButton {
                text: qsTr("Gamepad in menu instructions")
            }
            TabButton {
                text: qsTr("Gamepad in emulation instructions")
            }
        }
        SwipeView {
            id: view
            currentIndex: tabbar.currentIndex
            Layout.fillWidth: true
            Layout.fillHeight: true
            FancyText {
                textFormat: Text.RichText
                style: Text.Normal
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignTop
                text: qsTr("Spectacol (in Romanian it means <b>show</b>) is a <a href=\"https://github.com/bog-dan-ro/spectacol\"><b>free</b></a> ZX Spectrum emulator
                            based on famous <a href=\"http://fuse-emulator.sf.net\">FUSE Spectrum emulator</a>.
                            FUSE emulator has, by far, the most reachest ZX features list,
                            but <b>Spectacol</b>, currently, it enables only the following features:
                            <ul>
                                <li> Loads all ZX Spectrum files handles by FUSE
                                <li> Accurate 16K, 48K, 128K, +2, +2A and +3 emulation
                                <li> Quick load & save snapshots. It is quite useful when you play games
                                <li> Easily select the Joystick type
                                <li> Poke finder, disassambler & <b>inline assambler</b>. Allows the user to <i>crack</i> the games and gain infinite lives quickly
                            </ul>
                            <p>
                            In addition to FUSE's features, Spectacol adds more:
                            </p>
                            <ul>
                                <li> The UI is design with gamepad controls in mind. The user will be able to play all the games that support Joysticks using the gamepad.
                                <li> <b>Online</b> search allows the user to access <b>thousands</b> of ZX Spectrum apps & games! The search results are downloaded locally and they can be used to start the games offline
                                <li> On screen numeric cursor, allows the user to quickly start & control the games
                                <li> On Screen full/simplified 48K Keyboard needed when the numeric cursor is not enough
                            </ul>
                            <p>
                            Be aware that, Spectacol is in <b>BETA</b> stage, and <b>currently</b> is intent to be used with a gamepad.<br />
                            Please fell free to <a href=\"https://github.com/bog-dan-ro/spectacol/issues\">report bugs or ask for new features</a> by using <b>Menu-&gt;Help-&gt;Features and bugs</b>.<br />
                            To see how to use <b>Spectacol</b> with your gamepad, select <b>Gamepad</b> tabs above.
                            </p>")
                onLinkActivated: Qt.openUrlExternally(link);
            }
            ColumnLayout {
                Image {
                    Layout.fillHeight: true
                    Layout.fillWidth: true

                    fillMode: Image.PreserveAspectFit
                    source: "qrc:///images/gamepad.png"
                }
                FancyText {
                    Layout.fillWidth: true
                    textFormat: Text.RichText
                    style: Text.Normal
                    wrapMode: Text.WordWrap
                    horizontalAlignment: Text.AlignLeft
                    verticalAlignment: Text.AlignTop
                    text: qsTr("<ul>
                            <li> <b>START</b> toggle the menu
                            <li> <b>BACK</b> (not available on all gamepads) close the menu or the dialog, exits the application
                            <li> <b>UP/DOWN, LEFT AXIS</b> navigate trough menu & controls
                            <li> <b>A</b> select or activate
                            <li> <b>B</b> close the dialog or the menu
                            <li> <b>X</b> shows on screen numeric keys
                            <li> <b>Y</b> shows on screen 48K Keyboard
                            </ul>")
                }
            }
            ColumnLayout {
                Image {
                    Layout.fillHeight: true
                    Layout.fillWidth: true

                    fillMode: Image.PreserveAspectFit
                    source: "qrc:///images/gamepad.png"
                }
                FancyText {
                    Layout.fillWidth: true
                    textFormat: Text.RichText
                    style: Text.Normal
                    wrapMode: Text.WordWrap
                    horizontalAlignment: Text.AlignLeft
                    verticalAlignment: Text.AlignTop
                    text: qsTr("<ul> \
                            <li> <b>L1, R1, A, B</b> Joystick fire button
                            <li> <b>UP/DOWN/LEFT/RIGHT, LEFT AND RIGHT AXIS</b> Joystick directions
                            <li> <b>START</b> toggle menu
                            <li> <b>X</b> on screen numeric keys
                            <li> <b>Y</b> on screen 48K Keyboard
                            <li> <b>L2</b> Quick save snapshot
                            <li> <b>R2</b> Quick load snapshot
                            </ul>")
                }
            }
        }

        Button {
            Layout.fillWidth: true
            text: qsTr("Close <b>(B)</b>")
            onClicked: pageLoader.source = ""
        }
    }
}
