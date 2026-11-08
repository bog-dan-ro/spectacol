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

#pragma once

#include <QHash>
#include <QList>
#include <QSet>
#include <Qt>

#include <array>
#include <optional>
#include <utility>

// Host-side input ownership. No emulation state or device handles live here.
class ControllerInput
{
public:
    enum Mode { Joystick, Qaop, Custom };
    enum Axis { LeftX, LeftY, RightX, RightY, TouchX, TouchY, AxisCount };
    enum Direction { Up, Down, Left, Right, Fire, DirectionCount };
    // The standard Kempston bit layout, also used by ZxKit's joystick interfaces.
    static constexpr quint8 RightBit = 0x01, LeftBit = 0x02, DownBit = 0x04, UpBit = 0x08,
                            FireBit = 0x10;
    static constexpr int TouchButtonOffset = 0x10000;
    static constexpr std::array<int, DirectionCount> qaopKeys{ Qt::Key_Q, Qt::Key_A, Qt::Key_O,
                                                               Qt::Key_P, Qt::Key_Space };
    struct State
    {
        quint8 joystick = 0;
        QSet<int> keys;
        QSet<int> actions;
    };

    static bool isSpectrumKey(int key);
    void configure(int mode, const std::array<int, DirectionCount> &keys, double deadZone);
    void axis(Axis axis, double value);
    void press(int id, int action, quint8 direction = 0);
    std::optional<int> release(int id);
    void clear();
    void clearPhysical();
    void clearTouch();
    State state() const;

private:
    struct Button
    {
        int action;
        quint8 direction;
    };
    std::array<double, AxisCount> m_axes{};
    QHash<int, Button> m_buttons;
    std::array<int, DirectionCount> m_keys = qaopKeys;
    int m_mode = Joystick;
    double m_deadZone = 0.4;
};

// Shared action values keep saved controller mappings compatible across releases.
enum class ControllerAction : int {
    Fire = 0,
    Keyboard = Qt::Key_unknown + 1,
    Keypad,
    Pause,
    QuickSave,
    QuickLoad,
    FastForward
};

// Release-to-toggle matches the other controller shortcuts. Cancellation never
// toggles; source ownership keeps simultaneous buttons/touch independent.
class FastForwardControl
{
public:
    void setToggle(bool toggle)
    {
        if (m_toggle != toggle) {
            clear();
            m_toggle = toggle;
        }
    }
    void press(int source)
    {
        m_held.insert(source);
    }
    void release(int source, bool cancelled = false)
    {
        if (m_held.remove(source) && m_toggle && !cancelled)
            m_latchedSource = m_latchedSource < 0 ? source : -1;
    }
    void clear()
    {
        m_held.clear();
        m_latchedSource = -1;
    }
    void clearPhysical()
    {
        clearSources(false);
    }
    void clearTouch()
    {
        clearSources(true);
    }
    bool active() const
    {
        return m_latchedSource >= 0 || (!m_toggle && !m_held.isEmpty());
    }

private:
    void clearSources(bool touch)
    {
        m_held.removeIf(
                [touch](int id) { return (id >= ControllerInput::TouchButtonOffset) == touch; });
        if (m_latchedSource >= 0
            && (m_latchedSource >= ControllerInput::TouchButtonOffset) == touch)
            m_latchedSource = -1;
    }
    QSet<int> m_held;
    int m_latchedSource = -1;
    bool m_toggle = false;
};

// A key stays down until every input source that owns it has released it.
// Keys are opaque integers here; the emulator facade supplies ZxKey values.
class HeldKeyState
{
public:
    using Changes = QList<std::pair<int, bool>>;
    Changes setSource(quint64 source, const QSet<int> &keys);
    Changes clear();
    const QSet<int> &keys() const
    {
        return m_keys;
    }

private:
    Changes update();
    QHash<quint64, QSet<int>> m_sources;
    QSet<int> m_keys;
};
