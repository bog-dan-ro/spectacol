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
import QtQuick.Controls
import QtQuick.Layouts
import Spectacol

// The file manager's viewer: a file's BASIC listing, hex dump, disassembly or screen.
// Sources (BASIC, text) are a read-only TextArea whose document FileViewDocument styles, so the
// mouse and the OS select text as anywhere else. Hex dumps and disassemblies are long tables
// with an address per row: a ListView, which only builds the rows on screen.
// Left/Right switch the views. Up/Down move a line cursor; Space or Return (A) starts and ends
// a selection that moving extends (Shift too), Ctrl+C (X) copies it, G (Y) goes to an address.
// A screen copies as an image, or Ctrl+S (Y) saves it as a PNG. Escape or Backspace (B) clears
// the selection, then closes.
FocusScope {
    id: viewer

    required property FileManagerPanel panel
    property var content: ({})
    // Disassemblies restarted at an address, by page
    property var restarted: ({})
    // Where the selection started, or -1
    property int anchor: -1
    property string status
    readonly property var pages: content.pages ?? []
    readonly property var page: pages[tabs.currentIndex] ?? ({})
    readonly property var pageLines: restarted[tabs.currentIndex] ?? page.lines ?? []
    readonly property bool screenPage: (page.image ?? "") !== ""
    readonly property bool addressed: page.style === "hex" || page.style === "disassembly"
    readonly property bool textPage: !screenPage && !addressed
    // The line the cursor is on, and how many there are
    readonly property int row: textPage ? source.lineOf(edit.cursorPosition) : lines.currentIndex
    readonly property int rowCount: textPage ? source.lineCount : lines.count
    readonly property int selectionFirst: anchor < 0 ? row : Math.min(anchor, row)
    readonly property int selectionLast: anchor < 0 ? row : Math.max(anchor, row)
    signal closed
    signal goToRequested
    signal saveRequested

    // Puts the cursor on a line; a started selection reaches from its anchor to that line
    function setRow(target: int) {
        if (!textPage) {
            lines.currentIndex = target;
            return;
        }
        if (anchor < 0) {
            edit.deselect();
            edit.cursorPosition = source.lineStart(target);
        } else if (target >= anchor) {
            edit.select(source.lineStart(anchor), source.lineEnd(target));
        } else {
            edit.select(source.lineEnd(anchor), source.lineStart(target));
        }
    }
    function toStart() {
        anchor = -1;
        status = "";
        lines.currentIndex = 0;
        lines.positionViewAtBeginning();
        edit.deselect();
        edit.cursorPosition = 0;
    }
    function show(newContent: var) {
        content = newContent;
        restarted = {};
        tabs.currentIndex = 0;
        toStart();
        visible = true;
        forceActiveFocus();
    }
    function close() {
        visible = false;
        closed();
    }
    // Back: the selection first, then the viewer
    function back() {
        if (anchor >= 0)
            anchor = -1;
        else
            close();
    }
    function switchPage(delta: int) {
        if (!pages.length)
            return;
        tabs.currentIndex = (tabs.currentIndex + delta + pages.length) % pages.length;
        toStart();
    }
    // Moves the line cursor; a started selection, or Shift, extends the selection
    function move(rows: int, extend: bool) {
        if (screenPage || !rowCount)
            return;
        const from = row;
        if (extend && anchor < 0)
            anchor = from;
        setRow(Math.max(0, Math.min(rowCount - 1, from + rows)));
        if (!textPage)
            lines.positionViewAtIndex(lines.currentIndex, ListView.Contain);
    }
    function pageRows(): int {
        return Math.max(1, Math.floor((textPage ? scroll.height : lines.height) / lineHeight.height) - 1);
    }
    function toggleSelection() {
        if (screenPage)
            return;
        const at = row;
        anchor = anchor < 0 ? at : -1;
        setRow(at);
    }
    function selectAll() {
        if (screenPage || !rowCount)
            return;
        anchor = 0;
        setRow(rowCount - 1);
    }
    function copy() {
        if (screenPage) {
            panel.copyImage(page.image);
            status = qsTr("The screen is copied.");
            return;
        }
        // Text picked with the mouse or the OS's handles: exactly that
        if (textPage && anchor < 0 && edit.selectedText.length) {
            panel.copyText(edit.selectedText.replace(/\u2029/g, "\n"));
            status = qsTr("The selection is copied.");
            edit.deselect();
            return;
        }
        if (!pageLines.length)
            return;
        const first = Math.max(0, selectionFirst);
        const last = Math.min(pageLines.length - 1, selectionLast);
        panel.copyText(pageLines.slice(first, last + 1).join("\n") + "\n");
        status = qsTr("%n line(s) copied.", "", last - first + 1);
        anchor = -1;
    }
    // Hex and disassembly lines start with their address
    function lineAddress(line: string): int {
        const space = String(line).indexOf(" ");
        return space > 0 && /^[0-9A-F]+$/.test(line.slice(0, space)) ? parseInt(line.slice(0, space), 16) : -1;
    }
    function goTo(address: int): bool {
        if (!addressed)
            return false;
        let all = pageLines;
        const find = list => {
            let found = -1;
            for (let low = 0, high = list.length - 1; low <= high;) {
                const middle = (low + high) >> 1;
                const at = lineAddress(list[middle]);
                if (at >= 0 && at <= address) {
                    found = middle;
                    low = middle + 1;
                } else {
                    high = middle - 1;
                }
            }
            return found;
        };
        let row = find(all);
        const end = page.style === "disassembly" ? page.origin + page.bytes.byteLength
                                                 : (row >= 0 ? lineAddress(all[row]) + 16 : -1);
        if (row < 0 || address >= end) {
            status = qsTr("%1 is outside this view.").arg(address);
            return false;
        }
        // Inside an instruction: disassemble again from there
        if (page.style === "disassembly" && lineAddress(all[row]) !== address) {
            all = panel.disassemble(page.bytes, page.origin, address);
            const copy = Object.assign({}, restarted);
            copy[tabs.currentIndex] = all;
            restarted = copy;
            row = find(all);
        }
        anchor = -1;
        status = "";
        lines.currentIndex = row;
        lines.positionViewAtIndex(row, ListView.Center);
        return true;
    }

    visible: false

    Keys.onPressed: event => handleKey(event)
    function handleKey(event: var) {
        const shift = (event.modifiers & Qt.ShiftModifier) !== 0;
        const control = (event.modifiers & Qt.ControlModifier) !== 0;
        status = "";
        switch (event.key) {
        case Qt.Key_Escape:
        case Qt.Key_Backspace:
            back();
            break;
        case Qt.Key_F3:
        case Qt.Key_F10:
            close();
            break;
        case Qt.Key_Left:
            switchPage(-1);
            break;
        case Qt.Key_Right:
        case Qt.Key_Tab:
            switchPage(1);
            break;
        case Qt.Key_Up:
            move(-1, shift);
            break;
        case Qt.Key_Down:
            move(1, shift);
            break;
        case Qt.Key_PageUp:
            move(-pageRows(), shift);
            break;
        case Qt.Key_PageDown:
            move(pageRows(), shift);
            break;
        case Qt.Key_Home:
            move(-rowCount, shift);
            break;
        case Qt.Key_End:
            move(rowCount, shift);
            break;
        case Qt.Key_Return:
        case Qt.Key_Enter:
        case Qt.Key_Space:
        case Qt.Key_Select:
            toggleSelection();
            break;
        case Qt.Key_Copy:
            copy();
            break;
        case Qt.Key_Go:
            if (screenPage)
                saveRequested();
            else if (addressed)
                goToRequested();
            break;
        case Qt.Key_A:
            if (!control)
                return;
            selectAll();
            break;
        case Qt.Key_C:
        case Qt.Key_Insert:
            if (!control)
                return;
            copy();
            break;
        case Qt.Key_S:
            if (!control || !screenPage)
                return;
            saveRequested();
            break;
        case Qt.Key_G:
            if (!addressed)
                return;
            goToRequested();
            break;
        default:
            return;
        }
        event.accepted = true;
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.raised
        radius: Theme.radiusSmall
        border.width: Theme.line
        border.color: Theme.outline
    }

    TextMetrics {
        id: lineHeight
        font.family: Theme.monoFamily
        font.pixelSize: Theme.body
        text: "0"
    }

    // A table row's text, coloured: escaped, with runs of spaces kept
    function escaped(text: string): string {
        return text.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;").replace(/  /g, " &nbsp;");
    }
    function coloured(text: string, colour: color): string {
        return `<font color="${colour}">${escaped(text)}</font>`;
    }
    function styled(line: string, style: string): string {
        if (style === "hex") {
            // ADDR  HH HH … (16 bytes)  text
            const address = line.indexOf(" ");
            if (address < 0 || !/^[0-9A-F]+$/.test(line.slice(0, address)))
                return coloured(line, Theme.textMuted);
            return coloured(line.slice(0, address), Theme.textMuted) + escaped(line.slice(address, address + 50))
                    + coloured(line.slice(address + 50), Theme.textMuted);
        }
        // ADDR  bytes        MNEMONIC operands
        const instruction = line.slice(18);
        const space = instruction.indexOf(" ");
        const mnemonic = space < 0 ? instruction : instruction.slice(0, space);
        return coloured(line.slice(0, 18), Theme.textMuted) + coloured(mnemonic, Theme.accent)
                + escaped(space < 0 ? "" : instruction.slice(space));
    }

    FileViewDocument {
        id: source
        document: edit.textDocument
        page: viewer.textPage ? viewer.page : ({})
        keywordColor: Theme.accent
        numberColor: Theme.padX
        stringColor: Theme.warning
        controlColor: Theme.padY
        mutedColor: Theme.textMuted
        protectedColor: Theme.danger
        onPageChanged: edit.cursorPosition = 0
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.sp(2)
        spacing: Theme.sp(1)

        FancyText {
            Layout.fillWidth: true
            text: viewer.content.title ?? ""
            elide: Text.ElideMiddle
            font.pixelSize: Theme.title
            font.weight: Font.DemiBold
        }
        FancyText {
            Layout.fillWidth: true
            visible: text !== ""
            text: viewer.content.subtitle ?? ""
            elide: Text.ElideRight
            color: Theme.textMuted
            font.pixelSize: Theme.caption
        }

        TabBar {
            id: tabs
            objectName: "fileViewerTabs"
            Layout.fillWidth: true
            focusPolicy: Qt.NoFocus
            onCurrentIndexChanged: viewer.toStart()
            Repeater {
                model: viewer.pages
                delegate: TabButton {
                    required property var modelData
                    text: modelData.title
                    focusPolicy: Qt.NoFocus
                    width: implicitWidth
                }
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ScrollView {
                id: scroll
                objectName: "fileViewerSource"
                anchors.fill: parent
                visible: viewer.textPage
                TextArea {
                    id: edit
                    objectName: "fileViewerText"
                    readOnly: true
                    selectByMouse: true
                    persistentSelection: true
                    wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere
                    textFormat: TextEdit.PlainText
                    font.family: Theme.monoFamily
                    font.pixelSize: Theme.body
                    color: Theme.text
                    selectionColor: Theme.accent
                    selectedTextColor: Theme.textOnAccent
                    leftPadding: 0
                    rightPadding: Theme.sp(2)
                    topPadding: 0
                    bottomPadding: 0
                    background: null
                    // The viewer's keys first: the arrows move by line, Left and Right switch views
                    Keys.onPressed: event => viewer.handleKey(event)
                    // A press starts the mouse's own selection
                    onPressed: viewer.anchor = -1
                    // The line the cursor is on
                    Rectangle {
                        z: -1
                        visible: edit.selectionStart === edit.selectionEnd
                        y: edit.cursorRectangle.y
                        width: edit.width
                        height: edit.cursorRectangle.height
                        color: Qt.alpha(Theme.accent, 0.07)
                    }
                }
            }

            ListView {
                id: lines
                objectName: "fileViewerLines"
                readonly property string style: viewer.page.style ?? "hex"
                anchors.fill: parent
                visible: viewer.addressed
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                highlightMoveDuration: 0
                model: viewer.addressed ? viewer.pageLines : []
                ScrollIndicator.vertical: ScrollIndicator {}
                delegate: Item {
                    id: row
                    required property string modelData
                    required property int index
                    readonly property bool selected: viewer.anchor >= 0 && index >= viewer.selectionFirst
                                                     && index <= viewer.selectionLast
                    width: lines.width
                    height: Math.max(Theme.touchScreenUi ? Theme.sp(8) : 0, body.implicitHeight)
                    Rectangle {
                        anchors.fill: parent
                        visible: row.selected || row.ListView.isCurrentItem
                        color: row.selected ? Theme.accentFill : Qt.alpha(Theme.accent, 0.07)
                    }
                    Text {
                        id: body
                        width: row.width
                        anchors.verticalCenter: parent.verticalCenter
                        text: viewer.styled(row.modelData, lines.style)
                        textFormat: Text.StyledText
                        wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                        font.family: Theme.monoFamily
                        font.pixelSize: Theme.body
                        color: Theme.text
                    }
                    // A click puts the cursor on the line, Shift extends the selection to it,
                    // a long press starts one
                    TapHandler {
                        acceptedModifiers: Qt.NoModifier
                        onTapped: {
                            viewer.anchor = -1;
                            lines.currentIndex = row.index;
                            viewer.forceActiveFocus();
                        }
                        onLongPressed: {
                            lines.currentIndex = row.index;
                            viewer.anchor = row.index;
                        }
                    }
                    TapHandler {
                        acceptedModifiers: Qt.ShiftModifier
                        onTapped: {
                            if (viewer.anchor < 0)
                                viewer.anchor = lines.currentIndex;
                            lines.currentIndex = row.index;
                        }
                    }
                }
            }

            Image {
                id: screen
                objectName: "fileViewerScreen"
                anchors.centerIn: parent
                visible: viewer.screenPage
                source: viewer.page.image ?? ""
                // Whole multiples of the Spectrum's pixels
                readonly property real zoom: Math.max(1, Math.floor(Math.min(parent.width / 256, parent.height / 192)))
                width: 256 * zoom
                height: 192 * zoom
                sourceSize: Qt.size(256, 192)
                smooth: false
                fillMode: Image.PreserveAspectFit
            }
        }

        FancyText {
            objectName: "fileViewerStatus"
            Layout.fillWidth: true
            visible: text !== ""
            text: viewer.status
            elide: Text.ElideRight
            font.pixelSize: Theme.caption
            color: Theme.textMuted
        }
    }
}
