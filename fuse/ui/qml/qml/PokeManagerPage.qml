/*
    Copyright (c) 2016, BogDan Vatra <bogdan@kde.org>

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

// @scope main.qml

import QtQuick 2.6
import QtQuick.Window 2.2
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.1
import QtGamepad 1.0
import Fuse 1.0
import "private" 1.0

Item {
    Component.onCompleted: {
        fuse.processInputEvents = false;
        fuse.paused = true;
    }
    Component.onDestruction: {
        fuse.processInputEvents = true;
        fuse.paused = false;
    }

    GamepadKeyNavigation {
        gamepad: Gamepad { deviceId: fuse.gamepadId }
        buttonAKey: Qt.Key_Space
        buttonBKey: Qt.Key_Escape
        buttonYKey: Qt.Key_Y
        buttonXKey: Qt.Key_X
    }

    PokeModel {
        id: pokeModel
        onModelReset: view.currentIndex = 0
    }

    Popup {
        id: addPoke
        modal: true
        focus: true
        x: parent.width / 2 - width / 2
        height: TextSizes.scale34 * 7
        width: TextSizes.scale20 * 10
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnReleaseOutsideParent
        onOpened: address.forceActiveFocus(Qt.TabFocusReason);
        IntValidator {
            id: addressValidator
            bottom: bank.currentIndex === 8 ? 0x4000 : 0
            top: bank.currentIndex === 8 ? 0xffff : 0x3fff
        }
        GridLayout {
            columns: 2
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.right: parent.right
            Label {
                Layout.alignment: Qt.AlignRight
                text: qsTr("Bank:")
            }
            ComboBox {
                id: bank
                model: 64
                currentIndex: 8
                Keys.onUpPressed: {
                    if (popup.visible)
                        decrementCurrentIndex();
                    else
                        ok.forceActiveFocus(Qt.TabFocusReason);
                }
                Keys.onDownPressed: {
                    if (popup.visible)
                        incrementCurrentIndex();
                    else
                        address.forceActiveFocus(Qt.TabFocusReason);
                }
                Keys.onLeftPressed: if (!popup.visible) decrementCurrentIndex()
                Keys.onRightPressed: if (!popup.visible) incrementCurrentIndex()
            }

            Label {
                Layout.alignment: Qt.AlignRight
                text: qsTr("Address:")
            }
            TextField {
                id: address
                placeholderText: addressValidator.bottom + " ... " + addressValidator.top
                validator: addressValidator
                inputMethodHints: Qt.ImhDigitsOnly
                KeyNavigation.up: bank
                KeyNavigation.down: value
            }

            Label {
                Layout.alignment: Qt.AlignRight
                text: qsTr("Value:")
            }
            TextField {
                id: value
                placeholderText: "0 ... 255"
                validator: IntValidator {
                    bottom: 0
                    top: 255
                }
                inputMethodHints: Qt.ImhDigitsOnly
                KeyNavigation.up: address
                KeyNavigation.down: ok
            }

            Button {
                id: ok
                text: qsTr("Ok")
                onClicked: {
                    pokeModel.addPoke(bank.currentIndex, address.text, value.text);
                    addPoke.close();
                }
                KeyNavigation.up: value
                KeyNavigation.down: bank
                KeyNavigation.right: cancel
            }
            Button {
                id: cancel
                Layout.alignment: Qt.AlignRight
                text: qsTr("Cancel")
                onClicked: addPoke.close();
                KeyNavigation.left: ok
                KeyNavigation.up: value
                KeyNavigation.down: bank
            }
        }
    }

    FancyBaseList {
        id: view
        width: parent.width / 2
        focus: true

        model: pokeModel

        Keys.onPressed: {
            switch (event.key) {
            case Qt.Key_Left:
                decrementCurrentIndex();
                break;
            case Qt.Key_Right:
                incrementCurrentIndex();
                break;
            case Qt.Key_Escape:
                pageLoader.source = "";
                break;
            case Qt.Key_Y:
                addPoke.open();
                break;
            case Qt.Key_X:
                pokeModel.clear();
                break;
            }
        }

        delegate: Component {
            CheckBox {
                width: view.width
                checked: model.active
                enabled: !model.dissabled
                text: model.label
                onCheckedChanged: pokeModel.setActive(model.index, checked)
                background: Rectangle {color: Qt.rgba(0, 0, 0, 0.75)}
            }
        }
        header: FancyText {
            width: view.width
            text: qsTr("Poke manager")
            font.pixelSize: TextSizes.scale20
            horizontalAlignment: Text.AlignHCenter
            background: Rectangle {color: Qt.rgba(0, 0, 0, 0.75)}
        }

        footer: RowLayout {
            width: view.width
            spacing: Screen.pixelDensity * 5
            Button {
                text: qsTr("Add (Y)")
                onClicked: addPoke.open();
            }
            Button {
                text: qsTr("Clear (X)")
                onClicked: pokeModel.clear();
            }

            Item {
                Layout.fillWidth: true
            }

            Button {
                text: qsTr("Close (B)")
                onClicked: pageLoader.source = ""
            }
        }
    }
}
