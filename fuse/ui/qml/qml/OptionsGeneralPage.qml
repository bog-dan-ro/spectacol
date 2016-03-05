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
        buttonAKey: Qt.Key_Space
        buttonBKey: Qt.Key_Escape
        buttonYKey: Qt.Key_Tab
        buttonXKey: Qt.Key_Backtab
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
                text: qsTr("General Options")
            }

            Row {
                spacing: 2.5 * Screen.pixelDensity
                Label {
                    text: qsTr("Screen filter")
                    anchors.verticalCenter: parent.verticalCenter
                }
                ComboBox {
                    focus: true
                    id: screenFilter
                    Keys.onUpPressed: {
                        if (popup.visible)
                            decrease();
                        else
                            detectLoaders.focus = true;
                    }
                    Keys.onDownPressed: {
                        if (popup.visible)
                            increase();
                        else
                            emulationSpeed.focus = true;
                    }
                    Keys.onLeftPressed: if (!popup.visible) decrease();
                    Keys.onRightPressed: if (!popup.visible) increase();

                    model: fuse.filtersModel
                    currentIndex: fuse.selectedFilterIndex
                    onCurrentIndexChanged: fuse.selectedFilterIndex = currentIndex
                }
            }

            Row {
                spacing: 2.5 * Screen.pixelDensity
                Label {
                    text: qsTr("Emulation Speed")
                    anchors.verticalCenter: parent.verticalCenter
                }
                SpinBox {
                    id: emulationSpeed
                    Keys.onUpPressed: screenFilter.focus = true
                    Keys.onDownPressed: fastLoad.focus = true
                    Keys.onLeftPressed: decrease()
                    Keys.onRightPressed: increase()
                    from: 10
                    to: 1000
                    value: fuseSettings.emulationSpeed
                    onValueChanged: fuseSettings.emulationSpeed = value
                }
            }

            CheckBox {
                id: fastLoad
                KeyNavigation.up: emulationSpeed
                KeyNavigation.down: accelerateLoader
                text: qsTr("Fast loading")
                checked: fuseSettings.fastLoad
                onCheckedChanged: fuseSettings.fastLoad = checked
            }

            CheckBox {
                id: accelerateLoader
                KeyNavigation.up: fastLoad
                KeyNavigation.down: autoLoad
                text: qsTr("Accelerate loaders")
                checked: fuseSettings.accelerateLoader
                onCheckedChanged: fuseSettings.accelerateLoader = checked
            }

            CheckBox {
                id: autoLoad
                KeyNavigation.up: accelerateLoader
                KeyNavigation.down: detectLoaders
                text: qsTr("Auto load media")
                checked: fuseSettings.autoLoad
                onCheckedChanged: fuseSettings.autoLoad = checked
            }

            CheckBox {
                id: detectLoaders
                KeyNavigation.up: autoLoad
                KeyNavigation.down: screenFilter
                text: qsTr("Detect loaders")
                checked: fuseSettings.detectLoaders
                onCheckedChanged: fuseSettings.detectLoaders = checked
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Close (B)")
                onClicked: pageLoader.source = ""
            }
        }
    }
}
