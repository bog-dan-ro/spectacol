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

#include "controllerinput.h"

#include <algorithm>
#include <cmath>

bool ControllerInput::isSpectrumKey(int key)
{
    return (key >= Qt::Key_A && key <= Qt::Key_Z) || (key >= Qt::Key_0 && key <= Qt::Key_9)
            || key == Qt::Key_Shift || key == Qt::Key_Control || key == Qt::Key_Space
            || key == Qt::Key_Return || key == Qt::Key_Enter;
}

void ControllerInput::configure(int mode, const std::array<int, DirectionCount> &keys,
                                double deadZone)
{
    clear();
    m_mode = std::clamp(mode, int(Joystick), int(Custom));
    m_keys = keys;
    for (int i = 0; i < DirectionCount; ++i)
        if (!isSpectrumKey(m_keys[i]))
            m_keys[i] = qaopKeys[i];
    m_deadZone = std::isfinite(deadZone) ? std::clamp(deadZone, 0.0, 1.0) : 0.4;
}

void ControllerInput::axis(Axis axis, double value)
{
    if (axis >= LeftX && axis < AxisCount)
        m_axes[axis] = std::isfinite(value) ? std::clamp(value, -1.0, 1.0) : 0.0;
}

void ControllerInput::press(int id, int action, quint8 direction)
{
    if (!m_buttons.contains(id))
        m_buttons.insert(id, { action, direction });
}

std::optional<int> ControllerInput::release(int id)
{
    const auto it = m_buttons.find(id);
    if (it == m_buttons.end())
        return {};
    const int action = it->action;
    m_buttons.erase(it);
    return action;
}

void ControllerInput::clear()
{
    m_axes.fill(0);
    m_buttons.clear();
}

void ControllerInput::clearPhysical()
{
    std::fill(m_axes.begin(), m_axes.begin() + TouchX, 0.0);
    m_buttons.removeIf([](auto it) { return it.key() < TouchButtonOffset; });
}

void ControllerInput::clearTouch()
{
    m_axes[TouchX] = m_axes[TouchY] = 0;
    m_buttons.removeIf([](auto it) { return it.key() >= TouchButtonOffset; });
}

ControllerInput::State ControllerInput::state() const
{
    quint8 buttons = 0;
    for (int i = 0; i < AxisCount; ++i) {
        const bool horizontal = i % 2 == 0;
        if (m_axes[i] < 0 && m_axes[i] <= -m_deadZone)
            buttons |= horizontal ? LeftBit : UpBit;
        else if (m_axes[i] > 0 && m_axes[i] >= m_deadZone)
            buttons |= horizontal ? RightBit : DownBit;
    }
    State state;
    for (const auto &button : m_buttons) {
        state.actions.insert(button.action);
        if (button.direction)
            buttons |= button.direction;
        else if (!button.action) // JoystickFire is the default button action.
            buttons |= FireBit;
        else if (isSpectrumKey(button.action))
            state.keys.insert(button.action);
    }
    if (m_mode == Joystick) {
        state.joystick = buttons;
    } else {
        constexpr std::array<quint8, DirectionCount> bits{ UpBit, DownBit, LeftBit, RightBit,
                                                           FireBit };
        const auto &keys = m_mode == Qaop ? qaopKeys : m_keys;
        for (int i = 0; i < DirectionCount; ++i)
            if (buttons & bits[i])
                state.keys.insert(keys[i]);
    }
    return state;
}

HeldKeyState::Changes HeldKeyState::setSource(quint64 source, const QSet<int> &keys)
{
    if (keys.isEmpty())
        m_sources.remove(source);
    else
        m_sources.insert(source, keys);
    return update();
}

HeldKeyState::Changes HeldKeyState::clear()
{
    m_sources.clear();
    return update();
}

HeldKeyState::Changes HeldKeyState::update()
{
    QSet<int> keys;
    for (const auto &source : m_sources)
        keys.unite(source);
    Changes changes;
    for (const int key : m_keys - keys)
        changes.append({ key, false });
    for (const int key : keys - m_keys)
        changes.append({ key, true });
    m_keys = std::move(keys);
    return changes;
}
