/* Copyright (c) 2015-2026, BogDan Vatra. SPDX-License-Identifier: GPL-3.0-or-later */

import QtQuick
import QtGamepadLegacy as Legacy

Legacy.GamepadKeyNavigation {
    id: navigation

    property bool textInputActive: DesktopInput.visible
    property bool ready: false
    property bool repeating: false
    readonly property int stickDirection: {
        if (!ready || !active || !gamepad || Qt.application.state !== Qt.ApplicationActive)
            return 0;
        const left = Math.max(Math.abs(gamepad.axisLeftX), Math.abs(gamepad.axisLeftY)) >= 0.5;
        const x = left ? gamepad.axisLeftX : gamepad.axisRightX;
        const y = left ? gamepad.axisLeftY : gamepad.axisRightY;
        if (Math.max(Math.abs(x), Math.abs(y)) < 0.5)
            return 0;
        return Math.abs(y) >= Math.abs(x) ? (y < 0 ? upKey : downKey) : (x < 0 ? leftKey : rightKey);
    }
    Component.onCompleted: ready = true
    onStickDirectionChanged: {
        stickRepeat.stop();
        repeating = false;
        if (stickDirection && stickDirection !== Qt.Key_unknown) {
            ZxEmulator.sendNavigationKey(stickDirection);
            stickRepeat.start();
        }
    }
    property Timer stickRepeat: Timer {
        interval: navigation.repeating ? 90 : 400
        onTriggered: {
            if (!navigation.stickDirection)
                return;
            ZxEmulator.sendNavigationKey(navigation.stickDirection);
            navigation.repeating = true;
            restart();
        }
    }
    property Connections stickDevice: Connections {
        target: navigation.gamepad
        function onDeviceIdChanged() { navigation.stickRepeat.stop(); }
        function onConnectedChanged() {
            if (!navigation.gamepad.connected)
                navigation.stickRepeat.stop();
        }
    }

    // The text keyboard owns X while open; restore the page's shortcut on close.
    property Binding textInputShortcut: Binding {
        target: navigation
        property: "buttonXKey"
        value: Qt.Key_Backspace
        when: navigation.textInputActive
        restoreMode: Binding.RestoreBindingOrValue
    }
    property list<Binding> textInputModifiers: [
        Binding {
            target: navigation
            property: "buttonL1Key"
            value: Qt.Key_unknown
            when: navigation.textInputActive
            restoreMode: Binding.RestoreBindingOrValue
        },
        Binding {
            target: navigation
            property: "buttonR1Key"
            value: Qt.Key_unknown
            when: navigation.textInputActive
            restoreMode: Binding.RestoreBindingOrValue
        },
        Binding {
            target: navigation
            property: "buttonL2Key"
            value: Qt.Key_unknown
            when: navigation.textInputActive
            restoreMode: Binding.RestoreBindingOrValue
        },
        Binding {
            target: navigation
            property: "buttonR2Key"
            value: Qt.Key_unknown
            when: navigation.textInputActive
            restoreMode: Binding.RestoreBindingOrValue
        }
    ]
}
