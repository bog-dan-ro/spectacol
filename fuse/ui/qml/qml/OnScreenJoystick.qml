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

import QtQuick 2.6
import QtQuick.Window 2.0
import QtGamepad 1.0
import "private"

Item {
    anchors.fill: parent
    anchors.margins: (TextSizes.smallScreen ? 2.5 :  5) * Screen.pixelDensity
    // Directions
    Rectangle {
        property real angle : 0
        property real distance : 0

        anchors.bottom: parent.bottom
        anchors.left: parent.left

        width: (TextSizes.smallScreen ? 20 : 30) * Screen.pixelDensity // 1.5 , 3cm
        height: width

        color: Qt.rgba(0.5, 0.5, 0.5, 0.75)
        radius: width / 2
        ParallelAnimation {
            id: returnAnimation
            NumberAnimation { target: thumb.anchors; property: "horizontalCenterOffset";
                to: 0; duration: 100; easing.type: Easing.OutSine }
            NumberAnimation { target: thumb.anchors; property: "verticalCenterOffset";
                to: 0; duration: 100; easing.type: Easing.OutSine }
        }

        MultiPointTouchArea {
            id: touch
            anchors.fill: parent
            touchPoints: TouchPoint {
                property real fingerAngle : Math.atan2(x, y)
                property int mcx : x - touch.width * 0.5
                property int mcy : y - touch.height * 0.5
                property bool fingerOutOfBounds : fingerDistance2 < distanceBound2
                property real fingerDistance2 : mcx * mcx + mcy * mcy
                property real distanceBound : touch.width * 0.5
                property real distanceBound2 : distanceBound * distanceBound
                onPressedChanged: pressed ? returnAnimation.stop() : returnAnimation.restart()
                onAreaChanged: {
                    if (fingerOutOfBounds) {
                        thumb.anchors.horizontalCenterOffset = mcx
                        thumb.anchors.verticalCenterOffset = mcy
                    } else {
                        var angle = Math.atan2(mcy, mcx)
                        thumb.anchors.horizontalCenterOffset = Math.cos(angle) * distanceBound
                        thumb.anchors.verticalCenterOffset = Math.sin(angle) * distanceBound
                    }
                }
            }
        }

        Rectangle {
            id: thumb
            color: Qt.rgba(0, 0.5, 0.5, 0.75)
            width: (TextSizes.smallScreen ? 7 : 15) * Screen.pixelDensity // 0.7, 1.5 cm
            height: width
            radius: width / 2
            anchors {
                centerIn: parent
                onHorizontalCenterOffsetChanged: fuse.gamepadAxisEvent(GamepadManager.AxisLeftX, anchors.horizontalCenterOffset / parent.radius)
                onVerticalCenterOffsetChanged: fuse.gamepadAxisEvent(GamepadManager.AxisLeftY, anchors.verticalCenterOffset / parent.radius)
            }
        }
    }

    // Fire button
    Rectangle {
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        color: "red"
        opacity: 0.85
        width: (TextSizes.smallScreen ? 7 : 15) * Screen.pixelDensity // 0.7, 1.5 cm
        height: width
        radius: width / 2
        MultiPointTouchArea {
            anchors.fill: parent
            touchPoints: TouchPoint {
                onPressedChanged: pressed ? fuse.gamepadButtonPressEvent(GamepadManager.ButtonA) : fuse.gamepadButtonReleaseEvent(GamepadManager.ButtonA)
            }
        }
    }
}
