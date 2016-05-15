/* qmlui.cpp: convenient functions to post runnables on fuse thread.

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

#include "qmlui.h"
#include "fuseemulator.h"
#include "breakpointsmodel.h"
#include "disassamblemodel.h"

#include <fuse.h>
#include <settings.h>
#include <pokefinder/pokefinder.h>
#include <ui/uidisplay.h>
#include <ui/widget/widget.h>

#include <QAudioDeviceInfo>

#include <mutex>
#include <deque>

static std::mutex s_eventsMutex;
static std::deque<SpectrumEventFunction> s_events;

extern "C" int ui_init( int *, char ***)
{
    ui_mouse_present = 1;
    auto audio = QAudioDeviceInfo::defaultOutputDevice();
    if (!audio.isNull())
        settings_current.sound_freq = audio.preferredFormat().sampleRate();
    return ui_widget_init();
}

extern "C" int ui_end(void)
{
    return ui_widget_end();
}

extern "C" int ui_mouse_grab( int /*startup*/ )
{
    return 0;
}

extern "C" int ui_mouse_release( int /*suspend*/ )
{
    return 0;
}

extern "C" int ui_error_specific( ui_error_level severity, const char *message )
{
    QString msg = QLatin1String(message);
    g_fuseEmulator->callFunction([severity, msg]{
        emit g_fuseEmulator->error(FuseEmulator::ErrorLevel(severity), msg);
    });
    return 0;
}

extern "C" void ui_breakpoints_updated( void )
{
    g_fuseEmulator->breakpointsModel()->breakpointsUpdated();
}

extern "C" int ui_debugger_activate( void )
{
    g_fuseEmulator->activateDebugger();
    return 0;
}

extern "C" int ui_debugger_deactivate( int interruptable )
{
    g_fuseEmulator->deactivateDebugger(interruptable);
    return 0;
}

extern "C" int ui_debugger_update( void )
{
    g_fuseEmulator->updateDebugger();
    return 0;
}

extern "C" int ui_debugger_disassemble( libspectrum_word address )
{
    g_fuseEmulator->disassamble(address, 0 , 0x150);
    return 0;
}

extern "C" int ui_joystick_init( void )
{
  return 1;
}

extern "C" void ui_joystick_end( void )
{
}

extern "C" void ui_joystick_poll( void )
{
}

extern "C" ui_confirm_joystick_t ui_confirm_joystick( libspectrum_joystick /*libspectrum_type*/, int /*inputs*/ )
{
    if (settings_current.joy_prompt)
        g_fuseEmulator->callFunction([]{emit g_fuseEmulator->configureJoystick();});

    return UI_CONFIRM_JOYSTICK_JOYSTICK_1;
}

inline SpectrumEventFunction peekEvent()
{
    SpectrumEventFunction event;
    s_eventsMutex.lock();
    if (!s_events.empty()) {
        event = std::move(s_events.front());
        s_events.pop_front();
    }
    s_eventsMutex.unlock();
    return std::move(event);
}

extern "C" int ui_event( void )
{
    for(;;) {
        SpectrumEventFunction event(peekEvent());
        if (event)
            event();
        else
            break;
    }
    return 0;
}

extern "C" int ui_widgets_reset( void )
{
    pokefinder_clear();
    return 0;
}

extern "C" int ui_statusbar_update(ui_statusbar_item item, ui_statusbar_state state)
{
    g_fuseEmulator->uiStatusbarUpdate(item, state);
    return 0;
}

void pokeEvent(const SpectrumEventFunction &event)
{
    s_eventsMutex.lock();
    s_events.push_back(event);
    s_eventsMutex.unlock();
}
