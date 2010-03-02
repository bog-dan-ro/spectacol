/* gtkui.c: GTK+ routines for dealing with the user interface
   Copyright (c) 2000-2005 Philip Kendall, Russell Marks

   $Id$

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

   Author contact information:

   E-mail: philip-fuse@shadowmagic.org.uk

*/

#include <config.h>

#include <stdio.h>

#include <gdk/gdkkeysyms.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>

#include <glib.h>

#include <libspectrum.h>

#include "debugger/debugger.h"
#include "fuse.h"
#include "gtkinternals.h"
#include "ide/simpleide.h"
#include "ide/zxatasp.h"
#include "ide/zxcf.h"
#include "joystick.h"
#include "keyboard.h"
#include "machine.h"
#include "machines/specplus3.h"
#include "menu.h"
#include "psg.h"
#include "rzx.h"
#include "screenshot.h"
#include "settings.h"
#include "snapshot.h"
#include "timer/timer.h"
#include "ui/ui.h"
#include "utils.h"

/* The main Fuse window */
GtkWidget *gtkui_window;

/* The area into which the screen will be drawn */
GtkWidget *gtkui_drawing_area;

/* Popup menu widget(s), as invoked by F1 */
GtkWidget *gtkui_menu_popup;

/* The item factory used to create the menu bar */
GtkItemFactory *menu_factory;

/* And that used to create the popup menus */
GtkItemFactory *popup_factory;

/* True if we were paused via the Machine/Pause menu item */
static int paused = 0;

/* Structure used by the radio button selection widgets (eg the
   graphics filter selectors and Machine/Select) */
typedef struct gtkui_select_info {

  GtkWidget *dialog;
  GtkWidget **buttons;

  /* Used by the graphics filter selectors */
  scaler_available_fn selector;
  scaler_type selected;

  /* Used by the joystick confirmation */
  ui_confirm_joystick_t joystick;

} gtkui_select_info;

static gboolean gtkui_make_menu(GtkAccelGroup **accel_group,
				GtkWidget **menu_bar,
				GtkItemFactoryEntry *menu_data,
				guint menu_data_size);

static gboolean gtkui_lose_focus( GtkWidget*, GdkEvent*, gpointer );
static gboolean gtkui_gain_focus( GtkWidget*, GdkEvent*, gpointer );

static gboolean gtkui_delete( GtkWidget *widget, GdkEvent *event,
			      gpointer data );

static void menu_options_filter_done( GtkWidget *widget, gpointer user_data );
static void menu_machine_select_done( GtkWidget *widget, gpointer user_data );

static const GtkTargetEntry drag_types[] =
{
    { "text/uri-list", GTK_TARGET_OTHER_APP, 0 }
};

static void gtkui_drag_data_received( GtkWidget *widget,
                                      GdkDragContext *drag_context,
                                      gint x, gint y,
                                      GtkSelectionData *data,
                                      guint info, guint time )
{
  static char uri_prefix[] = "file://";
  char *filename, *p, *data_end;

  if ( data && data->length > sizeof( uri_prefix ) ) {
    filename = ( char * )( data->data + sizeof( uri_prefix ) - 1 );
    data_end = ( char * )( data->data + data->length );
    p = filename; 
    do {
      if ( *p == '\r' || *p == '\n' ) {
        *p = '\0';
	break;
      }
    } while ( p++ != data_end );

    if ( ( filename = g_uri_unescape_string( filename, NULL ) ) != NULL ) {
      fuse_emulation_pause();
      utils_open_file( filename, settings_current.auto_load, NULL );
      free( filename );
      display_refresh_all();
      fuse_emulation_unpause();
    }
  }
  gtk_drag_finish(drag_context, FALSE, FALSE, time);
}

