/* statusbar.c: routines for updating the status bar
   Copyright (c) 2003-2004 Philip Kendall

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

#include <gtk/gtk.h>

#include "gtkinternals.h"
#include "ui/ui.h"

static GtkWidget *status_bar;

static GdkPixmap
  *pixmap_tape_inactive, *pixmap_tape_active,
  *pixmap_mdr_inactive, *pixmap_mdr_active,
  *pixmap_disk_inactive, *pixmap_disk_active,
  *pixmap_pause_inactive, *pixmap_pause_active,
  *pixmap_mouse_inactive, *pixmap_mouse_active;

static GdkBitmap *pause_mask, *mouse_mask;

static GtkWidget
  *microdrive_status,	/* Is any microdrive motor running? */
  *disk_status,		/* Is the disk motor running? */
  *mouse_status,	/* Have we grabbed the mouse? */
  *pause_status,	/* Is emulation paused (via the menu option)? */
  *tape_status,		/* Is the tape running? */
  *speed_status,	/* How fast are we running? */
  *machine_name;	/* What machine is being emulated? */

int
gtkstatusbar_create( GtkBox *parent )
{
  GtkWidget *separator;

  status_bar = gtk_hbox_new( FALSE, 5 );
  gtk_box_pack_start( parent, status_bar, FALSE, FALSE, 3 );

  pixmap_tape_inactive = 
    gdk_pixmap_colormap_create_from_xpm_d( NULL, gdk_rgb_get_cmap(), NULL,
					   NULL, gtkpixmap_tape_inactive );
  pixmap_tape_active = 
    gdk_pixmap_colormap_create_from_xpm_d( NULL, gdk_rgb_get_cmap(), NULL,
					   NULL, gtkpixmap_tape_active );

  pixmap_mdr_inactive = 
    gdk_pixmap_colormap_create_from_xpm_d( NULL, gdk_rgb_get_cmap(), NULL,
					   NULL, gtkpixmap_mdr_inactive );
  pixmap_mdr_active = 
    gdk_pixmap_colormap_create_from_xpm_d( NULL, gdk_rgb_get_cmap(), NULL,
					   NULL, gtkpixmap_mdr_active );

  pixmap_disk_inactive = 
    gdk_pixmap_colormap_create_from_xpm_d( NULL, gdk_rgb_get_cmap(), NULL,
					   NULL, gtkpixmap_disk_inactive );
  pixmap_disk_active = 
    gdk_pixmap_colormap_create_from_xpm_d( NULL, gdk_rgb_get_cmap(), NULL,
					   NULL, gtkpixmap_disk_active );

  pixmap_pause_inactive = 
    gdk_pixmap_colormap_create_from_xpm_d( NULL, gdk_rgb_get_cmap(),
					   &pause_mask, NULL,
					   gtkpixmap_pause_inactive );
  pixmap_pause_active = 
    gdk_pixmap_colormap_create_from_xpm_d( NULL, gdk_rgb_get_cmap(), NULL,
					   NULL, gtkpixmap_pause_active );

  pixmap_mouse_inactive = 
    gdk_pixmap_colormap_create_from_xpm_d( NULL, gdk_rgb_get_cmap(),
					   &mouse_mask, NULL,
					   gtkpixmap_mouse_inactive );
  pixmap_mouse_active = 
    gdk_pixmap_colormap_create_from_xpm_d( NULL, gdk_rgb_get_cmap(), NULL,
					   NULL, gtkpixmap_mouse_active );

  speed_status = gtk_label_new( "100%" );
  gtk_label_set_width_chars( GTK_LABEL( speed_status ), 8 );
  gtk_box_pack_end( GTK_BOX( status_bar ), speed_status, FALSE, FALSE, 0 );

  separator = gtk_vseparator_new();
  gtk_box_pack_end( GTK_BOX( status_bar ), separator, FALSE, FALSE, 0 );

  tape_status = gtk_pixmap_new( pixmap_tape_inactive, NULL );
  gtk_box_pack_end( GTK_BOX( status_bar ), tape_status, FALSE, FALSE, 0 );

  microdrive_status = gtk_pixmap_new( pixmap_mdr_inactive, NULL );
  gtk_box_pack_end( GTK_BOX( status_bar ), microdrive_status, FALSE, FALSE,
		    0 );

  disk_status = gtk_pixmap_new( pixmap_disk_inactive, NULL );
  gtk_box_pack_end( GTK_BOX( status_bar ), disk_status, FALSE, FALSE, 0 );

  pause_status = gtk_pixmap_new( pixmap_pause_inactive, pause_mask );
  gtk_box_pack_end( GTK_BOX( status_bar ), pause_status, FALSE, FALSE, 0 );

  mouse_status = gtk_pixmap_new( pixmap_mouse_inactive, mouse_mask );
  gtk_box_pack_end( GTK_BOX( status_bar ), mouse_status, FALSE, FALSE, 0 );

  separator = gtk_vseparator_new();
  gtk_box_pack_end( GTK_BOX( status_bar ), separator, FALSE, FALSE, 0 );

  machine_name = gtk_label_new( NULL );
  gtk_box_pack_start( GTK_BOX( status_bar ), machine_name, FALSE, FALSE, 0 );

  return 0;
}

