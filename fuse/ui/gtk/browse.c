/* browse.c: tape browser dialog box
   Copyright (c) 2002-2004 Philip Kendall

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

#include <stdlib.h>
#include <string.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "compat.h"
#include "fuse.h"
#include "gtkinternals.h"
#include "menu.h"
#include "tape.h"
#include "ui/ui.h"

static int create_dialog( void );
static void add_block_details( libspectrum_tape_block *block,
			       void *user_data );
static void select_row( GtkCList *widget, gint row, gint column,
			GdkEventButton *event, gpointer data );
static void browse_done( GtkWidget *widget, gpointer data );
static gboolean delete_dialog( GtkWidget *widget, GdkEvent *event,
			       gpointer user_data );

static GdkPixmap *tape_marker_pixmap;
static GdkBitmap *tape_marker_mask;

static GtkWidget
  *dialog,			/* The dialog box itself */
  *blocks,			/* The list of blocks */
  *modified_label;		/* The label saying if the tape has been
				   modified */

static int dialog_created;	/* Have we created the dialog box yet? */

void
menu_media_tape_browse( GtkWidget *widget GCC_UNUSED,
			gpointer data GCC_UNUSED )
{
  /* Firstly, stop emulation */
  fuse_emulation_pause();

  if( !dialog_created )
    if( create_dialog() ) { fuse_emulation_unpause(); return; }

  if( ui_tape_browser_update( UI_TAPE_BROWSER_NEW_TAPE, NULL ) ) {
    fuse_emulation_unpause();
    return;
  }

  gtk_widget_show_all( dialog );

  /* Carry on with emulation */
  fuse_emulation_unpause();
}

static int
create_dialog( void )
{
  GtkWidget *scrolled_window;

  size_t i;
  gchar *titles[3] = { "", "Block type", "Data" };

  /* Give me a new dialog box */
  dialog = gtkstock_dialog_new( "Fuse - Browse Tape",
				G_CALLBACK( delete_dialog ) );

  /* And a scrolled window to pack the CList into */
  scrolled_window = gtk_scrolled_window_new( NULL, NULL );
  gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW( scrolled_window ),
				  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC );
  gtk_box_pack_start_defaults( GTK_BOX( GTK_DIALOG( dialog )->vbox ),
			       scrolled_window );

  /* And the CList itself */
  blocks = gtk_clist_new_with_titles( 3, titles );
  gtk_clist_column_titles_passive( GTK_CLIST( blocks ) );
  for( i = 0; i < 3; i++ )
    gtk_clist_set_column_auto_resize( GTK_CLIST( blocks ), i, TRUE );
  g_signal_connect( GTK_OBJECT( blocks ), "select-row",
		    G_CALLBACK( select_row ), NULL );
  gtk_container_add( GTK_CONTAINER( scrolled_window ), blocks );

  /* The tape marker pixmap */
  tape_marker_pixmap = 
    gdk_pixmap_colormap_create_from_xpm_d( NULL, gdk_rgb_get_cmap(),
					   &tape_marker_mask, NULL,
					   gtkpixmap_tape_marker );

  /* And the "tape modified" label */
  modified_label = gtk_label_new( "" );
  gtk_box_pack_start( GTK_BOX( GTK_DIALOG( dialog )->vbox ), modified_label,
		      FALSE, FALSE, 0 );

  /* Create the OK button */
  gtkstock_create_close( dialog, NULL, G_CALLBACK( browse_done ), FALSE );

  /* Make the window big enough to show at least some data */
  gtk_window_set_default_size( GTK_WINDOW( dialog ), -1, 200 );

  dialog_created = 1;

  return 0;
}

int
ui_tape_browser_update( ui_tape_browser_update_type change GCC_UNUSED,
                        libspectrum_tape_block *block GCC_UNUSED )
{
  int error, current_block;

  if( !dialog_created ) return 0;

  fuse_emulation_pause();
  gtk_clist_freeze( GTK_CLIST( blocks ) );

  gtk_clist_clear( GTK_CLIST( blocks ) );

  error = tape_foreach( add_block_details, blocks );
  if( error ) {
    gtk_clist_thaw( GTK_CLIST( blocks ) );
    fuse_emulation_unpause();
    return 1;
  }

  current_block = tape_get_current_block();
  if( current_block != -1 )
    gtk_clist_set_pixmap( GTK_CLIST( blocks ), current_block, 0,
			  tape_marker_pixmap, tape_marker_mask );

  gtk_clist_thaw( GTK_CLIST( blocks ) );

  if( tape_modified ) {
    gtk_label_set_text( GTK_LABEL( modified_label ), "Tape modified" );
  } else {
    gtk_label_set_text( GTK_LABEL( modified_label ), "Tape not modified" );
  }

  fuse_emulation_unpause();

  return 0;
}

static void
add_block_details( libspectrum_tape_block *block, void *user_data )
{
  GtkWidget *clist = user_data;

  gchar buffer[256];
  gchar *details[3] = { &buffer[0], &buffer[80], &buffer[160] };

  strcpy( details[0], "" );
  libspectrum_tape_block_description( details[1], 80, block );
  tape_block_details( details[2], 80, block );

  gtk_clist_append( GTK_CLIST( clist ), details );
}

/* Called when a row is selected */
static void
select_row( GtkCList *clist, gint row, gint column GCC_UNUSED,
	    GdkEventButton *event, gpointer data GCC_UNUSED )
{
  int current_block;

  /* Ignore events which aren't double-clicks or select-via-keyboard */
  if( event && event->type != GDK_2BUTTON_PRESS ) return;

  /* Don't do anything if the current block was clicked on */
  current_block = tape_get_current_block();
  if( row == current_block ) return;

  /* Otherwise, select the new block */
  tape_select_block_no_update( row );

  gtk_clist_set_pixmap( clist, row, 0, tape_marker_pixmap, tape_marker_mask );
  if( current_block != -1 ) gtk_clist_set_text( clist, current_block, 0, "" );
}

/* Called if the OK button is clicked */
static void
browse_done( GtkWidget *widget GCC_UNUSED, gpointer data GCC_UNUSED )
{
  gtk_widget_hide_all( dialog );
}

/* Catch attempts to delete the window and just hide it instead */
static gboolean
delete_dialog( GtkWidget *widget, GdkEvent *event GCC_UNUSED,
	       gpointer user_data GCC_UNUSED )
{
  gtk_widget_hide_all( widget );
  return TRUE;
}
