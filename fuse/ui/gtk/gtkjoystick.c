/* gtkjoystick.c: Joystick emulation
   Copyright (c) 2003-2004 Darren Salt, Philip Kendall

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

   Darren: linux@youmustbejoking.demon.co.uk

*/

#include <config.h>

#include <gtk/gtk.h>

#include "compat.h"
#include "fuse.h"
#include "gtkinternals.h"
#include "joystick.h"
#include "keyboard.h"
#include "menu.h"
#include "settings.h"

#if !defined USE_JOYSTICK || defined HAVE_JSW_H
/* Fake joystick, or override UI-specific handling */
#include "../uijoystick.c"

#else			/* #if !defined USE_JOYSTICK || defined HAVE_JSW_H */

#include "../sdl/sdljoystick.c"

struct button_info {
  int *setting;
  char name[80];
  GtkWidget *label;
  keyboard_key_name key;
};

struct joystick_info {

  int *type;
  GtkWidget *radio[ JOYSTICK_TYPE_COUNT ];

  struct button_info button[10];
};

static void setup_info( struct joystick_info *info, int callback_action );
static void create_joystick_type_selector( struct joystick_info *info,
					   GtkBox *parent );
static void
create_fire_button_selector( const char *title, struct button_info *info,
			     GtkBox *parent );
static void set_key_text( GtkWidget *label, keyboard_key_name key );

static void key_callback( gpointer data, guint action, GtkWidget *widget );
static void nothing_callback( gpointer data, guint action, GtkWidget *widget );
static void joystick_done( GtkButton *button, gpointer user_data );

static GtkItemFactoryEntry key_menu[] = {

  { "/Joystick Fire", NULL, key_callback, KEYBOARD_JOYSTICK_FIRE, "<Item>", NULL },

  { "/Numbers/0", NULL, key_callback, KEYBOARD_0, "<Item>", NULL },
  { "/Numbers/1", NULL, key_callback, KEYBOARD_1, "<Item>", NULL },
  { "/Numbers/2", NULL, key_callback, KEYBOARD_2, "<Item>", NULL },
  { "/Numbers/3", NULL, key_callback, KEYBOARD_3, "<Item>", NULL },
  { "/Numbers/4", NULL, key_callback, KEYBOARD_4, "<Item>", NULL },
  { "/Numbers/5", NULL, key_callback, KEYBOARD_5, "<Item>", NULL },
  { "/Numbers/6", NULL, key_callback, KEYBOARD_6, "<Item>", NULL },
  { "/Numbers/7", NULL, key_callback, KEYBOARD_7, "<Item>", NULL },
  { "/Numbers/8", NULL, key_callback, KEYBOARD_8, "<Item>", NULL },
  { "/Numbers/9", NULL, key_callback, KEYBOARD_9, "<Item>", NULL },

  { "/A-M/A", NULL, key_callback, KEYBOARD_a, "<Item>", NULL },
  { "/A-M/B", NULL, key_callback, KEYBOARD_b, "<Item>", NULL },
  { "/A-M/C", NULL, key_callback, KEYBOARD_c, "<Item>", NULL },
  { "/A-M/D", NULL, key_callback, KEYBOARD_d, "<Item>", NULL },
  { "/A-M/E", NULL, key_callback, KEYBOARD_e, "<Item>", NULL },
  { "/A-M/F", NULL, key_callback, KEYBOARD_f, "<Item>", NULL },
  { "/A-M/G", NULL, key_callback, KEYBOARD_g, "<Item>", NULL },
  { "/A-M/H", NULL, key_callback, KEYBOARD_h, "<Item>", NULL },
  { "/A-M/I", NULL, key_callback, KEYBOARD_i, "<Item>", NULL },
  { "/A-M/J", NULL, key_callback, KEYBOARD_j, "<Item>", NULL },
  { "/A-M/K", NULL, key_callback, KEYBOARD_k, "<Item>", NULL },
  { "/A-M/L", NULL, key_callback, KEYBOARD_l, "<Item>", NULL },
  { "/A-M/M", NULL, key_callback, KEYBOARD_m, "<Item>", NULL },

  { "/N-Z/N", NULL, key_callback, KEYBOARD_n, "<Item>", NULL },
  { "/N-Z/O", NULL, key_callback, KEYBOARD_o, "<Item>", NULL },
  { "/N-Z/P", NULL, key_callback, KEYBOARD_p, "<Item>", NULL },
  { "/N-Z/Q", NULL, key_callback, KEYBOARD_q, "<Item>", NULL },
  { "/N-Z/R", NULL, key_callback, KEYBOARD_r, "<Item>", NULL },
  { "/N-Z/S", NULL, key_callback, KEYBOARD_s, "<Item>", NULL },
  { "/N-Z/T", NULL, key_callback, KEYBOARD_t, "<Item>", NULL },
  { "/N-Z/U", NULL, key_callback, KEYBOARD_u, "<Item>", NULL },
  { "/N-Z/V", NULL, key_callback, KEYBOARD_v, "<Item>", NULL },
  { "/N-Z/W", NULL, key_callback, KEYBOARD_w, "<Item>", NULL },
  { "/N-Z/X", NULL, key_callback, KEYBOARD_x, "<Item>", NULL },
  { "/N-Z/Y", NULL, key_callback, KEYBOARD_y, "<Item>", NULL },
  { "/N-Z/Z", NULL, key_callback, KEYBOARD_z, "<Item>", NULL },

  { "/Space", NULL, key_callback, KEYBOARD_space, "<Item>", NULL },
  { "/Enter", NULL, key_callback, KEYBOARD_Enter, "<Item>", NULL },
  { "/Caps Shift", NULL, key_callback, KEYBOARD_Caps, "<Item>", NULL },
  { "/Symbol Shift", NULL, key_callback, KEYBOARD_Symbol, "<Item>", NULL },

  /* Different callback needed as KEYBOARD_NONE has the value 0 */
  { "/Nothing", NULL, nothing_callback, 1, "<Item>", NULL },

};

