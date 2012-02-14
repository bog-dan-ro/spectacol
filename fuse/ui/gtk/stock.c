/* stock.c: 'standard' GTK+ widgets etc
   Copyright (c) 2004 Darren Salt, Philip Kendall

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

   Philip Kendall <philip-fuse@shadowmagic.org.uk>

*/

#include <config.h>

#include <string.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "gtkinternals.h"

GtkAccelGroup*
gtkstock_add_accel_group( GtkWidget *widget )
{
  GtkAccelGroup *accel = gtk_accel_group_new();
  gtk_window_add_accel_group( GTK_WINDOW( widget ), accel );
  return accel;
}

static void
add_click( GtkWidget *btn, const gtkstock_button *button, GtkAccelGroup *accel,
	   guint key1, GdkModifierType mod1, guint key2, GdkModifierType mod2 )
{
  if( button->shortcut ) {
    if( button->shortcut != GDK_VoidSymbol )
      gtk_widget_add_accelerator( btn, "clicked", accel, button->shortcut,
				  button->modifier, 0 );
  } else if( key1 ) {
    gtk_widget_add_accelerator( btn, "clicked", accel, key1, mod1, 0 );
  }

  if( button->shortcut_alt ) {
    if( button->shortcut_alt != GDK_VoidSymbol )
      gtk_widget_add_accelerator( btn, "clicked", accel, button->shortcut_alt,
				  button->modifier_alt, 0 );
  }
  else if( key2 ) {
    gtk_widget_add_accelerator( btn, "clicked", accel, key2, mod2, 0 );
  }

}

GtkWidget*
gtkstock_create_button( GtkWidget *widget, GtkAccelGroup *accel,
			const gtkstock_button *button )
{
  GtkWidget *btn;
  gboolean link_object = ( button->label[0] == '!' );
  gboolean is_stock = !strncmp( button->label, "gtk-", 4 );

  if( !accel ) accel = gtkstock_add_accel_group (widget);

  if( is_stock ) {
    btn = gtk_button_new_from_stock( button->label + link_object );

/* FIXME: for future use

  else if( button->shortcut ) {
    button = gtk_button_new_with_mnemonic( button->label + link_object )
  }
*/

  } else {
    btn = gtk_button_new_with_label( button->label + link_object +
				     ( is_stock ? 4 : 0 ) );
  }

  if( GTK_IS_DIALOG( widget ) ) {
    gtk_container_add( GTK_CONTAINER( GTK_DIALOG( widget )->action_area ),
		       btn );
  } else {
    gtk_container_add( GTK_CONTAINER( widget ), btn );
  }

  if( button->action ) {
    if( link_object ) {
      gtk_signal_connect_object( GTK_OBJECT( btn ), "clicked",
				 button->action,
				 GTK_OBJECT( button->actiondata ) );
    } else {
      gtk_signal_connect( GTK_OBJECT( btn ), "clicked", button->action,
			  button->actiondata );
    }
  }

  if( button->destroy )
    gtk_signal_connect_object( GTK_OBJECT( btn ), "clicked", button->destroy,
			       GTK_OBJECT( widget ) );

  if( is_stock ) {
    if( !strcmp( button->label, GTK_STOCK_CLOSE ) )
      add_click( btn, button, accel, GDK_Return, 0, GDK_Escape, 0 );
    else if( !strcmp( button->label, GTK_STOCK_OK ) )
      add_click( btn, button, accel, GDK_Return, 0, 0, 0 );
    else if( !strcmp( button->label, GTK_STOCK_CANCEL ) )
      add_click( btn, button, accel, GDK_Escape, 0, 0, 0 );
    else if( !strcmp( button->label, GTK_STOCK_SAVE ) )
      add_click( btn, button, accel, GDK_S, GDK_MOD1_MASK, 0, 0 );
    else if( !strcmp( button->label, GTK_STOCK_YES ) )
      add_click( btn, button, accel, GDK_Y, GDK_MOD1_MASK, GDK_Return, 0 );
    else if( !strcmp( button->label, GTK_STOCK_NO ) )
      add_click( btn, button, accel, GDK_N, GDK_MOD1_MASK, GDK_Escape, 0 );
    else
      add_click( btn, button, accel, 0, 0, 0, 0 );
  } else {
    add_click( btn, button, accel, 0, 0, 0, 0 );
  }

  return btn;
}

GtkAccelGroup*
gtkstock_create_buttons( GtkWidget *widget, GtkAccelGroup *accel,
			 const gtkstock_button *buttons, size_t count )
{
  if( !accel ) accel = gtkstock_add_accel_group( widget );

  for( ; count; buttons++, count-- )
    gtkstock_create_button( widget, accel, buttons );

  return accel;
}

GtkAccelGroup*
gtkstock_create_ok_cancel( GtkWidget *widget, GtkAccelGroup *accel,
			   GCallback action, gpointer actiondata,
			   GCallback destroy )
{
  gtkstock_button btn[] = {
    { GTK_STOCK_CANCEL, NULL, NULL, NULL, 0, 0, 0, 0 },
    { GTK_STOCK_OK, NULL, NULL, NULL, 0, 0, 0, 0 },
  };
  btn[1].destroy = btn[0].destroy = destroy ? destroy : DEFAULT_DESTROY;
  btn[1].action = action;
  btn[1].actiondata = actiondata;

  return gtkstock_create_buttons( widget, accel, btn,
				  sizeof( btn ) / sizeof( btn[0] ) );
}

GtkAccelGroup*
gtkstock_create_close( GtkWidget *widget, GtkAccelGroup *accel,
		       GCallback destroy, gboolean esconly )
{
  gtkstock_button btn = {
    GTK_STOCK_CLOSE, NULL, NULL, (destroy ? destroy : DEFAULT_DESTROY),
    (esconly ? GDK_VoidSymbol : 0), 0, 0, 0
  };
  return gtkstock_create_buttons( widget, accel, &btn, 1 );
}

GtkWidget*
gtkstock_dialog_new( const gchar *title, GCallback destroy )
{
  GtkWidget *dialog = gtk_dialog_new();
  if( title ) gtk_window_set_title( GTK_WINDOW( dialog ), title );
  gtk_signal_connect( GTK_OBJECT( dialog ), "delete-event",
		      destroy ? destroy : DEFAULT_DESTROY, NULL );
  if( destroy == NULL ) gtk_window_set_modal( GTK_WINDOW( dialog ), TRUE );
  gtk_window_set_transient_for( GTK_WINDOW( dialog ),
                                GTK_WINDOW( gtkui_window ) );

  return dialog;
}
