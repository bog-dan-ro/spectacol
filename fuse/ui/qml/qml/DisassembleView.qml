/*
    Copyright (c) 2015, BogDan Vatra <bogdan@kde.org>

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

import QtQuick 2.5
import QtQuick.Layouts 1.1
import QtQuick.Window 2.0

ListView {
    id: view
    clip: true

    snapMode: ListView.SnapToItem
    highlightFollowsCurrentItem: true

    currentIndex: disassambleModel.delta

    VisualDataModel {
        id: visualModel
        model: disassambleModel
        delegate: Rectangle {
            property color paper: view.currentIndex !== index ? background : selectedBackground
            property color ink: view.currentIndex !== index ? foreground : selectedForeground

            width: view.width
            height: 7 * Screen.pixelDensity
            color: paper
            RowLayout {
                anchors.fill: parent
                FancyText {
                    Layout.fillHeight: true
                    Layout.fillWidth: false
                    horizontalAlignment: Text.AlignRight
                    fontSize: 4
                    style: Text.Normal
                    Layout.preferredWidth: 14 * Screen.pixelDensity
                    color: ink
                    text: model.addressText
                }

                Item { Layout.fillWidth: false;width: 1.5 * Screen.pixelDensity }

                FancyText {
                    Layout.fillHeight: true
                    Layout.fillWidth: false
                    Layout.preferredWidth: 15 * Screen.pixelDensity
                    horizontalAlignment: Text.AlignHCenter
                    fontSize: 2.5
                    style: Text.Normal
                    color: ink
                    wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                    text: model.bytes
                }

                Item { Layout.fillWidth: false;width: 1.5 * Screen.pixelDensity }

                FancyText {
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignLeft
                    fontSize: 4
                    color: ink
                    style: Text.Normal
                    text: model.disassable
                }
            }
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    view.focus = true;
                    view.currentIndex = model.index;
                }
                onDoubleClicked: fuse.addBreakpoint(model.address)
                onPressAndHold: fuse.addBreakpoint(model.address)
            }
        }
    }
    Keys.onUpPressed: {
        if (view.currentIndex > 1)
            decrementCurrentIndex();
        else {
            fuse.disassambleFetchUp(10);
            decrementCurrentIndex();
        }
    }

    Keys.onDownPressed: incrementCurrentIndex()
    Keys.onPressed: {
        event.accepted = true;
        switch (event.key) {
        case Qt.Key_Escape:
            pageLoader.source = "";
            break;
        }
    }
    model: visualModel
}
