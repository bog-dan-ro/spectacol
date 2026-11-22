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

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import Spectacol

// A yes or no question in Spectacol's style. The system's message boxes can't be reached with a
// gamepad or a TV remote everywhere. Left/Right (Up/Down) pick the answer, A, Return or OK gives
// it, B, Escape or Back answers no; touch and the mouse work too.
Dialog {
    id: dialog

    property string text
    property string acceptText: qsTr("Yes")
    property string rejectText: qsTr("No")
    property string alternateText
    property var commit: null
    signal alternateRequested
    // Answering yes loses something: the focus starts on no
    property bool destructive: false
    // Over the game no page turns the gamepad into keys: the dialog does it
    property bool ownGamepad: false

    parent: Overlay.overlay
    popupType: Popup.Item
    anchors.centerIn: parent
    width: Math.min(parent.width - Theme.sp(6), Theme.sp(110))
    modal: true
    focus: true
    // Back and Escape answer, they don't just close it
    closePolicy: Popup.NoAutoClose
    padding: Theme.sp(6)

    onOpened: (destructive ? rejectButton : acceptButton).forceActiveFocus(Qt.PopupFocusReason)

    Overlay.modal: Rectangle {
        color: Theme.scrim
    }
    background: Rectangle {
        color: Theme.raised
        radius: Theme.radius
        border.width: Theme.line
        border.color: Theme.outline
    }
    header: null
    footer: null

    property Instantiator gamepad: Instantiator {
        active: dialog.visible && dialog.ownGamepad
        delegate: DebuggerGamepad {
            transportKeys: false
            buttonXKey: Qt.Key_unknown
            buttonYKey: Qt.Key_unknown
            buttonStartKey: Qt.Key_unknown
        }
    }

    contentItem: ColumnLayout {
        spacing: Theme.sp(6)
        Keys.onEscapePressed: dialog.reject()
        Keys.onBackPressed: dialog.reject()

        Text {
            visible: text !== ""
            Layout.fillWidth: true
            text: dialog.title
            wrapMode: Text.Wrap
            font.pixelSize: Theme.title
            font.weight: Font.DemiBold
            color: Theme.text
        }
        Text {
            objectName: "confirmText"
            Layout.fillWidth: true
            text: dialog.text
            Accessible.role: Accessible.StaticText
            Accessible.name: text
            wrapMode: Text.Wrap
            font.pixelSize: Theme.body
            color: Theme.text
        }
        RowLayout {
            Layout.alignment: Qt.AlignRight
            spacing: Theme.sp(3)

            component Answer: DialogButton {
                id: answer
                required property Item previous
                required property Item next
                Layout.fillWidth: dialog.alternateText !== ""
                Layout.minimumWidth: Theme.sp(16)
                Keys.onLeftPressed: previous.forceActiveFocus(Qt.BacktabFocusReason)
                Keys.onRightPressed: next.forceActiveFocus(Qt.TabFocusReason)
                Keys.onUpPressed: previous.forceActiveFocus(Qt.BacktabFocusReason)
                Keys.onDownPressed: next.forceActiveFocus(Qt.TabFocusReason)
            }
            Answer {
                id: rejectButton
                objectName: "confirmNo"
                text: dialog.rejectText
                previous: acceptButton
                next: alternateButton.visible ? alternateButton : acceptButton
                onClicked: dialog.reject()
            }
            Answer {
                id: alternateButton
                objectName: "confirmAlternate"
                text: dialog.alternateText
                visible: text !== ""
                previous: rejectButton
                next: acceptButton
                onClicked: {
                    dialog.close();
                    dialog.alternateRequested();
                }
            }
            Answer {
                id: acceptButton
                objectName: "confirmYes"
                text: dialog.acceptText
                previous: alternateButton.visible ? alternateButton : rejectButton
                next: rejectButton
                onClicked: if (!dialog.commit || dialog.commit()) dialog.accept()
            }
        }
    }
}
