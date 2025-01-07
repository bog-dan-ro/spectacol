/* qmlui.cpp: convenient functions to post runnables on fuse thread.

    Copyright (c) 2015-2025, BogDan Vatra <bogdan@kde.org>

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
#include "fuserecording.h"
#include "fusesettings.h"
#include "breakpointsmodel.h"
#include "disassamblemodel.h"

#include <QAudioDevice>
#include <QMediaDevices>

#include <mutex>
#include <deque>

extern "C"  {
# include <fuse.h>
# include <fusetape.h>
# include <settings.h>
# include <pokefinder/pokefinder.h>
# include <ui/uidisplay.h>
# include <ui/widget/widget.h>
}

#ifdef Q_OS_ANDROID
# include <QCoreApplication>
#endif

namespace {
    std::mutex s_eventsMutex;
    std::deque<SpectrumEventFunction> s_events;
}

extern "C" int ui_init( int *, char ***)
{
    ui_mouse_present = 1;
    auto audio = QMediaDevices::defaultAudioOutput();
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

static std::atomic_uint_fast32_t blockErrors;

void showErrors()
{
    --blockErrors;
}

void hideErrors()
{
    ++blockErrors;
}

extern "C" int ui_error_specific( ui_error_level severity, const char *message )
{
    if (!blockErrors.load())
        FuseEmulator::instance().showMessage(QString::fromUtf8(message), FuseEmulator::ErrorLevel(severity));
    return 0;
}

extern "C" void ui_breakpoints_updated( void )
{
    FuseEmulator::instance().breakpointsModel()->breakpointsUpdated();
}

extern "C" int ui_debugger_activate( void )
{
    FuseEmulator::instance().activateDebugger();
    return 1;
}

extern "C" int ui_debugger_deactivate( int interruptable )
{
    FuseEmulator::instance().deactivateDebugger(interruptable);
    return 0;
}

extern "C" int ui_debugger_update( void )
{
    FuseEmulator::instance().updateDebugger();
    return 0;
}

extern "C" int ui_debugger_disassemble( libspectrum_word address )
{
    FuseEmulator::instance().disassamble(address, 0 , 0x150);
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
        FuseEmulator::instance().callFunctionDelayed(500, []{emit FuseEmulator::instance().configureJoystick();});

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
    return event;
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
    FuseEmulator::instance().settings()->callFunction([]{
        emit FuseEmulator::instance().settings()->currentMachineChanged();
    });
    return 0;
}

extern "C" int ui_statusbar_update(ui_statusbar_item item, ui_statusbar_state state)
{
    FuseEmulator::instance().uiStatusbarUpdate(item, state);
    return 0;
}

extern "C" int ui_statusbar_update_speed( float speed )
{
    Q_UNUSED(speed)
    return 0;
}

extern "C" char *ui_get_open_filename( const char *title )
{
    return FuseEmulator::instance().uiOpenFilename(title);
}

extern "C" char *ui_get_save_filename( const char *title )
{
    return FuseEmulator::instance().uiSaveFilename(title);
}

ui_confirm_save_t ui_confirm_save_specific( const char *message )
{
    return FuseEmulator::instance().uiConfirmSaveSpecific(message);
}

extern "C" int ui_query( const char *message )
{
    return FuseEmulator::instance().uiQuery(message);
}

extern "C" int ui_get_rollback_point(GSList *points)
{
    QStringList items;
    while (points) {
        items.push_front(QString::number(GPOINTER_TO_INT(points->data) / 50.0));
        points = points->next;
    }
    items.pop_front();
    FuseEmulator::instance().showMessage(QObject::tr("Rollback to second"));
    int res = FuseEmulator::instance().uiGetListIndex(items, QObject::tr("Rollback to second"));
    if (res != -1)
        return items.size() - 2 - res;
    return -1;
}

extern "C" void ui_pokemem_selector( const char *filename )
{
    FuseEmulator::instance().uiPokememSelector(filename);
}

extern "C" int ui_menu_activate(ui_menu_item item, int active)
{
    (void)active;
    switch (item) {
    case UI_MENU_ITEM_RECORDING:
        FuseEmulator::instance().m_recording->setStopVisible(active);
#ifdef Q_OS_ANDROID
        QNativeInterface::QAndroidApplication::runOnAndroidMainThread([active] {
            if (active)
                QJniObject{QNativeInterface::QAndroidApplication::context()}.callObjectMethod("getWindow", "()Landroid/view/Window;").callMethod<void>("addFlags", "(I)V",  128/*FLAG_KEEP_SCREEN_ON*/);
            else
                QJniObject{QNativeInterface::QAndroidApplication::context()}.callObjectMethod("getWindow", "()Landroid/view/Window;").callMethod<void>("clearFlags", "(I)V",  128/*FLAG_KEEP_SCREEN_ON*/);
        });
#endif
        break;

    default:
        break;
    }
    return 0;
}

extern "C" int ui_tape_browser_update(ui_tape_browser_update_type /*change*/, libspectrum_tape_block */*block*/)
{
    FuseEmulator::instance().m_tape->updateBrowseData();
    return 0;
}

void pokeEvent(const SpectrumEventFunction &event)
{
    s_eventsMutex.lock();
    s_events.push_back(event);
    s_eventsMutex.unlock();
}
