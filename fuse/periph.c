/* periph.c: code for handling peripherals
   Copyright (c) 2005-2011 Philip Kendall

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

#include <libspectrum.h>

#include "debugger/debugger.h"
#include "event.h"
#include "fuse.h"
#include "ide/divide.h"
#include "ide/simpleide.h"
#include "ide/zxatasp.h"
#include "ide/zxcf.h"
#include "if1.h"
#include "if2.h"
#include "joystick.h"
#include "periph.h"
#include "rzx.h"
#include "settings.h"
#include "ui/ui.h"
#include "ula.h"

/*
 * General peripheral list handling routines
 */

/* One peripheral and all the ports it responds to */
typedef struct periph_type_t {
  /* Can this peripheral ever be present on the currently emulated machine? */
  periph_present present;
  /* Is this peripheral currently active? */
  int active; 
  /* The preferences option which controls this peripheral */
  int *option;
  /* The list of ports this peripheral responds to */
  const periph_t *peripherals;
} periph_type_t;

/* All the peripherals we know about */
static GHashTable *peripherals_by_type = NULL;

/* Wrapper to pair up a port response with the peripheral it came from */
typedef struct periph_private_t {
  /* The peripheral this came from */
  periph_type type;
  /* The port response */
  periph_t peripheral;
} periph_private_t;

/* The list of currently active peripherals */
static GSList *peripherals = NULL;

/* The strings used for debugger events */
static const char *page_event_string = "page",
  *unpage_event_string = "unpage";

/* Place one port response in the list of currently active ones */
static void
periph_register( periph_type type, const periph_t *peripheral )
{
  periph_private_t *private;

  private = malloc( sizeof( *private ) );
  if( !private ) {
    ui_error( UI_ERROR_ERROR, "Out of memory at %s:%d", __FILE__, __LINE__ );
    fuse_abort();
  }

  private->type = type;
  private->peripheral = *peripheral;

  peripherals = g_slist_append( peripherals, private );
}

/* Register a peripheral with the system */
void
periph_register_type( periph_type type, int *option, const periph_t *peripherals )
{
  if( !peripherals_by_type )
    peripherals_by_type = g_hash_table_new( NULL, NULL );

  periph_type_t *type_data = malloc( sizeof( *type_data ) );
  if( !type_data ) {
    ui_error( UI_ERROR_ERROR, "Out of memory at %s:%d", __FILE__, __LINE__ );
    fuse_abort();
  }

  type_data->present = PERIPH_PRESENT_NEVER;
  type_data->active = 0;
  type_data->option = option;
  type_data->peripherals = peripherals;

  g_hash_table_insert( peripherals_by_type, GINT_TO_POINTER( type ), type_data );
}

/* FIXME: remove this function */
static void
periph_register_n( const periph_t *peripherals_list, size_t n )
{
  const periph_t *ptr;
  for( ptr = peripherals_list; n--; ptr++ ) periph_register( PERIPH_TYPE_UNKNOWN, ptr );
}

/* Get the data about one peripheral */
static gint
find_by_type( gconstpointer data, gconstpointer user_data )
{
  const periph_private_t *periph = data;
  periph_type type = GPOINTER_TO_INT( user_data );
  return periph->type - type;
}

/* Set whether a peripheral can be present on this machine or not */
void
periph_set_present( periph_type type, periph_present present )
{
  periph_type_t *type_data = g_hash_table_lookup( peripherals_by_type, GINT_TO_POINTER( type ) );
  if( type_data ) type_data->present = present;
}

/* Mark a specific peripheral as (in)active */
void
periph_activate_type( periph_type type, int active )
{
  periph_type_t *type_data = g_hash_table_lookup( peripherals_by_type, GINT_TO_POINTER( type ) );
  if( !type_data || type_data->active == active ) return;

  type_data->active = active;

  if( active ) {
    const periph_t *ptr;
    for( ptr = type_data->peripherals; ptr && ptr->mask != 0; ptr++ )
      periph_register( type, ptr );
  } else {
    GSList *found;
    while( ( found = g_slist_find_custom( peripherals, GINT_TO_POINTER( type ), find_by_type ) ) != NULL )
      peripherals = g_slist_remove( peripherals, found->data );
  }
}

