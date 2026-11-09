/* Copyright (c) 2015-2026, BogDan Vatra. SPDX-License-Identifier: GPL-3.0-or-later */

import QtQuick
import QtGamepadLegacy as Legacy

Legacy.GamepadKeyNavigation {
    id: navigation

    property bool textInputActive: DesktopInput.visible

    // The text keyboard owns X while open; restore the page's shortcut on close.
    property Binding textInputShortcut: Binding {
        target: navigation
        property: "buttonXKey"
        value: Qt.Key_Backspace
        when: navigation.textInputActive
        restoreMode: Binding.RestoreBindingOrValue
    }
}