int
ui_init( int *argc, char ***argv )
{
  GtkWidget *box, *menu_bar;
  GtkAccelGroup *accel_group;

  gtk_init(argc,argv);

  gdk_rgb_init();
  gdk_rgb_set_install( TRUE );
  gtk_widget_set_default_colormap( gdk_rgb_get_cmap() );
  gtk_widget_set_default_visual( gdk_rgb_get_visual() );

  gtkui_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  gtk_window_set_title( GTK_WINDOW(gtkui_window), "Fuse" );
  gtk_window_set_wmclass( GTK_WINDOW(gtkui_window), fuse_progname, "Fuse" );

  gtk_signal_connect(GTK_OBJECT(gtkui_window), "delete-event",
		     GTK_SIGNAL_FUNC(gtkui_delete), NULL);
  gtk_signal_connect(GTK_OBJECT(gtkui_window), "key-press-event",
		     GTK_SIGNAL_FUNC(gtkkeyboard_keypress), NULL);
  gtk_widget_add_events( gtkui_window, GDK_KEY_RELEASE_MASK );
  gtk_signal_connect(GTK_OBJECT(gtkui_window), "key-release-event",
		     GTK_SIGNAL_FUNC(gtkkeyboard_keyrelease), NULL);

  /* If we lose the focus, disable all keys */
  gtk_signal_connect( GTK_OBJECT( gtkui_window ), "focus-out-event",
		      GTK_SIGNAL_FUNC( gtkui_lose_focus ), NULL );
  gtk_signal_connect( GTK_OBJECT( gtkui_window ), "focus-in-event",
		      GTK_SIGNAL_FUNC( gtkui_gain_focus ), NULL );

  gtk_drag_dest_set( GTK_WIDGET( gtkui_window ),
                     GTK_DEST_DEFAULT_ALL,
                     drag_types,
                     G_N_ELEMENTS( drag_types ),
                     GDK_ACTION_COPY | GDK_ACTION_PRIVATE );
                     /* GDK_ACTION_PRIVATE alone DNW with ROX-Filer */

  gtk_signal_connect( GTK_OBJECT( gtkui_window ), "drag-data-received",
		      GTK_SIGNAL_FUNC( gtkui_drag_data_received ), NULL );

  box = gtk_vbox_new( FALSE, 0 );
  gtk_container_add(GTK_CONTAINER(gtkui_window), box);

  gtkui_make_menu( &accel_group, &menu_bar, gtkui_menu_data,
		   gtkui_menu_data_size );

  gtk_window_add_accel_group( GTK_WINDOW(gtkui_window), accel_group );
  gtk_box_pack_start( GTK_BOX(box), menu_bar, FALSE, FALSE, 0 );

  gtkui_drawing_area = gtk_drawing_area_new();
  if(!gtkui_drawing_area) {
    fprintf(stderr,"%s: couldn't create drawing area at %s:%d\n",
	    fuse_progname,__FILE__,__LINE__);
    return 1;
  }

  gtk_widget_add_events( GTK_WIDGET( gtkui_drawing_area ),
    GDK_POINTER_MOTION_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK );
  gtk_signal_connect( GTK_OBJECT( gtkui_drawing_area ), "motion-notify-event",
		      GTK_SIGNAL_FUNC( gtkmouse_position ), NULL );
  gtk_signal_connect( GTK_OBJECT( gtkui_drawing_area ), "button-press-event",
		      GTK_SIGNAL_FUNC( gtkmouse_button ), NULL );
  gtk_signal_connect( GTK_OBJECT( gtkui_drawing_area ), "button-release-event",
		      GTK_SIGNAL_FUNC( gtkmouse_button ), NULL );

  gtk_box_pack_start( GTK_BOX(box), gtkui_drawing_area, TRUE, TRUE, 0 );

  /* Create the statusbar */
  gtkstatusbar_create( GTK_BOX( box ) );

  gtk_widget_show_all( gtkui_window );
  gtkstatusbar_set_visibility( settings_current.statusbar );

  ui_mouse_present = 1;

  return 0;
}

