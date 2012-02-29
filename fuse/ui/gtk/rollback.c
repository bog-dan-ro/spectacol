/* rollback.c: select a rollback point
   Copyright (c) 2004 Philip Kendall

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

#include <gtk/gtk.h>

#include "fuse.h"
#include "gtkinternals.h"
#include "ui/ui.h"

GtkWidget *dialog, *list;

static int dialog_created = 0;
static int current_block;

static void
select_row( GtkCList *clist GCC_UNUSED, gint row, gint column GCC_UNUSED,
	    GdkEventButton *event, gpointer data GCC_UNUSED )
{
  /* Ignore events which aren't double-clicks or select-via-keyboard */
  if( event && event->type != GDK_2BUTTON_PRESS ) return;

  current_block = row;
}

static int
create_dialog( void )
{
  gchar *title[1] = { "Seconds" };

  dialog = gtkstock_dialog_new( "Fuse - Select Rollback Point", NULL );

  gtkstock_create_ok_cancel( dialog, NULL, NULL, NULL, NULL );

  list = gtk_clist_new_with_titles( 1, title );
  gtk_clist_column_titles_passive( GTK_CLIST( list ) );
  gtk_clist_set_column_auto_resize( GTK_CLIST( list ), 0, TRUE );
  g_signal_connect( G_OBJECT( list ), "select-row",
		    G_CALLBACK( select_row ), NULL );

  gtk_box_pack_start_defaults( GTK_BOX( GTK_DIALOG( dialog )->vbox ), list );

  return 0;
}

static int
update_list( GSList *points )
{
  gtk_clist_freeze( GTK_CLIST( list ) );

  gtk_clist_clear( GTK_CLIST( list ) );

  while( points ) {
    gchar buffer[256];
    gchar *buffer2[1] = { buffer };

    snprintf( buffer, 256, "%.2f", GPOINTER_TO_INT( points->data ) / 50.0 );

    gtk_clist_append( GTK_CLIST( list ), buffer2 );

    points = points->next;
  }

  gtk_clist_thaw( GTK_CLIST( list ) );

  return 0;
}

int
ui_get_rollback_point( GSList *points )
{
  fuse_emulation_pause();

  if( !dialog_created )
    if( create_dialog() ) { fuse_emulation_unpause(); return -1; }

  if( update_list( points ) ) { fuse_emulation_unpause(); return -1; }

  current_block = -1;

  gtk_widget_show_all( dialog );

  gtk_main();

  fuse_emulation_unpause();

  return current_block;
}
