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

// @scope main.qml
pragma ComponentBehavior: Bound

import QtCore
import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtGamepadLegacy
import Spectacol

// A twin-panel file manager in the spirit of Midnight Commander: host folders, and the files,
// BASIC listings and blocks inside tapes and disks. Enter opens folders and images.
Sheet {
    id: page
    crumb: qsTr("Media")
    title: qsTr("File manager")
    preferredWidth: Theme.sp(400)

    property bool rightActive: false
    property string message
    property string lastError
    // Shown while there is no error, e.g. that the Next restarts
    property string notice
    // NextZXOS caches the card: the Next restarts to read it again once the page closes
    property bool sdCardChanged: false
    readonly property FileManagerPanel activePanel: rightActive ? rightPanel : leftPanel
    readonly property FileManagerPanel otherPanel: rightActive ? leftPanel : rightPanel
    readonly property FileManagerPanelView activeView: rightActive ? rightView : leftView
    readonly property bool twin: width >= Theme.sp(170)
    readonly property int row: activeView.currentIndex
    readonly property var popups: [actionsMenu, nameDialog, copyDialog, confirmDialog, attributesDialog,
                                   editorDialog, profileDialog, addressDialog]

    readonly property var viewerHints: [
        { pad: "B", key: "Esc", text: viewer.anchor >= 0 ? qsTr("Unselect") : qsTr("Close"), action: () => viewer.back() },
        { pad: "◀▶", key: "←→", text: qsTr("View") }
    ].concat(viewer.screenPage ? [
        { pad: "X", key: "Ctrl+C", text: qsTr("Copy image"), action: () => viewer.copy() },
        { pad: "Y", key: "Ctrl+S", text: qsTr("Save PNG"), action: () => page.saveScreen() }
    ] : [
        { pad: "A", key: "Space", text: viewer.anchor >= 0 ? qsTr("End selection") : qsTr("Select"), action: () => viewer.toggleSelection() },
        { pad: "X", key: "Ctrl+C", text: qsTr("Copy"), action: () => viewer.copy() }
    ]).concat(viewer.addressed ? [{ pad: "Y", key: "G", text: qsTr("Go to"), action: () => page.askAddress() }] : [])
    readonly property var padHints: [
        { pad: "A", key: "Enter", text: qsTr("Open"), action: () => page.openCurrent() },
        { pad: "B", key: "Esc", text: qsTr("Close"), action: () => page.goBack() },
        { pad: "X", key: "F2", text: qsTr("Actions"), action: () => page.showActions() },
        { pad: "Y", key: "Ins", text: qsTr("Mark"), action: () => page.markCurrent() },
        { pad: "◀▶", key: "Tab", text: qsTr("Panel") }
    ].concat(activePanel.tapeBlocks ? [{ pad: "L2/R2", key: "[ ]", text: qsTr("Move block") }] : [])
    readonly property var keyHints: [
        { pad: "B", key: "Esc", text: qsTr("Close"), action: () => page.goBack() },
        { pad: "F2", key: "F2", text: qsTr("Menu"), action: () => page.showActions() },
        { pad: "F3", key: "F3", text: qsTr("View"), action: () => page.viewCurrent() }
    ].concat(Theme.hardwareKeyboard ? [{ pad: "F4", key: "F4", text: qsTr("Edit"), action: () => page.editCurrent() }] : []).concat([
        { pad: "F5", key: "F5", text: qsTr("Copy"), action: () => page.copyCurrent(false) },
        { pad: "F6", key: "F6", text: qsTr("Move"), action: () => page.copyCurrent(true) },
        { pad: "F7", key: "F7", text: qsTr("Folder"), action: () => page.makeFolder() },
        { pad: "F8", key: "F8", text: qsTr("Delete"), action: () => page.removeCurrent() }
    ]).concat(activePanel.tapeBlocks ? [{ pad: "[ ]", key: "[ ]", text: qsTr("Move block") }] : [])
    // Fingers get every action in the header's menu: there are no keys to press
    readonly property var touchHints: [{ pad: "B", key: "Esc", text: qsTr("Close"), action: () => page.goBack() }]
        .concat(actions().filter(entry => entry.enabled !== false)
                         .map(entry => ({ pad: "", key: "", text: entry.text, action: entry.action })))
    hints: viewer.visible ? viewerHints : Theme.gamepadHints ? padHints : Theme.touchScreenUi ? touchHints : keyHints

    function closePage() {
        mainScreen.closePage();
    }

    // The viewer's screen, as a PNG in the other panel's folder
    function saveScreen() {
        const path = otherPanel.saveImage(viewer.page.image, viewer.content.name);
        if (path)
            viewer.status = qsTr("Saved %1").arg(path);
    }

    function askAddress() {
        if (viewer.addressed)
            addressDialog.open();
    }

    function showError(text: string) {
        lastError = text;
        message = text;
    }

    function noteWritten(path: string) {
        if (!sdCardChanged && ZxEmulator.sdCard.inUse && leftPanel.sameFile(path, ZxEmulator.sdCard.imagePath)) {
            sdCardChanged = true;
            notice = qsTr("The Next restarts when the file manager closes, to read the changed SD card.");
        }
    }

    function focusActive() {
        activeView.forceActiveFocus();
    }

    function setRight(right: bool) {
        rightActive = right;
        settings.setValue("rightActive", right);
        message = "";
        focusActive();
    }

    function popupOpen(): bool {
        return popups.some(popup => popup.visible);
    }

    function goBack() {
        for (const popup of popups) {
            if (popup.visible) {
                popup.close();
                return;
            }
        }
        if (viewer.visible) {
            viewer.close();
            return;
        }
        // Back leaves the file manager from anywhere; ".." (or Backspace) goes up a folder
        closePage();
    }

    function goUp() {
        message = "";
        const row = activePanel.up();
        if (row < 0)
            closePage();
        else
            activeView.select(row);
    }

    // Runs a panel operation; the panels keep their cursors through the refresh
    function run(operation: var): bool {
        message = "";
        lastError = "";
        return operation();
    }

    function openCurrent() {
        message = "";
        const panel = activePanel;
        if (activeView.currentKind === "parent")
            goUp();
        else if (panel.open(row))
            activeView.select(0);
        else
            viewCurrent();
    }

    function viewCurrent() {
        const content = activePanel.view(row);
        if (content.pages && content.pages.length)
            viewer.show(content);
    }

    function editCurrent() {
        // Typing a program needs a real keyboard
        if (!Theme.hardwareKeyboard)
            return;
        const content = activePanel.editable(row);
        if (content.text === undefined) {
            showError(qsTr("Only BASIC programs and text can be edited."));
            return;
        }
        editorDialog.row = row;
        editorDialog.basic = content.basic === true;
        editorDialog.note = content.note ?? "";
        editorDialog.title = qsTr("Edit %1").arg(content.title);
        editor.text = content.text;
        editorDialog.open();
    }

    function markCurrent() {
        activePanel.toggleMark(row);
        activeView.select(row + 1);
    }

    function toggleAllMarks() {
        activePanel.markAll(activePanel.markedCount === 0);
    }

    function describe(names: var): string {
        return names.length === 1 ? qsTr("'%1'").arg(names[0]) : qsTr("%n entries", "", names.length);
    }

    function copyCurrent(move: bool) {
        const names = activePanel.targetNames(row);
        if (!names.length)
            return;
        copyDialog.move = move;
        copyDialog.names = names;
        copyDialog.open();
    }

    function finishCopy(move: bool, name: string, overwrite: bool): bool {
        const ok = run(() => activePanel.copyTo(row, otherPanel, move, name, overwrite));
        if (ok) {
            const target = otherPanel === rightPanel ? rightView : leftView;
            const copied = otherPanel.rowOf(name || activePanel.targetNames(row)[0]);
            if (copied >= 0)
                target.select(copied);
        }
        return ok;
    }

    function makeFolder() {
        if (!activePanel.canMakeFolder) {
            showError(qsTr("This location can't have folders."));
            return;
        }
        nameDialog.ask(qsTr("New folder"), qsTr("Folder name"), "", false,
                       name => page.run(() => page.activePanel.makeFolder(name)) && page.selectName(name));
    }

    function renameCurrent() {
        const names = activePanel.targetNames(row);
        if (names.length !== 1)
            return;
        nameDialog.ask(qsTr("Rename"), qsTr("New name"), names[0], false,
                       name => page.run(() => page.activePanel.rename(page.row, name)) && page.selectName(name));
    }

    function newImage() {
        if (activePanel.inMedia) {
            showError(qsTr("Create images in a host folder."));
            return;
        }
        nameDialog.ask(qsTr("New tape or disk image"), qsTr("File name"), "", true,
                       name => page.run(() => page.activePanel.createImage(name, formatBox.currentIndex))
                               && page.selectName(name));
    }

    function selectName(name: string): bool {
        let index = activePanel.rowOf(name);
        if (index < 0) {
            const formats = activePanel.imageFormats();
            index = activePanel.rowOf(name + "." + formats[formatBox.currentIndex].suffix);
        }
        if (index >= 0)
            activeView.select(index);
        return true;
    }

    function removeCurrent() {
        const names = activePanel.targetNames(row);
        if (!names.length)
            return;
        confirmDialog.ask(qsTr("Delete %1? Folders are deleted with everything in them.").arg(describe(names)),
                          () => page.run(() => page.activePanel.remove(page.row)));
    }

    function editAttributes() {
        const fields = activePanel.attributeFields(row);
        if (!fields.length) {
            showError(qsTr("Only files inside tapes and disks have attributes."));
            return;
        }
        attributesDialog.entries = fields;
        attributesDialog.title = qsTr("Attributes of %1").arg(describe(activePanel.targetNames(row)));
        attributesDialog.open();
    }

    function moveBlock(delta: int) {
        if (!activePanel.tapeBlocks)
            return;
        message = "";
        const moved = activePanel.moveBlock(row, delta);
        if (moved >= 0)
            activeView.select(moved);
    }

    function loadInEmulator() {
        const path = activePanel.hostPath(row);
        if (!path || (!activePanel.inMedia && ["parent", "folder"].includes(activeView.currentKind)))
            return;
        ZxEmulator.load(path);
        pageLoader.source = "";
    }

    function sameFolderInOther() {
        otherPanel.folder = activePanel.folder;
    }

    // An action's name, with its key where there is a keyboard
    function keyed(text: string, key: string): string {
        return Theme.keyboardHints ? qsTr("%1 (%2)").arg(text).arg(key) : text;
    }

    function actions(): var {
        const panel = activePanel;
        const entries = [
            { text: keyed(qsTr("View"), "F3"), action: viewCurrent },
            { text: keyed(qsTr("Copy to the other panel"), "F5"), action: () => copyCurrent(false) },
            { text: keyed(qsTr("Move to the other panel"), "F6"), action: () => copyCurrent(true) },
            { text: keyed(qsTr("Rename"), "Shift+F6"), action: renameCurrent },
            { text: keyed(qsTr("Delete"), "F8"), action: removeCurrent },
            { text: keyed(qsTr("Attributes"), "Alt+Enter"), action: editAttributes, enabled: panel.inMedia },
            { text: keyed(qsTr("Mark or unmark"), "Insert"), action: markCurrent },
            { text: panel.markedCount ? keyed(qsTr("Unmark all"), "*") : keyed(qsTr("Mark all"), "*"), action: toggleAllMarks }
        ];
        if (Theme.hardwareKeyboard)
            entries.splice(1, 0, { text: keyed(qsTr("Edit BASIC or text"), "F4"), action: editCurrent });
        if (panel.tapeBlocks) {
            entries.push({ text: keyed(qsTr("Move block up"), "["), action: () => moveBlock(-1) });
            entries.push({ text: keyed(qsTr("Move block down"), "]"), action: () => moveBlock(1) });
        }
        entries.push({ text: keyed(qsTr("New folder"), "F7"), action: makeFolder, enabled: panel.canMakeFolder });
        entries.push({ text: keyed(qsTr("New tape or disk image"), "Shift+F4"), action: newImage, enabled: !panel.inMedia });
        entries.push({ text: keyed(qsTr("Load in the emulator"), "Shift+Enter"), action: loadInEmulator });
        entries.push({ text: keyed(qsTr("Same folder in the other panel"), "Alt+O"), action: sameFolderInOther, enabled: !panel.inMedia });
        entries.push({ text: keyed(qsTr("Refresh"), "Ctrl+R"), action: () => { leftPanel.refresh(); rightPanel.refresh(); } });
        if (!Theme.touchScreenUi)
            entries.push({ text: keyed(qsTr("Close the file manager"), "F10"), action: closePage });
        return entries;
    }

    function showActions() {
        actionsMenu.entries = actions();
        actionsMenu.open();
    }

    Keys.onPressed: event => {
        const shift = event.modifiers & Qt.ShiftModifier;
        const control = event.modifiers & Qt.ControlModifier;
        const alt = event.modifiers & Qt.AltModifier;
        switch (event.key) {
        case Qt.Key_Tab:
        case Qt.Key_Backtab:
            setRight(!rightActive);
            break;
        case Qt.Key_Left:
            setRight(false);
            break;
        case Qt.Key_Right:
            setRight(true);
            break;
        case Qt.Key_Return:
        case Qt.Key_Enter:
            if (alt)
                editAttributes();
            else if (shift)
                loadInEmulator();
            else
                openCurrent();
            break;
        case Qt.Key_Backspace:
            goUp();
            break;
        case Qt.Key_Insert:
        case Qt.Key_Space:
            markCurrent();
            break;
        case Qt.Key_Asterisk:
            toggleAllMarks();
            break;
        case Qt.Key_F2:
        case Qt.Key_F9:
            showActions();
            break;
        case Qt.Key_F3:
            viewCurrent();
            break;
        case Qt.Key_F4:
            if (shift)
                newImage();
            else
                editCurrent();
            break;
        case Qt.Key_F5:
            copyCurrent(false);
            break;
        case Qt.Key_F6:
            if (shift)
                renameCurrent();
            else
                copyCurrent(true);
            break;
        case Qt.Key_F7:
            makeFolder();
            break;
        case Qt.Key_F8:
        case Qt.Key_Delete:
            removeCurrent();
            break;
        case Qt.Key_F10:
            closePage();
            break;
        case Qt.Key_BracketLeft:
            moveBlock(-1);
            break;
        case Qt.Key_BracketRight:
            moveBlock(1);
            break;
        case Qt.Key_PageUp:
        case Qt.Key_PageDown: {
            const rows = Math.max(1, Math.floor(activeView.height / Theme.listRowHeight) - 2);
            activeView.select(row + (event.key === Qt.Key_PageUp ? -rows : rows));
            break;
        }
        case Qt.Key_Home:
            activeView.select(0);
            break;
        case Qt.Key_End:
            activeView.select(activePanel.count - 1);
            break;
        case Qt.Key_R:
            if (!control)
                return;
            leftPanel.refresh();
            rightPanel.refresh();
            break;
        case Qt.Key_O:
            if (!alt)
                return;
            sameFolderInOther();
            break;
        default:
            return;
        }
        event.accepted = true;
    }

    FileManagerPanel {
        id: leftPanel
        peer: rightPanel
        topmostFolder: ZxEmulator.settings.restrictToSpectacol ? ZxEmulator.dataPath : ""
        onLocationChanged: settings.setValue("leftFolder", leftPanel.folder)
        onMediaWritten: path => page.noteWritten(path)
        onFailed: text => page.showError(text)
        onFilesystemNeeded: (row, choices, text) => profileDialog.ask(leftPanel, row, choices, text)
    }
    FileManagerPanel {
        id: rightPanel
        peer: leftPanel
        topmostFolder: ZxEmulator.settings.restrictToSpectacol ? ZxEmulator.dataPath : ""
        onLocationChanged: settings.setValue("rightFolder", rightPanel.folder)
        onMediaWritten: path => page.noteWritten(path)
        onFailed: text => page.showError(text)
        onFilesystemNeeded: (row, choices, text) => profileDialog.ask(rightPanel, row, choices, text)
    }

    Settings {
        id: settings
        category: "FileManager"
    }

    Component.onCompleted: {
        const start = ZxEmulator.dataPath;
        const left = settings.value("leftFolder", start);
        const right = settings.value("rightFolder", start);
        rightActive = String(settings.value("rightActive", false)) === "true";
        leftPanel.folder = left;
        rightPanel.folder = right;
        Qt.callLater(focusActive);
    }
    Component.onDestruction: {
        if (sdCardChanged)
            ZxEmulator.hardReset();
    }

    GamepadNavigation {
        gamepad: Gamepad { deviceId: ZxEmulator.gamepadId }
        buttonBKey: Qt.Key_Escape
        // In the viewer X copies and Y goes to an address (saves a screen)
        buttonXKey: editorDialog.visible ? Qt.Key_F2 : page.popupOpen() ? Qt.Key_unknown
                                         : viewer.visible ? Qt.Key_Copy : Qt.Key_F2
        buttonYKey: page.popupOpen() ? Qt.Key_unknown : viewer.visible ? Qt.Key_Go : Qt.Key_Insert
        buttonL1Key: Qt.Key_PageUp
        buttonR1Key: Qt.Key_PageDown
        buttonL2Key: Qt.Key_BracketLeft
        buttonR2Key: Qt.Key_BracketRight
        buttonSelectKey: Qt.Key_F10
        buttonStartKey: Qt.Key_Menu
        buttonGuideKey: Qt.Key_unknown
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.sp(3)
        spacing: Theme.sp(2)

        // One panel at a time on a narrow screen: these switch between the two
        TabBar {
            objectName: "fileManagerPanels"
            visible: !page.twin
            Layout.fillWidth: true
            focusPolicy: Qt.NoFocus
            currentIndex: page.rightActive ? 1 : 0
            Repeater {
                model: [leftPanel, rightPanel]
                delegate: TabButton {
                    required property FileManagerPanel modelData
                    required property int index
                    focusPolicy: Qt.NoFocus
                    implicitHeight: Theme.touchTarget
                    text: (modelData.location.replace(/\/+$/, "").split("/").pop() || "/")
                    Accessible.name: index ? qsTr("Right panel: %1").arg(modelData.location)
                                           : qsTr("Left panel: %1").arg(modelData.location)
                    onClicked: page.setRight(index === 1)
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: Theme.sp(2)

            FileManagerPanelView {
                id: leftView
                objectName: "fileManagerLeft"
                panel: leftPanel
                active: !page.rightActive
                focus: !page.rightActive
                visible: page.twin || !page.rightActive
                Layout.fillWidth: true
                Layout.fillHeight: true
                onClicked: page.setRight(false)
                onOpenRequested: {
                    page.setRight(false);
                    page.openCurrent();
                }
            }
            FileManagerPanelView {
                id: rightView
                objectName: "fileManagerRight"
                panel: rightPanel
                active: page.rightActive
                focus: page.rightActive
                visible: page.twin || page.rightActive
                Layout.fillWidth: true
                Layout.fillHeight: true
                onClicked: page.setRight(true)
                onOpenRequested: {
                    page.setRight(true);
                    page.openCurrent();
                }
            }
        }

        FancyText {
            objectName: "fileManagerMessage"
            Layout.fillWidth: true
            visible: text !== ""
            text: page.message || page.notice
            wrapMode: Text.Wrap
            maximumLineCount: 3
            color: page.message ? Theme.danger : Theme.textMuted
        }
    }

    FileViewerPage {
        id: viewer
        objectName: "fileViewer"
        anchors.fill: parent
        panel: page.activePanel
        onClosed: page.focusActive()
        onGoToRequested: page.askAddress()
        onSaveRequested: page.saveScreen()
    }

    FormDialog {
        id: addressDialog
        objectName: "fileManagerAddress"
        title: qsTr("Go to address")
        acceptText: qsTr("Go")
        returnFocus: viewer
        initialFocus: addressField
        acceptable: addressField.text.trim().length > 0
        onAboutToShow: addressField.text = ""
        commit: () => {
            const address = page.activePanel.parseAddress(addressField.text);
            if (address < 0) {
                errorMessage = qsTr("Type an address: 23803, or hex as $5CFB, 0x5CFB or 5CFBh.");
                return false;
            }
            if (!viewer.goTo(address)) {
                errorMessage = viewer.status;
                return false;
            }
            return true;
        }
        Label {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            color: Theme.text
            text: qsTr("Address: decimal (23803), or hex ($5CFB, 0x5CFB, 5CFBh)")
        }
        FancyTextField {
            id: addressField
            objectName: "fileManagerAddressField"
            Layout.fillWidth: true
            mono: true
            onAccepted: addressDialog.save()
        }
    }

    Menu {
        id: actionsMenu
        objectName: "fileManagerActions"
        property var entries: []
        parent: Overlay.overlay
        popupType: Popup.Item
        modal: true
        x: (parent.width - width) / 2
        y: Math.max(Theme.sp(4), (parent.height - height) / 2)
        width: Math.min(parent.width - Theme.sp(6), Theme.sp(110))
        // Some actions open dialogs: release the menu's grab before running them.
        exit: Transition {}
        background: Rectangle {
            implicitWidth: Theme.sp(110)
            color: Theme.raised
            radius: Theme.radiusSmall
            border.color: Theme.outline
        }
        onOpened: currentIndex = 0
        onClosed: if (!page.popupOpen() && !viewer.visible) page.focusActive()
        Repeater {
            model: actionsMenu.entries
            delegate: MenuItem {
                required property var modelData
                text: modelData.text
                enabled: modelData.enabled !== false
                implicitHeight: Theme.touchScreenUi ? Theme.touchTarget : Math.round(Theme.rowHeight * 0.85)
                onTriggered: Qt.callLater(modelData.action)
            }
        }
    }

    FormDialog {
        id: nameDialog
        objectName: "fileManagerName"
        property var submit: null
        property bool image: false
        returnFocus: page.activeView
        initialFocus: nameField
        acceptText: qsTr("OK")
        acceptable: nameField.text.trim().length > 0
        commit: () => {
            const ok = submit(nameField.text.trim());
            if (!ok)
                errorMessage = page.lastError;
            return ok;
        }
        function ask(title: string, label: string, text: string, image: bool, action: var) {
            nameDialog.title = title;
            nameLabel.text = label;
            nameField.text = text;
            nameDialog.image = image;
            submit = action;
            open();
        }
        onOpened: nameField.selectAll()
        Label {
            visible: nameDialog.image
            text: qsTr("Format")
            color: Theme.text
        }
        NavigationComboBox {
            id: formatBox
            objectName: "fileManagerFormat"
            visible: nameDialog.image
            Layout.fillWidth: true
            model: page.activePanel.imageFormats()
            textRole: "text"
        }
        Label {
            id: nameLabel
            color: Theme.text
        }
        FancyTextField {
            id: nameField
            objectName: "fileManagerNameField"
            Layout.fillWidth: true
            onAccepted: nameDialog.save()
        }
    }

    FormDialog {
        id: copyDialog
        objectName: "fileManagerCopy"
        property bool move: false
        property var names: []
        // Asked once this dialog has closed, so the question keeps the focus
        property var overwrite: null
        title: move ? qsTr("Move") : qsTr("Copy")
        acceptText: move ? qsTr("Move") : qsTr("Copy")
        returnFocus: overwrite ? null : page.activeView
        // Return (the gamepad's A) copies; a focused button would only take Space
        initialFocus: names.length === 1 ? copyName : copyText
        acceptable: names.length !== 1 || copyName.text.trim().length > 0
        onAboutToShow: copyName.text = names.length === 1 ? names[0] : ""
        onClosed: {
            const question = overwrite;
            if (question)
                Qt.callLater(() => {
                    copyDialog.overwrite = null;
                    confirmDialog.ask(question.text, question.action);
                });
        }
        commit: () => {
            const name = names.length === 1 ? copyName.text.trim() : "";
            const conflicts = page.activePanel.conflicts(page.row, page.otherPanel, name);
            if (conflicts.length) {
                const move = copyDialog.move;
                overwrite = { text: qsTr("Overwrite %1?").arg(page.describe(conflicts)),
                              action: () => page.finishCopy(move, name, true) };
                return true;
            }
            const ok = page.finishCopy(move, name, false);
            if (!ok)
                errorMessage = page.lastError;
            return ok;
        }
        FancyText {
            id: copyText
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            text: (copyDialog.move ? qsTr("Move %1 to %2") : qsTr("Copy %1 to %2"))
                  .arg(page.describe(copyDialog.names)).arg(page.otherPanel.location)
            Keys.onReturnPressed: copyDialog.save()
            Keys.onEnterPressed: copyDialog.save()
        }
        FancyTextField {
            id: copyName
            objectName: "fileManagerCopyName"
            visible: copyDialog.names.length === 1
            Layout.fillWidth: true
            onAccepted: copyDialog.save()
        }
    }

    Dialog {
        id: confirmDialog
        objectName: "fileManagerConfirm"
        property var yesAction: null
        property string text
        function ask(question: string, action: var) {
            text = question;
            yesAction = action;
            open();
        }
        parent: Overlay.overlay
        popupType: Popup.Item
        anchors.centerIn: parent
        width: Math.min(parent.width - Theme.sp(6), Theme.sp(120))
        modal: true
        focus: true
        title: qsTr("Confirm")
        standardButtons: Dialog.Yes | Dialog.No
        background: Rectangle {
            color: Theme.raised
            radius: Theme.radius
            border.color: Theme.outline
        }
        onOpened: question.forceActiveFocus()
        onAccepted: if (yesAction) yesAction()
        onClosed: if (!page.popupOpen()) page.focusActive()
        FancyText {
            id: question
            width: confirmDialog.availableWidth
            wrapMode: Text.Wrap
            text: confirmDialog.text
            Keys.onReturnPressed: confirmDialog.accept()
            Keys.onEnterPressed: confirmDialog.accept()
        }
    }

    FormDialog {
        id: attributesDialog
        objectName: "fileManagerAttributes"
        property var entries: []
        returnFocus: page.activeView
        commit: () => {
            const values = {};
            for (let i = 0; i < attributeFields.count; ++i) {
                const item = attributeFields.itemAt(i) as AttributeField;
                if (!item.valid)
                    return false;
                values[item.modelData.key] = item.value;
            }
            const ok = page.run(() => page.activePanel.setAttributes(page.row, values));
            if (!ok)
                errorMessage = page.lastError;
            return ok;
        }
        onOpened: {
            for (let i = 0; i < attributeFields.count; ++i) {
                const item = attributeFields.itemAt(i) as AttributeField;
                if (item.focusFirst())
                    return;
            }
        }
        Repeater {
            id: attributeFields
            model: attributesDialog.entries
            delegate: AttributeField {}
        }
    }

    component AttributeField: ColumnLayout {
        id: field
        required property var modelData
        readonly property string kind: modelData.kind
        readonly property bool valid: kind !== "number" || numberField.acceptableInput || modelData.readOnly
        readonly property var value: kind === "bool" ? (check.checked ? 1 : 0)
                                   : kind === "choice" ? (choiceBox.currentIndex >= 0 ? modelData.choices[choiceBox.currentIndex].value : modelData.value)
                                   : Number(numberField.text)
        function focusFirst(): bool {
            if (modelData.readOnly)
                return false;
            (kind === "bool" ? check : kind === "choice" ? choiceBox : numberField).forceActiveFocus(Qt.TabFocusReason);
            return true;
        }
        Layout.fillWidth: true
        spacing: Theme.sp(1)
        Label {
            visible: field.kind !== "bool"
            Layout.fillWidth: true
            text: field.modelData.label
            color: Theme.text
            wrapMode: Text.Wrap
        }
        CheckBox {
            id: check
            visible: field.kind === "bool"
            enabled: !field.modelData.readOnly
            text: field.modelData.label
            checked: field.modelData.value !== 0
            Keys.onReturnPressed: toggle()
            Keys.onEnterPressed: toggle()
        }
        NavigationComboBox {
            id: choiceBox
            visible: field.kind === "choice"
            enabled: !field.modelData.readOnly
            Layout.fillWidth: true
            model: field.modelData.choices ?? []
            textRole: "text"
            currentIndex: (field.modelData.choices ?? []).findIndex(choice => choice.value === field.modelData.value)
        }
        FancyTextField {
            id: numberField
            visible: field.kind === "number"
            readOnly: field.modelData.readOnly
            Layout.fillWidth: true
            text: String(field.modelData.value)
            inputMethodHints: Qt.ImhDigitsOnly
            validator: IntValidator {
                bottom: field.modelData.minimum
                top: field.modelData.maximum
            }
            onAccepted: attributesDialog.save()
        }
    }

    FormDialog {
        id: editorDialog
        objectName: "fileManagerEditor"
        property int row: -1
        property bool basic: false
        property string note
        returnFocus: page.activeView
        initialFocus: editor
        width: parent.width - Theme.sp(8)
        height: parent.height - Theme.sp(8)
        commit: () => {
            const ok = page.run(() => page.activePanel.saveText(editorDialog.row, editor.text));
            if (!ok)
                errorMessage = page.lastError;
            return ok;
        }
        Label {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            color: Theme.textMuted
            font.pixelSize: Theme.caption
            text: (editorDialog.basic ? qsTr("Type keywords plainly. \\{KEYWORD} and \\xHH escapes keep the program's exact bytes.") + " " : "")
                  + (editorDialog.note ? editorDialog.note + " " : "") + qsTr("F2, Ctrl+S or the gamepad's X saves.")
        }
        ScrollView {
            Layout.fillWidth: true
            Layout.preferredHeight: editorDialog.availableHeight - Theme.sp(12)
            TextArea {
                id: editor
                objectName: "fileManagerEditorText"
                font.family: Theme.monoFamily
                font.pixelSize: Theme.body
                wrapMode: TextEdit.NoWrap
                inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText
                // Return and the arrows edit the text: save from the keyboard or the X button
                Keys.onPressed: event => {
                    if (event.key === Qt.Key_F2 || (event.key === Qt.Key_S && (event.modifiers & Qt.ControlModifier))) {
                        editorDialog.save();
                        event.accepted = true;
                    }
                }
            }
        }
    }

    Dialog {
        id: profileDialog
        objectName: "fileManagerProfiles"
        property FileManagerPanel panel
        property int row: -1
        property var choices: []
        function ask(panel: FileManagerPanel, row: int, choices: var, text: string) {
            profileDialog.panel = panel;
            profileDialog.row = row;
            profileDialog.choices = choices;
            open();
        }
        parent: Overlay.overlay
        popupType: Popup.Item
        anchors.centerIn: parent
        width: Math.min(parent.width - Theme.sp(6), Theme.sp(120))
        modal: true
        focus: true
        title: qsTr("Which filesystem is on this disk?")
        background: Rectangle {
            color: Theme.raised
            radius: Theme.radius
            border.color: Theme.outline
        }
        onOpened: profiles.forceActiveFocus()
        onClosed: if (!page.popupOpen()) page.focusActive()
        contentItem: Item {
            implicitHeight: Theme.rowHeight * Math.min(6, profileDialog.choices.length) + Theme.sp(4)
            OptionList {
                id: profiles
                model: profileDialog.choices
                textOf: choice => qsTr("%1 (%n file(s))", "", choice.files).arg(choice.profile)
                onChosen: index => {
                    const panel = profileDialog.panel;
                    const row = profileDialog.row;
                    const profile = profileDialog.choices[index].profile;
                    profileDialog.close();
                    if (panel.open(row, profile))
                        (panel === rightPanel ? rightView : leftView).select(0);
                }
            }
        }
    }
}