static void
gtkui_menu_deactivate( GtkMenuShell *menu GCC_UNUSED,
		       gpointer data GCC_UNUSED )
{
  ui_mouse_resume();
}

static gboolean
gtkui_make_menu(GtkAccelGroup **accel_group,
                GtkWidget **menu_bar,
                GtkItemFactoryEntry *menu_data,
                guint menu_data_size)
{
  *accel_group = gtk_accel_group_new();
  menu_factory = gtk_item_factory_new( GTK_TYPE_MENU_BAR, "<main>",
				       *accel_group );
  gtk_item_factory_create_items( menu_factory, menu_data_size, menu_data,
				 NULL);
  *menu_bar = gtk_item_factory_get_widget( menu_factory, "<main>" );
  gtk_signal_connect( GTK_OBJECT( *menu_bar ), "deactivate",
		      GTK_SIGNAL_FUNC( gtkui_menu_deactivate ), NULL );

  /* We have to recreate the menus for the popup, unfortunately... */
  popup_factory = gtk_item_factory_new( GTK_TYPE_MENU, "<main>", NULL );
  gtk_item_factory_create_items( popup_factory, menu_data_size, menu_data,
				 NULL);
  gtkui_menu_popup = gtk_item_factory_get_widget( popup_factory, "<main>" );

  /* Start various menus in the 'off' state */
  ui_menu_activate( UI_MENU_ITEM_AY_LOGGING, 0 );
  ui_menu_activate( UI_MENU_ITEM_FILE_MOVIES_RECORDING, 0 );
  ui_menu_activate( UI_MENU_ITEM_MACHINE_PROFILER, 0 );
  ui_menu_activate( UI_MENU_ITEM_RECORDING, 0 );
  ui_menu_activate( UI_MENU_ITEM_RECORDING_ROLLBACK, 0 );
  ui_menu_activate( UI_MENU_ITEM_TAPE_RECORDING, 0 );

  return FALSE;
}

static void
gtkui_popup_menu_pos( GtkMenu *menu GCC_UNUSED, gint *xp, gint *yp,
		      GtkWidget *data GCC_UNUSED )
{
  gdk_window_get_position( gtkui_window->window, xp, yp );
}

/* Popup main menu, as invoked by F1. */
void
gtkui_popup_menu(void)
{
  gtk_menu_popup( GTK_MENU(gtkui_menu_popup), NULL, NULL,
		  (GtkMenuPositionFunc)gtkui_popup_menu_pos, NULL,
		  0, 0 );
}

int
ui_event(void)
{
  while(gtk_events_pending())
    gtk_main_iteration();
  return 0;
}

int
ui_end(void)
{
  /* Don't display the window whilst doing all this! */
  gtk_widget_hide( gtkui_window );

  return 0;
}

/* Create a dialog box with the given error message */
int
ui_error_specific( ui_error_level severity, const char *message )
{
  GtkWidget *dialog, *label, *vbox;
  const gchar *title;

  /* If we don't have a UI yet, we can't output widgets */
  if( !display_ui_initialised ) return 0;

  /* Set the appropriate title */
  switch( severity ) {
  case UI_ERROR_INFO:	 title = "Fuse - Info"; break;
  case UI_ERROR_WARNING: title = "Fuse - Warning"; break;
  case UI_ERROR_ERROR:	 title = "Fuse - Error"; break;
  default:		 title = "Fuse - (Unknown Error Level)"; break;
  }

  /* Create the dialog box */
  dialog = gtkstock_dialog_new( title, GTK_SIGNAL_FUNC( gtk_widget_destroy ) );

  /* Add the OK button into the lower half */
  gtkstock_create_close( dialog, NULL, GTK_SIGNAL_FUNC (gtk_widget_destroy),
			 FALSE );

  /* Create a label with that message */
  label = gtk_label_new( message );

  /* Make a new vbox for the top part for saner spacing */
  vbox=gtk_vbox_new( FALSE, 0 );
  gtk_box_pack_start( GTK_BOX( GTK_DIALOG( dialog )->vbox ),
                      vbox, TRUE, TRUE, 0 );
  gtk_container_set_border_width( GTK_CONTAINER( vbox ), 5 );
  gtk_container_set_border_width( GTK_CONTAINER( GTK_DIALOG( dialog )->action_area ),
                                  5 );

  /* Put the label in it */
  gtk_container_add( GTK_CONTAINER( vbox ), label );

  gtk_widget_show_all( dialog );

  return 0;
}