static const guint key_menu_count = sizeof( key_menu ) / sizeof( key_menu[0] );

void
menu_options_joysticks_select( gpointer callback_data GCC_UNUSED,
			       guint callback_action,
			       GtkWidget *widget GCC_UNUSED )
{
  GtkWidget *dialog, *hbox, *vbox;
  struct joystick_info info;
  size_t i;

  fuse_emulation_pause();

  setup_info( &info, callback_action );

  dialog = gtkstock_dialog_new( "Fuse - Configure Joystick", NULL );

  hbox = gtk_hbox_new( FALSE, 4 );
  gtk_container_set_border_width( GTK_CONTAINER( hbox ), 4 );
  gtk_box_pack_start( GTK_BOX( GTK_DIALOG( dialog )->vbox ), hbox,
		      FALSE, FALSE, 0 );

  create_joystick_type_selector( &info, GTK_BOX( hbox ) );

  vbox = gtk_vbox_new( FALSE, 2 );
  gtk_box_pack_start_defaults( GTK_BOX( hbox ), vbox );

  for( i = 0; i < 10; i += 5 ) {
    
    int j;
    
    vbox = gtk_vbox_new( FALSE, 2 );
    gtk_box_pack_start_defaults( GTK_BOX( hbox ), vbox );
    
    for( j = i; j < i + 5; j++ )
      if( info.button[j].setting ) {
	create_fire_button_selector( info.button[j].name, &( info.button[j] ),
				     GTK_BOX( vbox ) );
      }
  }

  gtkstock_create_ok_cancel( dialog, NULL, GTK_SIGNAL_FUNC( joystick_done ),
			     &info, NULL );

  gtk_widget_show_all( dialog );
  gtk_main();

  fuse_emulation_unpause();
}

static void
setup_info( struct joystick_info *info, int callback_action )
{
  size_t i;

  switch( callback_action ) {

  case 1:
    info->type = &( settings_current.joystick_1_output );
    info->button[0].setting = &( settings_current.joystick_1_fire_1  );
    info->button[1].setting = &( settings_current.joystick_1_fire_2  );
    info->button[2].setting = &( settings_current.joystick_1_fire_3  );
    info->button[3].setting = &( settings_current.joystick_1_fire_4  );
    info->button[4].setting = &( settings_current.joystick_1_fire_5  );
    info->button[5].setting = &( settings_current.joystick_1_fire_6  );
    info->button[6].setting = &( settings_current.joystick_1_fire_7  );
    info->button[7].setting = &( settings_current.joystick_1_fire_8  );
    info->button[8].setting = &( settings_current.joystick_1_fire_9  );
    info->button[9].setting = &( settings_current.joystick_1_fire_10 );
    for( i = 0; i < 10; i++ )
      snprintf( info->button[i].name, 80, "Button %lu", (unsigned long)i + 1 );
    break;

  case 2:
    info->type = &( settings_current.joystick_2_output );
    info->button[0].setting = &( settings_current.joystick_2_fire_1  );
    info->button[1].setting = &( settings_current.joystick_2_fire_2  );
    info->button[2].setting = &( settings_current.joystick_2_fire_3  );
    info->button[3].setting = &( settings_current.joystick_2_fire_4  );
    info->button[4].setting = &( settings_current.joystick_2_fire_5  );
    info->button[5].setting = &( settings_current.joystick_2_fire_6  );
    info->button[6].setting = &( settings_current.joystick_2_fire_7  );
    info->button[7].setting = &( settings_current.joystick_2_fire_8  );
    info->button[8].setting = &( settings_current.joystick_2_fire_9  );
    info->button[9].setting = &( settings_current.joystick_2_fire_10 );
    for( i = 0; i < 10; i++ )
      snprintf( info->button[i].name, 80, "Button %lu", (unsigned long)i + 1 );
    break;

  case 3:
    info->type = &( settings_current.joystick_keyboard_output );
    info->button[0].setting = &( settings_current.joystick_keyboard_up  );
    snprintf( info->button[0].name, 80, "Button for UP" );
    info->button[1].setting = &( settings_current.joystick_keyboard_down  );
    snprintf( info->button[1].name, 80, "Button for DOWN" );
    info->button[2].setting = &( settings_current.joystick_keyboard_left  );
    snprintf( info->button[2].name, 80, "Button for LEFT" );
    info->button[3].setting = &( settings_current.joystick_keyboard_right  );
    snprintf( info->button[3].name, 80, "Button for RIGHT" );
    info->button[4].setting = &( settings_current.joystick_keyboard_fire  );
    snprintf( info->button[4].name, 80, "Button for FIRE" );
    for( i = 5; i < 10; i++ ) info->button[i].setting = NULL;
    break;

  }
}