/* Is a specific peripheral active at the moment? */
int
periph_is_active( periph_type type )
{
  periph_type_t *type_data = g_hash_table_lookup( peripherals_by_type, GINT_TO_POINTER( type ) );
  return type_data ? type_data->active : 0;
}

/* Work out whether a peripheral is present on this machine, and mark it
   (in)active as appropriate */
static void
set_activity( gpointer key, gpointer value, gpointer user_data )
{
  periph_type type = GPOINTER_TO_INT( key );
  periph_type_t *type_data = value;
  int active;

  switch ( type_data->present ) {
  case PERIPH_PRESENT_NEVER: active = 0; break;
  case PERIPH_PRESENT_OPTIONAL: active = *(type_data->option); break;
  case PERIPH_PRESENT_ALWAYS: active = 1; break;
  }

  periph_activate_type( type, active );
}

/* Free the memory used by a peripheral-port response pair */
static void
free_peripheral( gpointer data, gpointer user_data GCC_UNUSED )
{
  periph_private_t *private = data;
  free( private );
}

/* Make a peripheral as being never present on this machine */
static void
set_type_inactive( gpointer key, gpointer value, gpointer user_data )
{
  periph_type_t *type_data = value;
  type_data->present = PERIPH_PRESENT_NEVER;
  type_data->active = 0;
}

/* Mark all peripherals as being never present on this machine */
static void
set_types_inactive( void )
{
  g_hash_table_foreach( peripherals_by_type, set_type_inactive, NULL );
}

/* Empty out the list of peripherals */
void
periph_clear( void )
{
  g_slist_foreach( peripherals, free_peripheral, NULL );
  g_slist_free( peripherals );
  peripherals = NULL;
  set_types_inactive();
}

/*
 * The actual routines to read and write a port
 */

/* Internal type used for passing to read_peripheral and write_peripheral */
struct peripheral_data_t {

  libspectrum_word port;

  int attached;
  libspectrum_byte value;
};

/* Read a byte from a port, taking the appropriate time */
libspectrum_byte
readport( libspectrum_word port )
{
  libspectrum_byte b;

  ula_contend_port_early( port );
  ula_contend_port_late( port );
  b = readport_internal( port );

  /* Very ugly to put this here, but unless anything else needs this
     "writeback" mechanism, no point producing a general framework */
  if( ( port & 0x8002 ) == 0 &&
      ( machine_current->machine == LIBSPECTRUM_MACHINE_128   ||
	machine_current->machine == LIBSPECTRUM_MACHINE_PLUS2    ) )
    writeport_internal( 0x7ffd, b );

  tstates++;

  return b;
}

/* Read a byte from a specific port response */
static void
read_peripheral( gpointer data, gpointer user_data )
{
  periph_private_t *private = data;
  struct peripheral_data_t *callback_info = user_data;

  periph_t *peripheral;

  peripheral = &( private->peripheral );

  if( peripheral->read &&
      ( ( callback_info->port & peripheral->mask ) == peripheral->value ) ) {
    callback_info->value &= peripheral->read( callback_info->port,
					      &( callback_info->attached ) );
  }
}

/* Read a byte from a port, taking no time */
libspectrum_byte
readport_internal( libspectrum_word port )
{
  struct peripheral_data_t callback_info;

  /* Trigger the debugger if wanted */
  if( debugger_mode != DEBUGGER_MODE_INACTIVE )
    debugger_check( DEBUGGER_BREAKPOINT_TYPE_PORT_READ, port );

  /* If we're doing RZX playback, get a byte from the RZX file */
  if( rzx_playback ) {

    libspectrum_error error;
    libspectrum_byte value;

    error = libspectrum_rzx_playback( rzx, &value );
    if( error ) {
      rzx_stop_playback( 1 );

      /* Add a null event to mean we pick up the RZX state change in
	 z80_do_opcodes() */
      event_add( tstates, event_type_null );
      return readport_internal( port );
    }

    return value;
  }

  /* If we're not doing RZX playback, get the byte normally */
  callback_info.port = port;
  callback_info.attached = 0;
  callback_info.value = 0xff;

  g_slist_foreach( peripherals, read_peripheral, &callback_info );

  if( !callback_info.attached )
    callback_info.value = machine_current->unattached_port();

  /* If we're RZX recording, store this byte */
  if( rzx_recording ) rzx_store_byte( callback_info.value );

  return callback_info.value;
}