/* The callbacks used by various routines */

static gboolean
gtkui_lose_focus( GtkWidget *widget GCC_UNUSED,
		  GdkEvent *event GCC_UNUSED, gpointer data GCC_UNUSED )
{
  keyboard_release_all();
  ui_mouse_suspend();
  return TRUE;
}

static gboolean
gtkui_gain_focus( GtkWidget *widget GCC_UNUSED,
		  GdkEvent *event GCC_UNUSED, gpointer data GCC_UNUSED )
{
  ui_mouse_resume();
  return TRUE;
}

/* Called by the main window on a "delete-event" */
static gboolean
gtkui_delete( GtkWidget *widget, GdkEvent *event GCC_UNUSED, gpointer data )
{
  menu_file_exit( widget, data );
  return TRUE;
}

/* Called by the menu when File/Exit selected */
void
menu_file_exit( GtkWidget *widget GCC_UNUSED, gpointer data GCC_UNUSED )
{
  if( gtkui_confirm( "Exit Fuse?" ) ) {

    if( menu_check_media_changed() ) return;

    fuse_exiting = 1;

    /* Stop the paused state to allow us to exit (occurs from main
       emulation loop) */
    if( paused ) menu_machine_pause( NULL, NULL );
  }
}

/* Select a graphics filter from those for which `available' returns
   true */
scaler_type
menu_get_scaler( scaler_available_fn selector )
{
  gtkui_select_info dialog;
  GSList *button_group = NULL;

  int count;
  scaler_type scaler;

  /* Store the function which tells us which scalers are currently
     available */
  dialog.selector = selector;

  /* No scaler currently selected */
  dialog.selected = SCALER_NUM;

  /* Some space to store the radio buttons in */
  dialog.buttons = malloc( SCALER_NUM * sizeof(GtkWidget* ) );
  if( dialog.buttons == NULL ) {
    ui_error( UI_ERROR_ERROR, "out of memory at %s:%d", __FILE__, __LINE__ );
    return SCALER_NUM;
  }

  count = 0;

  /* Create the necessary widgets */
  dialog.dialog = gtkstock_dialog_new( "Fuse - Select Scaler", NULL );

  for( scaler = 0; scaler < SCALER_NUM; scaler++ ) {

    if( !selector( scaler ) ) continue;

    dialog.buttons[ count ] =
      gtk_radio_button_new_with_label( button_group, scaler_name( scaler ) );
    button_group =
      gtk_radio_button_group( GTK_RADIO_BUTTON( dialog.buttons[ count ] ) );

    gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( dialog.buttons[ count ] ),
				  current_scaler == scaler );

    gtk_container_add( GTK_CONTAINER( GTK_DIALOG( dialog.dialog )->vbox ),
		       dialog.buttons[ count ] );

    count++;
  }

  /* Create and add the actions buttons to the dialog box */
  gtkstock_create_ok_cancel( dialog.dialog, NULL,
			     GTK_SIGNAL_FUNC( menu_options_filter_done ),
			     (gpointer) &dialog, NULL );

  gtk_widget_show_all( dialog.dialog );

  /* Process events until the window is done with */
  gtk_main();

  return dialog.selected;
}

