/* pokefinder.c: GTK+ interface to the poke finder
   Copyright (c) 2003-2011 Philip Kendall

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

#include <errno.h>
#include <stdio.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "compat.h"
#include "debugger/debugger.h"
#include "gtkinternals.h"
#include "menu.h"
#include "pokefinder/pokefinder.h"
#include "ui/ui.h"

static int create_dialog( void );
static void gtkui_pokefinder_incremented( GtkWidget *widget,
					  gpointer user_data GCC_UNUSED );
static void gtkui_pokefinder_decremented( GtkWidget *widget,
					  gpointer user_data GCC_UNUSED );
static void gtkui_pokefinder_search( GtkWidget *widget, gpointer user_data );
static void gtkui_pokefinder_reset( GtkWidget *widget, gpointer user_data );
static void gtkui_pokefinder_close( GtkWidget *widget, gpointer user_data );
static gboolean delete_dialog( GtkWidget *widget, GdkEvent *event,
			       gpointer user_data );
static void update_pokefinder( void );
static void possible_click( GtkCList *clist, gint row, gint column,
			    GdkEventButton *event, gpointer user_data );

static int dialog_created = 0;

static GtkWidget
  *dialog,			/* The dialog box itself */
  *count_label,			/* The number of possible locations */
  *location_list;		/* The list of possible locations */

/* The possible locations */

#define MAX_POSSIBLE 20

int possible_page[ MAX_POSSIBLE ];
libspectrum_word possible_offset[ MAX_POSSIBLE ];

void
menu_machine_pokefinder( GtkWidget *widget GCC_UNUSED,
			 gpointer data GCC_UNUSED )
{
  int error;

  if( !dialog_created ) {
    error = create_dialog(); if( error ) return;
  }

  gtk_widget_show_all( dialog );
  update_pokefinder();
}

static int
create_dialog( void )
{
  GtkWidget *hbox, *vbox, *label, *entry;
  GtkAccelGroup *accel_group;
  size_t i;

  gchar *location_titles[] = { "Page", "Offset" };

  dialog = gtkstock_dialog_new( "Fuse - Poke Finder",
				G_CALLBACK( delete_dialog ) );

  hbox = gtk_hbox_new( FALSE, 0 );
  gtk_box_pack_start_defaults( GTK_BOX( GTK_DIALOG( dialog )->vbox ), hbox );

  label = gtk_label_new( "Search for:" );
  gtk_box_pack_start( GTK_BOX( hbox ), label, TRUE, TRUE, 5 );

  entry = gtk_entry_new();
  g_signal_connect( GTK_OBJECT( entry ), "activate",
		    G_CALLBACK( gtkui_pokefinder_search ), NULL );
  gtk_box_pack_start( GTK_BOX( hbox ), entry, TRUE, TRUE, 5 );

  vbox = gtk_vbox_new( FALSE, 0 );
  gtk_box_pack_start( GTK_BOX( hbox ), vbox, TRUE, TRUE, 5 );

  count_label = gtk_label_new( "" );
  gtk_box_pack_start( GTK_BOX( vbox ), count_label, TRUE, TRUE, 5 );

  location_list = gtk_clist_new_with_titles( 2, location_titles );
  gtk_clist_column_titles_passive( GTK_CLIST( location_list ) );
  for( i = 0; i < 2; i++ )
    gtk_clist_set_column_auto_resize( GTK_CLIST( location_list ), i, TRUE );
  gtk_box_pack_start( GTK_BOX( vbox ), location_list, TRUE, TRUE, 5 );

  g_signal_connect( GTK_OBJECT( location_list ), "select-row",
		    G_CALLBACK( possible_click ), NULL );

  {
    static gtkstock_button btn[] = {
      { "Incremented", G_CALLBACK( gtkui_pokefinder_incremented ), NULL, NULL, 0, 0, 0, 0 },
      { "Decremented", G_CALLBACK( gtkui_pokefinder_decremented ), NULL, NULL, 0, 0, 0, 0 },
      { "!Search", G_CALLBACK( gtkui_pokefinder_search ), NULL, NULL, GDK_Return, 0, 0, 0 },
      { "Reset", G_CALLBACK( gtkui_pokefinder_reset ), NULL, NULL, 0, 0, 0, 0 }
    };
    btn[2].actiondata = GTK_OBJECT( entry );
    accel_group = gtkstock_create_buttons( dialog, NULL, btn,
					   sizeof( btn ) / sizeof( btn[0] ) );
    gtkstock_create_close( dialog, accel_group,
			   G_CALLBACK( gtkui_pokefinder_close ), TRUE );
  }

  /* Users shouldn't be able to resize this window */
  gtk_window_set_policy( GTK_WINDOW( dialog ), FALSE, FALSE, TRUE );

  dialog_created = 1;

  return 0;
}

