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
import QtGamepadLegacy
import Spectacol

Sheet {
    id: page

    property var printer: ZxEmulator.zxPrinterStatus()
    readonly property list<string> formats: ["pdf", "png", "pbm", "txt"]
    property int formatIndex: 0
    readonly property string format: formats[formatIndex]

    function refresh() {
        printer = ZxEmulator.zxPrinterStatus();
    }

    function closeDialog(): bool {
        if (!destination.closeNested()) {
            if (preview.visible)
                preview.close();
            else if (confirmClear.visible)
                confirmClear.reject();
            else
                closeRequested();
        }
        return true;
    }

    Component.onCompleted: Qt.callLater(page.refresh)

    title: qsTr("ZX Printer")
    crumb: qsTr("Media")
    hints: [
        { pad: "A", key: "Enter", text: qsTr("Select") },
        { pad: "B", key: "Esc", text: qsTr("Back"), action: () => page.closeDialog() }
    ]

    GamepadNavigation {
        gamepad: Gamepad { deviceId: ZxEmulator.gamepadId }
        buttonAKey: Qt.Key_Return
        buttonBKey: Qt.Key_Escape
    }

    SettingsList {
        id: settings
        SettingRow {
            label: qsTr("Connect ZX Printer")
            description: !page.printer.available ? qsTr("Unavailable on this machine")
                       : page.printer.hasEditorPrint ? qsTr("Add a numbered line with Enter, then choose Edit → Print to list the program. Use LPRINT for text or COPY for the screen.")
                       : qsTr("Print from BASIC with LPRINT, LLIST or COPY.")
            enabled: page.printer.available || page.printer.enabled
            checked: page.printer.enabled
            onToggled: (on) => {
                ZxEmulator.setZxPrinterEnabled(on);
                page.refresh();
            }
        }
        SettingRow {
            id: previewRow
            objectName: "printPreview"
            label: qsTr("Preview printout")
            enabled: page.printer.rows > 0
            kind: "action"
            onClicked: preview.open()
        }
        SettingRow {
            label: qsTr("Export format")
            kind: "choice"
            enabled: page.printer.rows > 0
            model: page.formats.map(format => format.toUpperCase())
            currentIndex: page.formatIndex
            onActivated: (index) => page.formatIndex = index
            description: page.format === "txt" ? qsTr("Recognizes aligned Spectrum characters. Unknown characters become question marks.")
                                               : ""
        }
        SettingRow {
            id: exportRow
            label: qsTr("Export printout")
            description: page.printer.rows === 0 ? qsTr("The paper is empty.")
                       : page.format === "txt" && !page.printer.hasText ? qsTr("There is no recognized text to export.")
                       : page.printer.truncated ? qsTr("The paper buffer is full. Clear it to capture more output.")
                       : page.printer.rows === 1 ? qsTr("1 printed scanline")
                       : qsTr("%1 printed scanlines").arg(page.printer.rows)
            enabled: page.printer.rows > 0 && (page.format !== "txt" || page.printer.hasText)
            kind: "action"
            onClicked: {
                destination.path = ZxEmulator.zxPrinterExportPath(page.format);
                destination.open();
            }
        }
        SettingRow {
            id: clearRow
            label: qsTr("Clear paper")
            enabled: page.printer.rows > 0
            kind: "action"
            onClicked: confirmClear.open()
        }
    }

    PrinterPreviewDialog {
        id: preview
        onClosed: {
            previewRow.forceActiveFocus(Qt.BacktabFocusReason);
            settings.ensureVisible(previewRow);
        }
    }

    HostPathDialog {
        id: destination
        title: qsTr("Export ZX Printer output")
        returnFocus: exportRow
        saveFile: true
        defaultSuffix: page.format
        nameFilters: [qsTr("%1 files (*.%2)").arg(page.format.toUpperCase()).arg(page.format)]
        submit: (path) => ZxEmulator.exportZxPrinter(path, page.format)
    }

    ConfirmDialog {
        id: confirmClear
        title: qsTr("Clear ZX Printer paper?")
        text: qsTr("This removes the captured printout.")
        acceptText: qsTr("Clear")
        destructive: true
        onAccepted: {
            ZxEmulator.clearZxPrinterPaper();
            page.refresh();
        }
        onClosed: clearRow.enabled ? clearRow.forceActiveFocus() : settings.move(0)
    }
}