/* Write a byte to a port, taking the appropriate time */
void
writeport( libspectrum_word port, libspectrum_byte b )
{
  ula_contend_port_early( port );
  writeport_internal( port, b );
  ula_contend_port_late( port ); tstates++;
}

/* Write a byte to a specific port response */
static void
write_peripheral( gpointer data, gpointer user_data )
{
  periph_private_t *private = data;
  struct peripheral_data_t *callback_info = user_data;

  periph_t *peripheral;

  peripheral = &( private->peripheral );
  
  if( peripheral->write &&
      ( ( callback_info->port & peripheral->mask ) == peripheral->value ) )
    peripheral->write( callback_info->port, callback_info->value );
}

/* Write a byte to a port, taking no time */
void
writeport_internal( libspectrum_word port, libspectrum_byte b )
{
  struct peripheral_data_t callback_info;

  /* Trigger the debugger if wanted */
  if( debugger_mode != DEBUGGER_MODE_INACTIVE )
    debugger_check( DEBUGGER_BREAKPOINT_TYPE_PORT_WRITE, port );

  callback_info.port = port;
  callback_info.value = b;
  
  g_slist_foreach( peripherals, write_peripheral, &callback_info );
}

/*
 * The more Fuse-specific peripheral handling routines
 */

int
periph_setup( const periph_t *peripherals_list, size_t n )
{
  periph_clear();
  periph_register_n( peripherals_list, n );
  return 0;
}

static void
update_cartridge_menu( void )
{
  int cartridge, dock, if2;

  dock = machine_current->capabilities &
         LIBSPECTRUM_MACHINE_CAPABILITY_TIMEX_DOCK;
  if2 = periph_is_active( PERIPH_TYPE_INTERFACE2 );

  cartridge = dock || if2;

  ui_menu_activate( UI_MENU_ITEM_MEDIA_CARTRIDGE, cartridge );
  ui_menu_activate( UI_MENU_ITEM_MEDIA_CARTRIDGE_DOCK, dock );
  ui_menu_activate( UI_MENU_ITEM_MEDIA_CARTRIDGE_IF2, if2 );
}

static void
update_ide_menu( void )
{
  int ide, simpleide, zxatasp, zxcf, divide;

  simpleide = settings_current.simpleide_active;
  zxatasp = settings_current.zxatasp_active;
  zxcf = settings_current.zxcf_active;
  divide = settings_current.divide_enabled;

  ide = simpleide || zxatasp || zxcf || divide;

  ui_menu_activate( UI_MENU_ITEM_MEDIA_IDE, ide );
  ui_menu_activate( UI_MENU_ITEM_MEDIA_IDE_SIMPLE8BIT, simpleide );
  ui_menu_activate( UI_MENU_ITEM_MEDIA_IDE_ZXATASP, zxatasp );
  ui_menu_activate( UI_MENU_ITEM_MEDIA_IDE_ZXCF, zxcf );
  ui_menu_activate( UI_MENU_ITEM_MEDIA_IDE_DIVIDE, divide );
}

void
periph_update( void )
{
  if( ui_mouse_present ) {
    if( settings_current.kempston_mouse ) {
      if( !ui_mouse_grabbed ) ui_mouse_grabbed = ui_mouse_grab( 1 );
    } else {
      if(  ui_mouse_grabbed ) ui_mouse_grabbed = ui_mouse_release( 1 );
    }
  }

  g_hash_table_foreach( peripherals_by_type, set_activity, NULL );

  ui_menu_activate( UI_MENU_ITEM_MEDIA_IF1,
		    periph_is_active( PERIPH_TYPE_INTERFACE1 ) );
  ui_menu_activate( UI_MENU_ITEM_MEDIA_CARTRIDGE_IF2,
		    periph_is_active( PERIPH_TYPE_INTERFACE2 ) );

  update_cartridge_menu();
  update_ide_menu();
  if1_update_menu();
  specplus3_765_update_fdd();
  machine_current->memory_map();
}

/* Register debugger page/unpage events for a peripheral */
int
periph_register_paging_events( const char *type_string, int *page_event,
			       int *unpage_event )
{
  *page_event = debugger_event_register( type_string, page_event_string );
  *unpage_event = debugger_event_register( type_string, unpage_event_string );
  if( *page_event == -1 || *unpage_event == -1 ) return 1;

  return 0;
}