/* Callback used by the filter selection dialog */
static void
menu_options_filter_done( GtkWidget *widget GCC_UNUSED, gpointer user_data )
{
  int i, count;
  gtkui_select_info *ptr = (gtkui_select_info*)user_data;

  count = 0;

  for( i = 0; i < SCALER_NUM; i++ ) {

    if( !ptr->selector( i ) ) continue;

    if( gtk_toggle_button_get_active(
	  GTK_TOGGLE_BUTTON( ptr->buttons[ count ] )
	)
      ) {
      ptr->selected = i;
    }

    count++;
  }

  gtk_widget_destroy( ptr->dialog );
  gtk_main_quit();
}

/* Machine/Pause */
void
menu_machine_pause( GtkWidget *widget GCC_UNUSED, gpointer data GCC_UNUSED )
{
  int error;

  if( paused ) {
    paused = 0;
    ui_statusbar_update( UI_STATUSBAR_ITEM_PAUSED,
			 UI_STATUSBAR_STATE_INACTIVE );
    timer_estimate_reset();
    gtk_main_quit();
  } else {

    /* Stop recording any competition mode RZX file */
    if( rzx_recording && rzx_competition_mode ) {
      ui_error( UI_ERROR_INFO, "Stopping competition mode RZX recording" );
      error = rzx_stop_recording(); if( error ) return;
    }

    paused = 1;
    ui_statusbar_update( UI_STATUSBAR_ITEM_PAUSED, UI_STATUSBAR_STATE_ACTIVE );
    gtk_main();
  }

}

/* Called by the menu when Machine/Reset selected */
void
menu_machine_reset( GtkWidget *widget GCC_UNUSED, gpointer data )
{
  int hard_reset = GPOINTER_TO_INT( data );

  if( gtkui_confirm( "Reset?" ) && machine_reset( hard_reset ) ) {
    ui_error( UI_ERROR_ERROR, "couldn't reset machine: giving up!" );

    /* FIXME: abort() seems a bit extreme here, but it'll do for now */
    fuse_abort();
  }
}

/* Called by the menu when Machine/Select selected */
void
menu_machine_select( GtkWidget *widget GCC_UNUSED, gpointer data GCC_UNUSED )
{
  gtkui_select_info dialog;

  int i;

  /* Some space to store the radio buttons in */
  dialog.buttons = malloc( machine_count * sizeof(GtkWidget* ) );
  if( dialog.buttons == NULL ) {
    ui_error( UI_ERROR_ERROR, "out of memory at %s:%d", __FILE__, __LINE__ );
    return;
  }

  /* Stop emulation */
  fuse_emulation_pause();

  /* Create the necessary widgets */
  dialog.dialog = gtkstock_dialog_new( "Fuse - Select Machine", NULL );

  dialog.buttons[0] =
    gtk_radio_button_new_with_label(
      NULL, libspectrum_machine_name( machine_types[0]->machine )
    );

  for( i=1; i<machine_count; i++ ) {
    dialog.buttons[i] =
      gtk_radio_button_new_with_label(
        gtk_radio_button_group (GTK_RADIO_BUTTON (dialog.buttons[i-1] ) ),
	libspectrum_machine_name( machine_types[i]->machine )
      );
  }

  for( i=0; i<machine_count; i++ ) {
    gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( dialog.buttons[i] ),
				  machine_current == machine_types[i] );
    gtk_container_add( GTK_CONTAINER( GTK_DIALOG( dialog.dialog )->vbox ),
		       dialog.buttons[i] );
  }

  /* Create and add the actions buttons to the dialog box */
  gtkstock_create_ok_cancel( dialog.dialog, NULL,
			     GTK_SIGNAL_FUNC( menu_machine_select_done ),
			     (gpointer) &dialog, NULL );

  gtk_widget_show_all( dialog.dialog );

  /* Process events until the window is done with */
  gtk_main();

  /* And then carry on with emulation again */
  fuse_emulation_unpause();
}

/* Callback used by the machine selection dialog */
static void
menu_machine_select_done( GtkWidget *widget GCC_UNUSED, gpointer user_data )
{
  int i;
  gtkui_select_info *ptr = user_data;

  for( i=0; i<machine_count; i++ ) {
    if( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(ptr->buttons[i]) ) &&
        machine_current != machine_types[i]
      )
    {
      machine_select( machine_types[i]->machine );
    }
  }

  gtk_widget_destroy( ptr->dialog );
  gtk_main_quit();
}