static void
gtkui_pokefinder_incremented( GtkWidget *widget GCC_UNUSED,
			      gpointer user_data GCC_UNUSED )
{
  pokefinder_incremented();
  update_pokefinder();
}

static void
gtkui_pokefinder_decremented( GtkWidget *widget GCC_UNUSED,
			      gpointer user_data GCC_UNUSED )
{
  pokefinder_decremented();
  update_pokefinder();
}

static void
gtkui_pokefinder_search( GtkWidget *widget, gpointer user_data GCC_UNUSED )
{
  long value;

  errno = 0;
  value = strtol( gtk_entry_get_text( GTK_ENTRY( widget ) ), NULL, 10 );

  if( errno != 0 || value < 0 || value > 255 ) {
    ui_error( UI_ERROR_ERROR, "Invalid value: use an integer from 0 to 255" );
    return;
  }

  pokefinder_search( value );
  update_pokefinder();
}

static void
gtkui_pokefinder_reset( GtkWidget *widget GCC_UNUSED,
			gpointer user_data GCC_UNUSED)
{
  pokefinder_clear();
  update_pokefinder();
}

static gboolean
delete_dialog( GtkWidget *widget, GdkEvent *event GCC_UNUSED,
	       gpointer user_data )
{
  gtkui_pokefinder_close( widget, user_data );
  return TRUE;
}

static void
gtkui_pokefinder_close( GtkWidget *widget, gpointer user_data GCC_UNUSED )
{
  gtk_widget_hide_all( widget );
}

static void
update_pokefinder( void )
{
  size_t page, offset, bank, bank_offset;
  gchar buffer[256], *possible_text[2] = { &buffer[0], &buffer[128] };

  gtk_clist_freeze( GTK_CLIST( location_list ) );
  gtk_clist_clear( GTK_CLIST( location_list ) );

  if( pokefinder_count && pokefinder_count <= MAX_POSSIBLE ) {

    size_t which;

    which = 0;

    for( page = 0; page < MEMORY_PAGES_IN_16K * SPECTRUM_RAM_PAGES; page++ ) {
      memory_page *mapping = &memory_map_ram[page];
      bank = mapping->page_num;

      for( offset = 0; offset < MEMORY_PAGE_SIZE; offset++ )
	if( ! (pokefinder_impossible[page][offset/8] & 1 << (offset & 7)) ) {
	  bank_offset = mapping->offset + offset;

	  possible_page[ which ] = bank;
	  possible_offset[ which ] = bank_offset;
	  which++;
	
	  snprintf( possible_text[0], 128, "%lu", (unsigned long)bank );
	  snprintf( possible_text[1], 128, "0x%04X", (unsigned)bank_offset );

	  gtk_clist_append( GTK_CLIST( location_list ), possible_text );
	}
    }

    gtk_widget_show( location_list );

  } else {
    gtk_widget_hide( location_list );
  }

  gtk_clist_thaw( GTK_CLIST( location_list ) );

  snprintf( buffer, 256, "Possible locations: %lu",
	    (unsigned long)pokefinder_count );
  gtk_label_set_text( GTK_LABEL( count_label ), buffer );
}  

static void
possible_click( GtkCList *clist GCC_UNUSED, gint row, gint column GCC_UNUSED,
		GdkEventButton *event, gpointer user_data GCC_UNUSED )
{
  int error;

  /* Ignore events which aren't double-clicks or select-via-keyboard */
  if( event && event->type != GDK_2BUTTON_PRESS ) return;

  error = debugger_breakpoint_add_address(
    DEBUGGER_BREAKPOINT_TYPE_WRITE, memory_source_ram, possible_page[ row ],
    possible_offset[ row ], 0, DEBUGGER_BREAKPOINT_LIFE_PERMANENT, NULL
  );
  if( error ) return;

  ui_debugger_update();
}

void
gtkui_pokefinder_clear( void )
{
  pokefinder_clear();
  if( dialog_created ) update_pokefinder();
}
