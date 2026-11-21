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

Dialog {
    id: dialog

    property int pageIndex: 0
    readonly property var preview: visible ? ZxEmulator.zxPrinterPreviewPage(pageIndex) : ({})
    readonly property int pageCount: preview.pageCount ?? 0

    function movePage(delta: int) {
        const index = pageIndex + delta;
        if (index >= 0 && index < pageCount) {
            if ((index === 0 && previousButton.activeFocus)
                    || (index === pageCount - 1 && nextButton.activeFocus))
                closeButton.forceActiveFocus(Qt.TabFocusReason);
            paperView.cancelFlick();
            pageIndex = index;
            paperView.contentY = 0;
        }
    }

    function scroll(delta: int) {
        paperView.contentY = Math.max(0, Math.min(paperView.contentHeight - paperView.height,
                                                 paperView.contentY + delta * Theme.rowHeight));
    }

    parent: Overlay.overlay
    popupType: Popup.Item
    anchors.centerIn: parent
    width: Math.min(parent.width - Theme.sp(4), Theme.sp(140))
    height: Math.min(parent.height - Theme.sp(4), Theme.sp(190))
    title: qsTr("Print preview")
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape
    padding: Theme.sp(3)
    header: null

    onAboutToShow: {
        pageIndex = 0;
        paperView.contentY = 0;
    }
    onOpened: closeButton.forceActiveFocus(Qt.PopupFocusReason)

    Overlay.modal: Rectangle {
        color: Theme.scrim
    }
    background: Rectangle {
        color: Theme.raised
        radius: Theme.radius
        border.width: Theme.line
        border.color: Theme.outline
    }

    contentItem: ColumnLayout {
        spacing: Theme.sp(3)
        Keys.onLeftPressed: dialog.movePage(-1)
        Keys.onRightPressed: dialog.movePage(1)
        Keys.onUpPressed: dialog.scroll(-1)
        Keys.onDownPressed: dialog.scroll(1)
        Keys.onBackPressed: dialog.close()

        RowLayout {
            FancyText {
                Layout.fillWidth: true
                text: dialog.title
                font.pixelSize: Theme.title
                font.weight: Font.DemiBold
            }
            FancyText {
                visible: dialog.pageCount > 1
                text: qsTr("Page %1 of %2").arg(dialog.pageIndex + 1).arg(dialog.pageCount)
                color: Theme.textMuted
            }
        }

        Flickable {
            id: paperView
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: image.source.toString() !== "" && image.status !== Image.Error
            contentHeight: paper.height
            contentWidth: width
            boundsBehavior: Flickable.StopAtBounds
            clip: true
            ScrollBar.vertical: ScrollBar {}

            Rectangle {
                id: paper
                width: Math.max(0, paperView.width - Theme.sp(4))
                height: Math.max(paperView.height, image.height + Theme.sp(4))
                x: (paperView.width - width) / 2
                color: "white"

                Image {
                    id: image
                    objectName: "printPreviewImage"
                    x: Theme.sp(2)
                    y: Theme.sp(2)
                    width: Math.max(0, paper.width - Theme.sp(4))
                    height: sourceSize.width > 0 ? width * sourceSize.height / sourceSize.width : 0
                    source: dialog.preview.source ?? ""
                    sourceSize: dialog.preview.sourceSize ?? Qt.size(0, 0)
                    smooth: false
                    cache: false
                    Accessible.role: Accessible.Graphic
                    Accessible.name: qsTr("Captured printout")
                }
            }
        }
        FancyText {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: !paperView.visible
            text: qsTr("Couldn’t load the print preview.")
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
        }
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.sp(2)
            DialogButton {
                id: previousButton
                objectName: "printPreviewPrevious"
                Layout.fillWidth: true
                visible: dialog.pageCount > 1
                enabled: dialog.pageIndex > 0
                text: qsTr("Previous")
                onClicked: dialog.movePage(-1)
            }
            DialogButton {
                id: nextButton
                objectName: "printPreviewNext"
                Layout.fillWidth: true
                visible: dialog.pageCount > 1
                enabled: dialog.pageIndex + 1 < dialog.pageCount
                text: qsTr("Next")
                onClicked: dialog.movePage(1)
            }
            Item { Layout.fillWidth: true; visible: dialog.pageCount <= 1 }
            DialogButton {
                id: closeButton
                objectName: "printPreviewClose"
                text: qsTr("Close")
                onClicked: dialog.close()
            }
        }
    }
}
