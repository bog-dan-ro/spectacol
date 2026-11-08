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
import QtQuick.Layouts
import QtGamepadLegacy
import Spectacol
import "private"

// @scope main.qml
Sheet {
    property bool returnToCard: false
    crumb: qsTr("ZX Spectrum Next")
    title: qsTr("SD card image")
    preferredWidth: Theme.sp(150)
    hints: [
        { pad: "A", key: "Enter", text: qsTr("Download"), action: () => { if (!busy) NextSdCard.download(); } },
        { pad: "B", key: "Esc", text: qsTr("Cancel"), action: () => pageLoader.source = "" }
    ]

    Component.onCompleted: downloadButton.forceActiveFocus()
    // closing the page cancels the download
    Component.onDestruction: if (busy) NextSdCard.cancel()

    GamepadKeyNavigation {
        gamepad: Gamepad { deviceId: ZxEmulator.gamepadId }
        buttonBKey: Qt.Key_Escape
    }

    Connections {
        target: NextSdCard
        function onReady() { pageLoader.source = returnToCard ? "SdCardPage.qml" : ""; }
    }

    readonly property bool busy: NextSdCard.state === NextSdCard.Checking || NextSdCard.state === NextSdCard.Downloading || NextSdCard.state === NextSdCard.Extracting

    ColumnLayout {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: Theme.sp(6)
        spacing: Theme.sp(4)

        FancyText {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignLeft
            textFormat: Text.RichText
            text: "<style>a { color: " + Theme.accent + "; }</style>" + qsTr("The <b>ZX Spectrum Next</b> boots NextZXOS from an SD card.<br/><br/>"
                       + "Spectacol can download the official SD card image from "
                       + "<a href=\"https://www.specnext.com\">specnext.com</a> "
                       + "(about 52 MB to download, 1 GB on disk).<br/>"
                       + "The emulated Next writes its changes to this image.")
        }

        FancyText {
            Layout.fillWidth: true
            visible: NextSdCard.state !== NextSdCard.Idle
            horizontalAlignment: Text.AlignLeft
            wrapMode: Text.WordWrap
            text: {
                switch (NextSdCard.state) {
                case NextSdCard.Checking: return qsTr("Checking the latest official distribution…");
                case NextSdCard.Downloading: return qsTr("Downloading...");
                case NextSdCard.Extracting: return qsTr("Extracting the SD card image...");
                case NextSdCard.Ready: return qsTr("The SD card image is ready.");
                case NextSdCard.Failed: return qsTr("Failed: %1").arg(NextSdCard.errorString);
                }
                return "";
            }
        }

        ProgressBar {
            Layout.fillWidth: true
            visible: busy
            value: NextSdCard.progress
        }

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: Theme.sp(4)
            Button {
                id: downloadButton
                text: NextSdCard.state === NextSdCard.Failed ? qsTr("Retry") : qsTr("Download")
                enabled: !busy
                onClicked: NextSdCard.download()
                KeyNavigation.right: cancelButton
                Keys.onReturnPressed: clicked()
            }
            Button {
                id: cancelButton
                text: qsTr("Cancel")
                onClicked: {
                    NextSdCard.cancel();
                    pageLoader.source = "";
                }
                KeyNavigation.left: downloadButton
                Keys.onReturnPressed: clicked()
            }
        }
    }
}