static void
create_joystick_type_selector( struct joystick_info *info, GtkBox *parent )
{
  GtkWidget *frame, *box;
  GSList *button_group;
  size_t i;

  frame = gtk_frame_new( "Joystick type" );
  gtk_box_pack_start( parent, frame, FALSE, FALSE, 0 );

  box = gtk_vbox_new( FALSE, 0 );
  gtk_container_add( GTK_CONTAINER( frame ), box );

  button_group = NULL;

  for( i = 0; i < JOYSTICK_TYPE_COUNT; i++ ) {

    info->radio[ i ] =
      gtk_radio_button_new_with_label( button_group, joystick_name[ i ] );
    button_group =
      gtk_radio_button_group( GTK_RADIO_BUTTON( info->radio[ i ] ) );
    gtk_box_pack_start( GTK_BOX( box ), info->radio[ i ], FALSE, FALSE, 0 );

    gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( info->radio[ i ] ),
				  i == *( info->type ) );

  }

}

static void
create_fire_button_selector( const char *title, struct button_info *info,
			     GtkBox *parent )
{
  GtkWidget *menu, *frame, *box;
  GtkItemFactory *factory;
  size_t i;

  frame = gtk_frame_new( title );
  gtk_box_pack_start_defaults( parent, frame );

  box = gtk_hbox_new( FALSE, 4 );
  gtk_container_set_border_width( GTK_CONTAINER( box ), 2 );
  gtk_container_add( GTK_CONTAINER( frame ), box );

  info->key = *info->setting;
  info->label = gtk_label_new( "" );

  for( i = 0; i < key_menu_count; i++ ) {
    
    keyboard_key_name key;
      
    if( key_menu[i].callback == nothing_callback ) {
      key = KEYBOARD_NONE;
    } else {
      key = key_menu[i].callback_action;
    }

    if( key == *info->setting ) {
      set_key_text( info->label, key );
      break;
    }

  }

  gtk_box_pack_start_defaults( GTK_BOX( box ), info->label );

  factory = gtk_item_factory_new( GTK_TYPE_OPTION_MENU, "<fire>", NULL );
  gtk_item_factory_create_items( factory, key_menu_count, key_menu, info );

  menu = gtk_item_factory_get_widget( factory, "<fire>" );
  gtk_box_pack_start_defaults( GTK_BOX( box ), menu );
}

static void
set_key_text( GtkWidget *label, keyboard_key_name key )
{
  const char *text;
  char buffer[40];

  text = keyboard_key_text( key );

  snprintf( buffer, 40, "%s", text );

  gtk_label_set_text( GTK_LABEL( label ), buffer );
}


static void
key_callback( gpointer data, guint action, GtkWidget *widget GCC_UNUSED )
{
  struct button_info *info = data;

  info->key = action;
  set_key_text( info->label, info->key );
}

static void
nothing_callback( gpointer data, guint action GCC_UNUSED,
		  GtkWidget *widget GCC_UNUSED )
{
  key_callback( data, KEYBOARD_NONE, NULL );
}

static void
joystick_done( GtkButton *button GCC_UNUSED, gpointer user_data )
{
  struct joystick_info *info = user_data;

  int i;
  GtkToggleButton *toggle;

  for( i = 0; i < 10; i++ )
    if( info->button[i].setting )
      *info->button[i].setting = info->button[i].key;

  for( i = 0; i < JOYSTICK_TYPE_COUNT; i++ ) {

    toggle = GTK_TOGGLE_BUTTON( info->radio[ i ] );

    if( gtk_toggle_button_get_active( toggle ) ) {
      *( info->type ) = i;
      return;
    }

  }

}

#endif			/* #if !defined USE_JOYSTICK || defined HAVE_JSW_H */
