#include "keyboard.h"
#include "ui/ui.h"
#include "ui/uidisplay.h"
#include <libspectrum.h>

keysyms_map_t keysyms_map[] = {{ INPUT_KEY_NONE, INPUT_KEY_NONE }};

int
ui_init( int *argc, char ***argv )
{
    return 1;
}

int
ui_event( void )
{
    return 1;
}

int
ui_mouse_grab( int startup )
{
    return 1;
}

int
ui_mouse_release( int suspend )
{
    return 1;
}

int
ui_joystick_init( void )
{
    return 1;
}

void
ui_joystick_poll( void )
{
}

void
ui_joystick_end( void )
{
}

int
ui_end( void )
{
    return 1;
}

int
uidisplay_init( int width, int height )
{
    return 1;
}

int
uidisplay_hotswap_gfx_mode( void )
{
    return 1;
}

void
uidisplay_frame_save( void )
{
}

void
uidisplay_frame_restore( void )
{
}

void
uidisplay_putpixel( int x, int y, int colour )
{
}

void
uidisplay_plot8( int x, int y, libspectrum_byte data,
             libspectrum_byte ink, libspectrum_byte paper )
{
}

void
uidisplay_plot16( int x, int y, libspectrum_word data,
          libspectrum_byte ink, libspectrum_byte paper )
{
}

void
uidisplay_frame_end( void )
{
}

void
uidisplay_area( int x, int y, int width, int height )
{
}

int
uidisplay_end( void )
{
    return 1;
}

int
ui_widgets_reset( void )
{
    return 1;
}

void
ui_breakpoints_updated( void )
{
}

int
ui_debugger_update( void )
{
    return 1;
}

int
ui_debugger_disassemble( libspectrum_word address )
{
    return 1;
}

int
ui_debugger_activate( void )
{
    return 1;
}

int
ui_debugger_deactivate( int interruptable )
{
    return 1;
}

int
ui_error_specific( ui_error_level severity, const char *message )
{
    return 1;
}

ui_confirm_joystick_t
ui_confirm_joystick( libspectrum_joystick libspectrum_type, int inputs )
{
    return UI_CONFIRM_JOYSTICK_NONE;
}

int
ui_statusbar_update( ui_statusbar_item item, ui_statusbar_state state )
{
  return 0;
}

char *ui_get_open_filename( const char *title )
{
    return 0;
}

char *ui_get_save_filename( const char *title )
{
    return 0;
}

int ui_query( const char *message )
{
    return 0;
}

void ui_pokemem_selector( const char *filename )
{

}

int ui_menu_activate( ui_menu_item item, int active )
{
    return 0;
}
