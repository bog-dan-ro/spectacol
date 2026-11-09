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

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtCore
import QtGamepadLegacy
import Spectacol

// @scope main.qml
Sheet {
    id: page
    crumb: qsTr("Library")
    title: qsTr("Search online")
    preferredWidth: Theme.sp(260)
    readonly property var catalogue: ZxEmulator.onlineGamesModel
    readonly property bool narrow: panelWidth < Theme.sp(120)
    property int pendingIndex: -1
    hints: [
        {
            pad: "A",
            key: "Enter",
            text: qsTr("Details"),
            action: () => page.openDetails(results.currentIndex)
        },
        {
            pad: "X",
            key: "F3",
            text: qsTr("Search"),
            action: () => query.forceActiveFocus()
        },
        {
            pad: "Y",
            key: "F4",
            text: qsTr("Source"),
            action: () => source.forceActiveFocus()
        },
        {
            pad: "B",
            key: "Esc",
            text: qsTr("Close"),
            action: () => pageLoader.source = ""
        }
    ]
    Keys.onPressed: event => {
        if (event.key === Qt.Key_F3 || event.key === Qt.Key_F4) {
            (event.key === Qt.Key_F3 ? query : source).forceActiveFocus();
            event.accepted = true;
        }
    }
    Settings {
        id: preferences
        category: "OnlineSearch"
        property alias searchString: query.text
        property int sourceIndex: 0
    }
    GamepadNavigation {
        gamepad: Gamepad {
            deviceId: ZxEmulator.gamepadId
        }
        buttonAKey: Qt.Key_Return
        buttonBKey: Qt.Key_Escape
        buttonXKey: Qt.Key_F3
        buttonYKey: Qt.Key_F4
    }
    Connections {
        target: page.catalogue
        function onDownloaded(path) {
            if (ZxEmulator.copyToFavourites(path))
                pageLoader.source = "";
        }
        function onPageLoaded(firstPage) {
            if (firstPage)
                results.currentIndex = results.count ? 0 : -1;
            else if (page.pendingIndex >= 0) {
                results.currentIndex = Math.min(page.pendingIndex, results.count - 1);
                results.positionViewAtIndex(results.currentIndex, GridView.Contain);
            }
            page.pendingIndex = -1;
        }
        function onStateChanged() {
            if (page.catalogue.errorString.length)
                page.pendingIndex = -1;
            if (detailDialog.awaitingFiles && detailDialog.opened && !page.catalogue.loadingDetails) {
                detailDialog.awaitingFiles = false;
                media.currentIndex = media.count ? 0 : -1;
                (media.count ? media : closeDetails).forceActiveFocus();
            }
        }
    }
    function performSearch() {
        pendingIndex = -1;
        catalogue.search(query.text);
        results.forceActiveFocus();
    }
    function openDetails(index) {
        if (index < 0 || index >= results.count)
            return;
        pendingIndex = -1;
        results.currentIndex = index;
        results.forceActiveFocus();
        detailDialog.selectedTitle = catalogue.data(catalogue.index(index, 0), Qt.DisplayRole);
        detailDialog.awaitingFiles = true;
        catalogue.select(index);
        detailDialog.open();
    }
    function playSelection() {
        if (media.currentIndex >= 0 && !catalogue.downloading && !catalogue.loadingDetails)
            catalogue.download(media.currentIndex, ZxEmulator.dataPath + "/Downloaded/Online");
    }
    Component.onCompleted: {
        ZxEmulator.processInputEvents = false;
        catalogue.source = preferences.sourceIndex;
        if (query.text.length)
            performSearch();
        else
            query.forceActiveFocus();
    }
    Component.onDestruction: {
        catalogue.cancel();
        ZxEmulator.processInputEvents = true;
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.sp(Theme.touchScreenUi ? 2 : 5)
        spacing: Theme.sp(Theme.touchScreenUi ? 1 : 3)
        GridLayout {
            columns: page.narrow ? 2 : 3
            Layout.fillWidth: true
            ComboBox {
                id: source
                Layout.preferredWidth: Theme.sp(48)
                Layout.columnSpan: page.narrow ? 2 : 1
                Layout.fillWidth: page.narrow
                model: page.catalogue.sources
                currentIndex: preferences.sourceIndex
                Accessible.name: qsTr("Search source")
                onActivated: {
                    preferences.sourceIndex = currentIndex;
                    page.catalogue.source = currentIndex;
                    page.performSearch();
                }
            }
            FancyTextField {
                id: query
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                placeholderText: qsTr("Title, or one letter to browse")
                selectByMouse: true
                maximumLength: 200
                onAccepted: page.performSearch()
                KeyNavigation.down: results
            }
            Button {
                text: qsTr("Search")
                onClicked: page.performSearch()
            }
        }
        Text {
            Layout.fillWidth: true
            text: page.catalogue.searching ? qsTr("Searching %1…").arg(source.currentText) : page.catalogue.searched ? qsTr("%1 of %2 titles · %3").arg(results.count).arg(page.catalogue.total).arg(source.currentText) : qsTr("Search by title, or leave the field empty to browse the catalogue.")
            wrapMode: Text.Wrap
            font.pixelSize: Theme.caption
            color: Theme.textMuted
        }
        Text {
            Layout.fillWidth: true
            visible: text.length > 0 && !detailDialog.visible
            text: page.catalogue.errorString
            textFormat: Text.PlainText
            wrapMode: Text.Wrap
            font.pixelSize: Theme.body
            color: Theme.warning
        }
        GridView {
            id: results
            objectName: "onlineResults"
            Layout.fillWidth: true
            Layout.fillHeight: true
            readonly property int columns: Math.max(1, Math.floor(width / Theme.sp(page.narrow ? 44 : 64)))
            cellWidth: width / columns
            cellHeight: Theme.sp(24)
            clip: true
            model: page.catalogue
            keyNavigationWraps: false
            boundsBehavior: Flickable.StopAtBounds
            highlightMoveDuration: Theme.reduceMotion ? 0 : 100
            ScrollBar.vertical: ScrollBar {}
            Keys.onReturnPressed: page.openDetails(currentIndex)
            Keys.onEnterPressed: page.openDetails(currentIndex)
            Keys.onPressed: event => {
                const next = currentIndex + (event.key === Qt.Key_Down ? columns : 1);
                if ((event.key === Qt.Key_Down || event.key === Qt.Key_Right) && currentIndex >= 0 && next >= count && page.catalogue.hasMore) {
                    page.pendingIndex = next;
                    page.catalogue.loadMore();
                    event.accepted = true;
                }
            }
            delegate: Item {
                id: card
                required property int index
                required property string title
                required property string subtitle
                width: results.cellWidth
                height: results.cellHeight
                Rectangle {
                    anchors.fill: parent
                    anchors.margins: Theme.sp(1)
                    radius: Theme.radiusSmall
                    color: card.index === results.currentIndex ? Theme.accentFill : Theme.raised
                    border.color: Theme.accent
                    border.width: card.index === results.currentIndex ? Theme.focusLine : 0
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: Theme.sp(3)
                        spacing: Theme.sp(1)
                        Text {
                            Layout.fillWidth: true
                            text: card.title
                            textFormat: Text.PlainText
                            wrapMode: Text.Wrap
                            maximumLineCount: 2
                            elide: Text.ElideRight
                            font.pixelSize: Theme.body
                            color: Theme.text
                        }
                        Text {
                            Layout.fillWidth: true
                            visible: text.length > 0
                            text: card.subtitle
                            textFormat: Text.PlainText
                            elide: Text.ElideRight
                            font.pixelSize: Theme.caption
                            color: Theme.textMuted
                        }
                    }
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: page.openDetails(card.index)
                }
            }
            Text {
                anchors.centerIn: parent
                width: parent.width
                visible: page.catalogue.searched && !page.catalogue.searching && results.count === 0
                text: qsTr("No results. Try another title or source.")
                wrapMode: Text.Wrap
                horizontalAlignment: Text.AlignHCenter
                font.pixelSize: Theme.body
                color: Theme.textMuted
            }
        }
        Button {
            visible: page.catalogue.hasMore
            enabled: !page.catalogue.searching
            text: qsTr("Load more")
            onClicked: page.catalogue.loadMore()
            KeyNavigation.up: results
        }
    }

    Dialog {
        id: detailDialog
        objectName: "onlineDetails"
        property string selectedTitle
        property bool awaitingFiles: false
        parent: Overlay.overlay
        x: Math.round((parent.width - width) / 2)
        y: Math.round((parent.height - height) / 2)
        width: Math.min(page.width - Theme.sp(8), Theme.sp(165))
        height: Math.min(page.height - Theme.sp(8), Theme.sp(170))
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        padding: Theme.sp(5)
        Overlay.modal: Rectangle {
            color: Theme.scrim
        }
        onOpened: {
            if (!page.catalogue.loadingDetails) {
                awaitingFiles = false;
                media.currentIndex = media.count ? 0 : -1;
                (media.count ? media : closeDetails).forceActiveFocus();
            } else {
                closeDetails.forceActiveFocus();
            }
        }
        onClosed: {
            awaitingFiles = false;
            page.catalogue.cancelSelection();
            results.forceActiveFocus();
        }
        background: Rectangle {
            color: Theme.raised
            radius: Theme.radius
            border.width: Theme.line
            border.color: Theme.outline
        }
        header: Text {
            padding: Theme.sp(5)
            text: page.catalogue.details.title || detailDialog.selectedTitle
            textFormat: Text.PlainText
            wrapMode: Text.Wrap
            font.pixelSize: Theme.title
            color: Theme.text
        }
        contentItem: ColumnLayout {
            spacing: Theme.sp(3)
            Keys.onPressed: event => {
                if (event.key === Qt.Key_F3) {
                    if (page.catalogue.details.website.toString().length)
                        Qt.openUrlExternally(page.catalogue.details.website);
                    event.accepted = true;
                }
            }
            ScrollView {
                id: detailScroll
                Layout.fillWidth: true
                Layout.fillHeight: true
                contentWidth: availableWidth
                clip: true
                ColumnLayout {
                    width: detailScroll.availableWidth
                    spacing: Theme.sp(3)
                    Text {
                        Layout.fillWidth: true
                        text: page.catalogue.loadingDetails ? qsTr("Loading title…") : page.catalogue.details.subtitle
                        textFormat: Text.PlainText
                        wrapMode: Text.Wrap
                        font.pixelSize: Theme.caption
                        color: Theme.textMuted
                    }
                    Image {
                        id: fullPreview
                        objectName: "onlinePreview"
                        Layout.fillWidth: true
                        Layout.preferredHeight: Math.min(width * 0.625, Math.max(Theme.sp(20), detailScroll.availableHeight - Theme.sp(16)))
                        source: detailDialog.visible ? page.catalogue.details.screenshot : ""
                        sourceSize: Qt.size(640, 480)
                        visible: source.toString().length > 0
                        asynchronous: true
                        fillMode: Image.PreserveAspectFit
                        Text {
                            anchors.centerIn: parent
                            visible: fullPreview.status === Image.Error
                            text: qsTr("Screenshot unavailable")
                            font.pixelSize: Theme.caption
                            color: Theme.textMuted
                        }
                    }
                    Text {
                        Layout.fillWidth: true
                        text: page.catalogue.details.description
                        textFormat: Text.StyledText
                        wrapMode: Text.Wrap
                        font.pixelSize: Theme.body
                        color: Theme.text
                    }
                }
            }
            Text {
                Layout.fillWidth: true
                visible: text.length > 0
                text: page.catalogue.errorString
                textFormat: Text.PlainText
                wrapMode: Text.Wrap
                font.pixelSize: Theme.body
                color: Theme.warning
            }
            ListView {
                id: media
                objectName: "onlineMedia"
                Layout.fillWidth: true
                Layout.preferredHeight: Math.min(count, 3) * Theme.rowHeight
                visible: count > 0
                enabled: !page.catalogue.downloading
                model: page.catalogue.details.files
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.vertical: ScrollBar {}
                keyNavigationWraps: false
                Keys.onReturnPressed: page.playSelection()
                Keys.onEnterPressed: page.playSelection()
                delegate: Item {
                    id: file
                    required property int index
                    required property var modelData
                    width: media.width
                    height: Theme.rowHeight
                    Rectangle {
                        anchors.fill: parent
                        radius: Theme.radiusSmall
                        color: file.index === media.currentIndex ? Theme.accentFill : "transparent"
                        border.width: file.index === media.currentIndex ? Theme.focusLine : 0
                        border.color: Theme.accent
                    }
                    Text {
                        anchors.fill: parent
                        anchors.margins: Theme.sp(3)
                        text: file.modelData.label
                        textFormat: Text.PlainText
                        elide: Text.ElideMiddle
                        verticalAlignment: Text.AlignVCenter
                        font.pixelSize: Theme.body
                        color: Theme.text
                    }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            media.currentIndex = file.index;
                            media.forceActiveFocus();
                        }
                    }
                }
            }
            Text {
                Layout.fillWidth: true
                visible: !page.catalogue.loadingDetails && !page.catalogue.details.files.length && !page.catalogue.errorString.length
                text: qsTr("No supported media files are listed for this title. Check the source page for availability.")
                wrapMode: Text.Wrap
                font.pixelSize: Theme.body
                color: Theme.textMuted
            }
            RowLayout {
                Layout.fillWidth: true
                Button {
                    id: play
                    Layout.fillWidth: true
                    text: page.catalogue.downloading ? qsTr("Downloading…") : qsTr("Play")
                    enabled: media.count > 0 && !page.catalogue.downloading
                    onClicked: page.playSelection()
                    Keys.onReturnPressed: page.playSelection()
                    KeyNavigation.up: media
                    KeyNavigation.right: sourcePage
                    KeyNavigation.down: retryDetails
                }
                Button {
                    id: sourcePage
                    Layout.fillWidth: true
                    text: qsTr("Source page")
                    enabled: page.catalogue.details.website.toString().length > 0
                    onClicked: Qt.openUrlExternally(page.catalogue.details.website)
                    Keys.onReturnPressed: clicked()
                    KeyNavigation.left: play
                    KeyNavigation.right: closeDetails
                    KeyNavigation.up: media
                    KeyNavigation.down: retryDetails
                }
                Button {
                    id: closeDetails
                    Layout.fillWidth: true
                    text: qsTr("Back")
                    onClicked: detailDialog.close()
                    Keys.onReturnPressed: detailDialog.close()
                    KeyNavigation.left: sourcePage
                    KeyNavigation.up: media
                    KeyNavigation.down: retryDetails
                }
            }
            Button {
                id: retryDetails
                visible: page.catalogue.errorString.length > 0 && !page.catalogue.loadingDetails
                text: qsTr("Retry details")
                onClicked: {
                    detailDialog.awaitingFiles = true;
                    page.catalogue.select(results.currentIndex);
                }
                Keys.onReturnPressed: clicked()
                KeyNavigation.up: closeDetails
            }
        }
        footer: HintBar {
            hints: [
                {
                    pad: "A",
                    key: "Enter",
                    text: qsTr("Play selected file"),
                    action: () => page.playSelection()
                },
                {
                    pad: "B",
                    key: "Esc",
                    text: qsTr("Results"),
                    action: () => detailDialog.close()
                }
            ]
        }
    }
}