void
menu_machine_debugger( GtkWidget *widget GCC_UNUSED, gpointer data GCC_UNUSED )
{
  debugger_mode = DEBUGGER_MODE_HALTED;
  if( paused ) ui_debugger_activate();
}

/* Called on machine selection */
int
ui_widgets_reset( void )
{
  gtkui_pokefinder_clear();
  return 0;
}

void
menu_help_keyboard( GtkWidget *widget GCC_UNUSED, gpointer data GCC_UNUSED )
{
  gtkui_picture( "keyboard.scr", 0 );
}

void
menu_help_about( GtkWidget *widget GCC_UNUSED, gpointer data GCC_UNUSED )
{
  gtk_show_about_dialog( GTK_WINDOW( gtkui_window ),
                         "name", "Fuse",
                         "comments", "The Free Unix Spectrum Emulator",
                         "copyright", "(c) 1999-2008 Philip Kendall and others.",
                         "version", VERSION,
                         "website", "http://fuse-emulator.sourceforge.net/",
                         NULL );
}

/* Generic `tidy-up' callback */
void
gtkui_destroy_widget_and_quit( GtkWidget *widget, gpointer data GCC_UNUSED )
{
  gtk_widget_destroy( widget );
  gtk_main_quit();
}

/* Functions to activate and deactivate certain menu items */

int
ui_menu_item_set_active( const char *path, int active )
{
  GtkWidget *menu_item;

  menu_item = gtk_item_factory_get_widget( menu_factory, path );
  if( !menu_item ) {
    ui_error( UI_ERROR_ERROR, "couldn't get menu item '%s' from menu_factory",
	      path );
    return 1;
  }
  gtk_widget_set_sensitive( menu_item, active );

  menu_item = gtk_item_factory_get_widget( popup_factory, path );
  if( !menu_item ) {
    ui_error( UI_ERROR_ERROR, "couldn't get menu item '%s' from popup_factory",
	      path );
    return 1;
  }
  gtk_widget_set_sensitive( menu_item, active );

  return 0;
}

static void
confirm_joystick_done( GtkWidget *widget GCC_UNUSED, gpointer user_data )
{
  int i;
  gtkui_select_info *ptr = user_data;

  for( i = 0; i < JOYSTICK_CONN_COUNT; i++ ) {

    GtkToggleButton *button = GTK_TOGGLE_BUTTON( ptr->buttons[ i ] );

    if( gtk_toggle_button_get_active( button ) ) {
      ptr->joystick = i;
      break;
    }
  }

  gtk_widget_destroy( ptr->dialog );
  gtk_main_quit();
}

ui_confirm_joystick_t
ui_confirm_joystick( libspectrum_joystick libspectrum_type,
		     int inputs GCC_UNUSED )
{
  gtkui_select_info dialog;
  char title[ 80 ];
  int i;
  GSList *group = NULL;

  if( !settings_current.joy_prompt ) return UI_CONFIRM_JOYSTICK_NONE;

  /* Some space to store the radio buttons in */
  dialog.buttons =
    malloc( JOYSTICK_CONN_COUNT * sizeof( *dialog.buttons ) );
  if( !dialog.buttons ) {
    ui_error( UI_ERROR_ERROR, "out of memory at %s:%d", __FILE__, __LINE__ );
    return UI_CONFIRM_JOYSTICK_NONE;
  }

  /* Stop emulation */
  fuse_emulation_pause();

  /* Create the necessary widgets */
  snprintf( title, sizeof( title ), "Fuse - Configure %s Joystick",
	    libspectrum_joystick_name( libspectrum_type ) );
  dialog.dialog = gtkstock_dialog_new( title, NULL );

  for( i = 0; i < JOYSTICK_CONN_COUNT; i++ ) {

    GtkWidget **button = &( dialog.buttons[ i ] );

    *button =
      gtk_radio_button_new_with_label( group, joystick_connection[ i ] );
    group = gtk_radio_button_get_group( GTK_RADIO_BUTTON( *button ) );

    gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( *button ), i == 0 );
    gtk_container_add( GTK_CONTAINER( GTK_DIALOG( dialog.dialog )->vbox ),
		       *button );
  }

  /* Create and add the actions buttons to the dialog box */
  gtkstock_create_ok_cancel( dialog.dialog, NULL,
			     GTK_SIGNAL_FUNC( confirm_joystick_done ),
			     (gpointer) &dialog, NULL );

  gtk_widget_show_all( dialog.dialog );

  /* Process events until the window is done with */
  dialog.joystick = UI_CONFIRM_JOYSTICK_NONE;
  gtk_main();

  /* And then carry on with emulation again */
  fuse_emulation_unpause();

  return dialog.joystick;
}