int
gtkstatusbar_set_visibility( int visible )
{
  if( visible ) {
    gtk_widget_show( status_bar );
  } else {
    gtk_widget_hide( status_bar );
  }

  return 0;
}

void
gtkstatusbar_update_machine( const char *name )
{
  gtk_label_set_text( GTK_LABEL( machine_name ), name );
}

int
ui_statusbar_update( ui_statusbar_item item, ui_statusbar_state state )
{
  GdkPixmap *which;

  switch( item ) {

  case UI_STATUSBAR_ITEM_DISK:
    switch( state ) {
    case UI_STATUSBAR_STATE_NOT_AVAILABLE:
      gtk_widget_hide( disk_status ); break;
    case UI_STATUSBAR_STATE_ACTIVE:
      gtk_widget_show( disk_status );
      gtk_pixmap_set( GTK_PIXMAP( disk_status ), pixmap_disk_active, NULL );
      break;
    default:
      gtk_widget_show( disk_status );
      gtk_pixmap_set( GTK_PIXMAP( disk_status ), pixmap_disk_inactive, NULL );
      break;
    }      
    return 0;

  case UI_STATUSBAR_ITEM_MOUSE:
    which = ( state == UI_STATUSBAR_STATE_ACTIVE ?
	      pixmap_mouse_active : pixmap_mouse_inactive );
    gtk_pixmap_set( GTK_PIXMAP( mouse_status ), which, mouse_mask  );
    return 0;

  case UI_STATUSBAR_ITEM_PAUSED:
    which = ( state == UI_STATUSBAR_STATE_ACTIVE ?
	      pixmap_pause_active : pixmap_pause_inactive );
    gtk_pixmap_set( GTK_PIXMAP( pause_status ), which, pause_mask  );
    return 0;

  case UI_STATUSBAR_ITEM_MICRODRIVE:
    switch( state ) {
    case UI_STATUSBAR_STATE_NOT_AVAILABLE:
      gtk_widget_hide( microdrive_status ); break;
    case UI_STATUSBAR_STATE_ACTIVE:
      gtk_widget_show( microdrive_status );
      gtk_pixmap_set( GTK_PIXMAP( microdrive_status ), pixmap_mdr_active,
		      NULL );
      break;
    default:
      gtk_widget_show( microdrive_status );
      gtk_pixmap_set( GTK_PIXMAP( microdrive_status ), pixmap_mdr_inactive,
		      NULL );
      break;
    }      
    return 0;

  case UI_STATUSBAR_ITEM_TAPE:
    which = ( state == UI_STATUSBAR_STATE_ACTIVE ?
	      pixmap_tape_active : pixmap_tape_inactive );
    gtk_pixmap_set( GTK_PIXMAP( tape_status ), which, NULL  );
    return 0;

  }

  ui_error( UI_ERROR_ERROR, "Attempt to update unknown statusbar item %d",
	    item );
  return 1;
}

int
ui_statusbar_update_speed( float speed )
{
  char buffer[8];

  snprintf( buffer, 8, "%3.0f%%", speed );
  gtk_label_set_text( GTK_LABEL( speed_status ), buffer );

  return 0;
}
