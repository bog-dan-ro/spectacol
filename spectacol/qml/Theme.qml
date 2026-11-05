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

pragma Singleton

import QtQuick
import QtQuick.Window
import Spectacol

// Spectacol's design tokens: colour roles, the type scale, spacing and motion.
// The UI is dark by design: it always sits on top of the game.
QtObject {
    // "Large" scales everything by 1.5, for a TV across the room
    readonly property real uiScale: ZxEmulator.settings.largeUi ? 1.5 : 1.0
    readonly property bool smallScreen: Math.min(Screen.height, Screen.width) / Screen.pixelDensity < 80
    readonly property bool mobileUi: Qt.platform.os === "android" || Qt.platform.os === "ios"
    readonly property bool gamepadHints: ZxEmulator.gamepadId !== -1
    // Android TV remotes report D-pad/Select/Back keys without a Gamepad device.
    readonly property bool remoteHints: mobileUi && !ZxEmulator.touchscreen && !gamepadHints
    readonly property bool keyboardHints: !mobileUi && !gamepadHints
    readonly property bool showHintBar: !mobileUi || gamepadHints || remoteHints
    readonly property int headerHeight: rowHeight

    // The overlay (gamepad, touch, TV) or the desktop layout (menu bar, docked panels)
    readonly property bool desktopMode: {
        switch (ZxEmulator.settings.uiMode) {
        case ZxSettings.OverlayUi: return false;
        case ZxSettings.DesktopUi: return true;
        }
        return Qt.platform.os !== "android" && Qt.platform.os !== "ios" && !ZxEmulator.touchscreen
                && ZxEmulator.gamepadId === -1;
    }

    // Colour roles
    readonly property color surface: "#f214161b"     // 95%
    readonly property color raised: "#20242c"
    readonly property color scrim: "#a807080b"       // 66%
    readonly property color outline: "#353b47"
    readonly property color text: "#eceef2"
    readonly property color textMuted: "#a4aab6"
    readonly property color accent: "#00d7d7"        // the 128K menu's cyan
    readonly property color textOnAccent: "#001a1b"
    readonly property color accentFill: "#2400d7d7"  // 14%: the selected row
    readonly property color danger: "#ff6464"
    readonly property color warning: "#e6d64a"
    readonly property color success: "#48d672"
    // The 128K's rainbow stripe, the one brand flourish
    readonly property var stripe: ["#d70000", "#d7d700", "#00d700", "#00d7d7"]
    // The gamepad's face buttons
    readonly property color padA: "#6fdc8c"
    readonly property color padB: "#ff7b72"
    readonly property color padX: "#79b8ff"
    readonly property color padY: "#f2cc60"

    // Type scale: a major third from 16 px. The platform's UI font; monospace only for code
    readonly property int caption: Math.round(13 * uiScale)
    readonly property int body: Math.round(16 * uiScale)
    readonly property int title: Math.round(20 * uiScale)
    readonly property int pageTitle: Math.round(25 * uiScale)
    readonly property int display: Math.round(31 * uiScale)
    readonly property string monoFamily: Qt.platform.os === "android" ? "Droid Sans Mono" : "Monospace"

    // Spacing: a 4 px grid, sp(4) is 16 px
    function sp(steps: real): int { return Math.round(4 * steps * uiScale); }
    readonly property int rowHeight: Math.round(48 * uiScale)
    readonly property int radius: Math.round(10 * uiScale)
    readonly property int radiusSmall: Math.round(6 * uiScale)
    readonly property int line: Math.max(1, Math.round(uiScale))
    readonly property int focusLine: Math.max(2, Math.round(2 * uiScale))

    // Motion: short, decelerating in, accelerating out; nothing with "Reduce motion"
    readonly property bool reduceMotion: ZxEmulator.settings.reduceMotion
    readonly property int enterDuration: reduceMotion ? 0 : 200
    readonly property int exitDuration: reduceMotion ? 0 : 150
    readonly property int slide: Math.round(24 * uiScale)
}