/*
 * Font code
 */

int
gtkui_get_monospaced_font( gtkui_font *font )
{
  *font = pango_font_description_from_string( "Monospace 10" );
  if( !(*font) ) {
    ui_error( UI_ERROR_ERROR, "couldn't find a monospaced font" );
    return 1;
  }

  return 0;
}

void
gtkui_free_font( gtkui_font font )
{
  pango_font_description_free( font );
}

void
gtkui_set_font( GtkWidget *widget, gtkui_font font )
{
  gtk_widget_modify_font( widget, font );
}

static void
key_scroll_event( GtkCList *clist GCC_UNUSED, GtkScrollType scroll,
		  gfloat position, gpointer user_data )
{
  GtkAdjustment *adjustment = user_data;
  long int oldbase = adjustment->value;
  long int base = oldbase;

  switch( scroll )
  {
  case GTK_SCROLL_JUMP:
    base = position ? adjustment->upper : adjustment->lower;
    break;
  case GTK_SCROLL_STEP_BACKWARD:
    base -= adjustment->step_increment;
    break;
  case GTK_SCROLL_STEP_FORWARD:
    base += adjustment->step_increment;
    break;
  case GTK_SCROLL_PAGE_BACKWARD:
    base -= adjustment->page_increment;
    break;
  case GTK_SCROLL_PAGE_FORWARD:
    base += adjustment->page_increment;
    break;
  default:
    return;
  }

  if( base < 0 ) {
    base = 0;
  } else if( base > adjustment->upper - adjustment->page_size ) {
    base = adjustment->upper - adjustment->page_size;
  }

  if( base != oldbase ) gtk_adjustment_set_value( adjustment, base );
}

static gboolean
wheel_scroll_event( GtkWidget *clist GCC_UNUSED, GdkEvent *event,
		    gpointer user_data )
{
  GtkAdjustment *adjustment = user_data;
  long int oldbase = adjustment->value;
  long int base = oldbase;

  switch( event->scroll.direction )
  {
  case GDK_SCROLL_UP:
    base -= adjustment->page_increment / 2;
    break;
  case GDK_SCROLL_DOWN:
    base += adjustment->page_increment / 2;
    break;
  default:
    return FALSE;
  }

  if( base < 0 ) {
    base = 0;
  } else if( base > adjustment->upper - adjustment->page_size ) {
    base = adjustment->upper - adjustment->page_size;
  }

  if( base != oldbase ) gtk_adjustment_set_value( adjustment, base );

  return TRUE;
}

void
gtkui_scroll_connect( GtkCList *clist, GtkAdjustment *adj )
{
  gtk_signal_connect( GTK_OBJECT( clist ), "scroll-vertical",
		      GTK_SIGNAL_FUNC( key_scroll_event ), adj );
  gtk_signal_connect( GTK_OBJECT( clist ), "scroll-event",
		      GTK_SIGNAL_FUNC( wheel_scroll_event ), adj );
}
