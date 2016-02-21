import QtQuick 2.6
import QtQuick.Window 2.2
import QtGamepad 1.0
import Qt.labs.controls 1.0

// @scope main.qml

Flickable {
    contentHeight: pane.height
    Component.onCompleted: fuse.paused = true
    Component.onDestruction: fuse.paused = false

    function getIndex(data, str) {
        for (var i = 0; i < data.length; ++i) {
            if (data[i] === str) {
                console.log(data[i] + "   " + i);
                return i;
            }
        }
        return 0;
    }

    GamepadKeyNavigation {
        gamepad: Gamepad { deviceId: fuse.gamepadId }
        buttonBKey: Qt.Key_Escape
        buttonAKey: Qt.Key_Space
    }

    Pane {
        id: pane
        focus: true
        background.opacity: 0.85
        PropertyAnimation on x {
            to: (pageLoader.width - pane.width) / 2
            easing.type: Easing.InOutBack
            easing.amplitude: 2.0
            easing.period: 1.5
            duration: 500
        }
        Column {
            spacing: 2.5 * Screen.pixelDensity
            anchors.horizontalCenter: parent.horizontalCenter
            Label {
                text: qsTr("Sound Options")
            }


            CheckBox {
                id: soundEnabled
                KeyNavigation.up: speakerType
                KeyNavigation.down: loadingSound
                focus: true
                text: qsTr("Sound enabled")
                checked: fuseSettings.soundEnabled
                onCheckedChanged: fuseSettings.soundEnabled = checked
            }

            CheckBox {
                id: loadingSound
                KeyNavigation.up: soundEnabled
                KeyNavigation.down: soundForce8Bit
                text: qsTr("Loading sound")
                checked: fuseSettings.loadingSound
                onCheckedChanged: fuseSettings.loadingSound = checked
            }

            CheckBox {
                id: soundForce8Bit
                KeyNavigation.up: loadingSound
                KeyNavigation.down: ayStereoSeparation
                text: qsTr("Force 8 bit")
                checked: fuseSettings.soundForce8Bit
                onCheckedChanged: fuseSettings.soundForce8Bit = checked
            }

            Row {
                spacing: 2.5 * Screen.pixelDensity
                Label {
                    text: qsTr("AY stereo separation")
                    anchors.verticalCenter: parent.verticalCenter
                }

                ComboBox {
                    id: ayStereoSeparation
//TODO:                    KeyNavigation.up: speakerType
//TODO:                    KeyNavigation.down: loadingSound
                    model: ["None", "ACB", "ABC"]
                    currentIndex: getIndex(model, fuseSettings.AYStereoSeparation)
                    onCurrentTextChanged: fuseSettings.AYStereoSeparation = currentText
                }
            }

            Row {
                spacing: 2.5 * Screen.pixelDensity
                Label {
                    text: qsTr("Speaker type")
                    anchors.verticalCenter: parent.verticalCenter
                }

                ComboBox {
                    id: speakerType
//TODO:                    KeyNavigation.up: speakerType
//TODO:                    KeyNavigation.down: loadingSound
                    model: ["TV speaker", "Beeper", "Unfiltered"]
                    currentIndex: getIndex(model, fuseSettings.speakerType)
                    onCurrentTextChanged: fuseSettings.speakerType = currentText
                }
            }


            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Close (B)")
                onClicked: pageLoader.source = ""
            }
        }
    }
}
