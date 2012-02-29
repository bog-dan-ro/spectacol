/* memory.c: the GTK+ memory browser
   Copyright (c) 2004-2005 Philip Kendall

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
#include <string.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "compat.h"
#include "fuse.h"
#include "gtkinternals.h"
#include "memory.h"
#include "menu.h"

static libspectrum_word memaddr = 0x0000;

static void
update_display( GtkCList *clist, libspectrum_word base )
{
  size_t i, j;

  gchar buffer[ 8 + 64 + 20 ];
  gchar *text[] = { &buffer[0], &buffer[ 8 ], &buffer[ 8 + 64 ] };
  char buffer2[ 8 ];

  memaddr = base;
  gtk_clist_freeze( clist );
  gtk_clist_clear( clist );

  for( i = 0; i < 20; i++ ) {
    snprintf( text[0], 8, "%04X", base );

    text[1][0] = '\0';
    for( j = 0; j < 0x10; j++, base++ ) {

      libspectrum_byte b = readbyte_internal( base );

      snprintf( buffer2, 4, "%02X ", b );
      strncat( text[1], buffer2, 4 );

      text[2][j] = ( b >= 32 && b < 127 ) ? b : '.';
    }
    text[2][ 0x10 ] = '\0';

    gtk_clist_append( clist, text );
  }

  gtk_clist_thaw( clist );
}

static void
scroller( GtkAdjustment *adjustment, gpointer user_data )
{
  libspectrum_word base;
  GtkCList *clist = user_data;

  /* Drop the low bits before displaying anything */
  base = adjustment->value; base &= 0xfff0;

  update_display( clist, base );
}

void
menu_machine_memorybrowser( GtkWidget *widget GCC_UNUSED,
			    gpointer data GCC_UNUSED )
{
  GtkWidget *dialog, *box, *clist, *scrollbar;
  GtkObject *adjustment;
  size_t i;
  int error;
  gtkui_font font;

  fuse_emulation_pause();
  gchar *titles[] = { "Address", "Hex", "Data" };

  error = gtkui_get_monospaced_font( &font ); if( error ) return;

  dialog = gtkstock_dialog_new( "Fuse - Memory Browser", NULL );

  gtkstock_create_close( dialog, NULL, NULL, FALSE );

  box = gtk_hbox_new( FALSE, 0 );
  gtk_box_pack_start_defaults( GTK_BOX( GTK_DIALOG( dialog )->vbox ), box );

  clist = gtk_clist_new_with_titles( 3, titles );
  gtk_clist_column_titles_passive( GTK_CLIST( clist ) );
  for( i = 0; i < 3; i++ )
    gtk_clist_set_column_auto_resize( GTK_CLIST( clist ), i, TRUE );
  gtkui_set_font( clist, font );
  update_display( GTK_CLIST( clist ), memaddr );
  gtk_box_pack_start_defaults( GTK_BOX( box ), clist );

  adjustment = gtk_adjustment_new( memaddr, 0x0000, 0xffff, 0x10, 0xa0, 0x13f );
  g_signal_connect( adjustment, "value-changed", G_CALLBACK( scroller ),
		    clist );

  gtkui_scroll_connect( GTK_CLIST( clist ), GTK_ADJUSTMENT( adjustment ) );

  scrollbar = gtk_vscrollbar_new( GTK_ADJUSTMENT( adjustment ) );
  gtk_box_pack_start( GTK_BOX( box ), scrollbar, FALSE, FALSE, 0 );

  gtk_widget_show_all( dialog );
  gtk_main();

  gtkui_free_font( font );
  fuse_emulation_unpause();

  return;
}
