import QtQuick 2.6
import QtQuick.Window 2.2
import QtGamepad 1.0
import Qt.labs.controls 1.0

// @scope main.qml

Flickable {
    contentHeight: pane.height
    Component.onCompleted: fuse.paused = true
    Component.onDestruction: fuse.paused = false
    anchors.margins: Screen.pixelDensity

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
                text: qsTr("Peripherals Options")
            }

            CheckBox {
                id: kempstonJoystick
                KeyNavigation.up: interface2
                KeyNavigation.down: kempstonMouse
                focus: true
                text: qsTr("Kempston joystick")
                checked: fuseSettings.kempstonJoystick
                onCheckedChanged: fuseSettings.kempstonJoystick = checked
            }

            CheckBox {
                id: kempstonMouse
                KeyNavigation.up: kempstonJoystick
                KeyNavigation.down: interface1
                text: qsTr("Kempston mouse")
                checked: fuseSettings.kempstonMouse
                onCheckedChanged: fuseSettings.kempstonMouse = checked
            }

            CheckBox {
                id: interface1
                KeyNavigation.up: kempstonMouse
                KeyNavigation.down: interface2
                text: qsTr("Interface 1")
                checked: fuseSettings.interface1
                onCheckedChanged: fuseSettings.interface1 = checked
            }

            CheckBox {
                id: interface2
                KeyNavigation.up: interface1
                KeyNavigation.down: kempstonJoystick
                text: qsTr("Interface 2")
                checked: fuseSettings.interface2
                onCheckedChanged: fuseSettings.interface2 = checked
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Close (B)")
                onClicked: pageLoader.source = ""
            }
        }
    }
}
