/* periph.c: code for handling peripherals
   Copyright (c) 2005-2009 Philip Kendall

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
#include "fuller.h"
#include "disk/opus.h"
#include "ide/divide.h"
#include "ide/simpleide.h"
#include "ide/zxatasp.h"
#include "ide/zxcf.h"
#include "if1.h"
#include "if2.h"
#include "joystick.h"
#include "kempmouse.h"
#include "melodik.h"
#include "speccyboot.h"
#include "periph.h"
#include "rzx.h"
#include "settings.h"
#include "ui/ui.h"
#include "ula.h"

/*
 * General peripheral list handling routines
 */

/* Full information about a peripheral */
typedef struct periph_private_t {

  int id;
  int active;

  periph_t peripheral;

} periph_private_t;

static GSList *peripherals = NULL;
static int last_id = 0;

/* The strings used for debugger events */
static const char *page_event_string = "page",
  *unpage_event_string = "unpage";

static gint find_by_id( gconstpointer data, gconstpointer id );
static void free_peripheral( gpointer data, gpointer user_data );

/* Register a peripheral. Returns -1 on error or a peripheral ID if
   successful */
int
periph_register( const periph_t *peripheral )
{
  periph_private_t *private;

  private = malloc( sizeof( periph_private_t ) );
  if( !private ) {
    ui_error( UI_ERROR_ERROR, "Out of memory at %s:%d", __FILE__, __LINE__ );
    return -1;
  }

  private->id = last_id++;
  private->active = 1;
  private->peripheral = *peripheral;

  peripherals = g_slist_append( peripherals, private );

  return private->id;
}

/* Register many peripherals */
int
periph_register_n( const periph_t *peripherals_list, size_t n )
{
  const periph_t *ptr;

  for( ptr = peripherals_list; n--; ptr++ ) {
    int id;
    id = periph_register( ptr ); if( id == -1 ) return -1;
  }

  return 0;
}

/* (De)activate a specific peripheral */
int
periph_set_active( int id, int active )
{
  GSList *ptr;
  periph_private_t *private;

  ptr = g_slist_find_custom( peripherals, &id, find_by_id );
  if( !ptr ) {
    ui_error( UI_ERROR_ERROR, "couldn't find peripheral ID %d", id );
    return 1;
  }

  private = ptr->data;

  private->active = active;

  return 0;
}

static gint
find_by_id( gconstpointer data, gconstpointer user_data )
{
  const periph_private_t *private = data;
  int id = *(const int*)user_data;

  return private->id - id;
}

/* Clear all peripherals */
void
periph_clear( void )
{
  g_slist_foreach( peripherals, free_peripheral, NULL );
  g_slist_free( peripherals );
  peripherals = NULL;

  last_id = 0;
}

static void
free_peripheral( gpointer data, gpointer user_data GCC_UNUSED )
{
  periph_t *private = data;

  free( private );
}

/*
 * The actual routines to read and write a port
 */

static void read_peripheral( gpointer data, gpointer user_data );
static void write_peripheral( gpointer data, gpointer user_data );

/* Internal type used for passing to read_peripheral and write_peripheral */
struct peripheral_data_t {

  libspectrum_word port;

  int attached;
  libspectrum_byte value;
};

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

static void
read_peripheral( gpointer data, gpointer user_data )
{
  periph_private_t *private = data;
  struct peripheral_data_t *callback_info = user_data;

  periph_t *peripheral;

  peripheral = &( private->peripheral );

  if( private->active && peripheral->read &&
      ( ( callback_info->port & peripheral->mask ) == peripheral->value ) ) {
    callback_info->value &= peripheral->read( callback_info->port,
					      &( callback_info->attached ) );
  }
}

void
writeport( libspectrum_word port, libspectrum_byte b )
{
  ula_contend_port_early( port );
  writeport_internal( port, b );
  ula_contend_port_late( port ); tstates++;
}

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

static void
write_peripheral( gpointer data, gpointer user_data )
{
  periph_private_t *private = data;
  struct peripheral_data_t *callback_info = user_data;

  periph_t *peripheral;

  peripheral = &( private->peripheral );
  
  if( private->active && peripheral->write &&
      ( ( callback_info->port & peripheral->mask ) == peripheral->value ) )
    peripheral->write( callback_info->port, callback_info->value );
}

/*
 * The more Fuse-specific peripheral handling routines
 */

/* What sort of Kempston interface does the current machine have */
static periph_present kempston_present;

/* Is the Kempston interface currently active */
int periph_kempston_active;

/* What sort of Interface I does the current machine have */
periph_present interface1_present;

/* Is the Interface I currently active */
int periph_interface1_active;

/* What sort of Interface II does the current machine have */
periph_present interface2_present;

/* Is the Interface II currently active */
int periph_interface2_active;

/* What sort of +D interface does the current machine have */
periph_present plusd_present;

/* Is the +D currently active */
int periph_plusd_active;

/* What sort of Beta 128 interface does the current machine have */
periph_present beta128_present;

/* Is the Beta 128 currently active */
int periph_beta128_active;

/* What sort of Opus interface does the current machine have */
periph_present opus_present;

/* Is the Opus currently active */
int periph_opus_active;

/* What sort of Fuller Box does the current machine have */
periph_present fuller_present;

/* Is the Fuller Box currently active */
int periph_fuller_active;

/* What sort of Melodik does the current machine have */
periph_present melodik_present;

/* Is the Melodik currently active */
int periph_melodik_active;

/* What sort of SpeccyBoot does the current machine have */
periph_present speccyboot_present;

/* Is the SpeccyBoot active */
int periph_speccyboot_active;

int
periph_setup( const periph_t *peripherals_list, size_t n )
{
  int error;

  periph_clear();

  error =
    periph_register_n( simpleide_peripherals, simpleide_peripherals_count );
  if( error ) return error;

  periph_register_n( zxatasp_peripherals, zxatasp_peripherals_count );
  periph_register_n( zxcf_peripherals, zxcf_peripherals_count );
  periph_register_n( divide_peripherals, divide_peripherals_count );
  periph_register_n( plusd_peripherals, plusd_peripherals_count );
  periph_register_n( if1_peripherals, if1_peripherals_count );

  periph_register_n( kempmouse_peripherals, kempmouse_peripherals_count );

  periph_register_n( fuller_peripherals, fuller_peripherals_count );
  periph_register_n( melodik_peripherals, melodik_peripherals_count );

  periph_register_n( speccyboot_peripherals, speccyboot_peripherals_count );

  error = periph_register_n( peripherals_list, n ); if( error ) return error;

  kempston_present = PERIPH_PRESENT_NEVER;
  interface1_present = PERIPH_PRESENT_NEVER;
  interface2_present = PERIPH_PRESENT_NEVER;
  plusd_present = PERIPH_PRESENT_NEVER;
  beta128_present = PERIPH_PRESENT_NEVER;
  opus_present = PERIPH_PRESENT_NEVER;
  fuller_present = PERIPH_PRESENT_NEVER;
  melodik_present = PERIPH_PRESENT_NEVER;
  speccyboot_present = PERIPH_PRESENT_NEVER;

  return 0;
}

void
periph_setup_kempston( periph_present present ) {
  kempston_present = present;
}

void
periph_setup_interface1( periph_present present ) {
  interface1_present = present;
}

void
periph_setup_interface2( periph_present present ) {
  interface2_present = present;
}

void
periph_setup_plusd( periph_present present ) {
  plusd_present = present;
}

void
periph_setup_beta128( periph_present present ) {
  beta128_present = present;
}

void
periph_register_beta128( void ) {
  periph_register_n( beta_peripherals, beta_peripherals_count );
}

void
periph_setup_opus( periph_present present ) {
  opus_present = present;
}

void
periph_setup_fuller( periph_present present ) {
  fuller_present = present;
}

void
periph_setup_melodik( periph_present present ) {
  melodik_present = present;
}

void
periph_setup_speccyboot( periph_present present ) {
  speccyboot_present = present;
}

static void
update_cartridge_menu( void )
{
  int cartridge, dock;

  dock = machine_current->capabilities &
         LIBSPECTRUM_MACHINE_CAPABILITY_TIMEX_DOCK;

  cartridge = dock || periph_interface2_active;

  ui_menu_activate( UI_MENU_ITEM_MEDIA_CARTRIDGE, cartridge );
  ui_menu_activate( UI_MENU_ITEM_MEDIA_CARTRIDGE_DOCK, dock );
  ui_menu_activate( UI_MENU_ITEM_MEDIA_CARTRIDGE_IF2, periph_interface2_active );
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
  switch( kempston_present ) {
  case PERIPH_PRESENT_NEVER: periph_kempston_active = 0; break;
  case PERIPH_PRESENT_OPTIONAL:
    periph_kempston_active = settings_current.joy_kempston; break;
  case PERIPH_PRESENT_ALWAYS: periph_kempston_active = 1; break;
  }

  switch( interface1_present ) {
  case PERIPH_PRESENT_NEVER: periph_interface1_active = 0; break;
  case PERIPH_PRESENT_OPTIONAL:
    periph_interface1_active = settings_current.interface1; break;
  case PERIPH_PRESENT_ALWAYS: periph_interface1_active = 1; break;
  }

  switch( interface2_present ) {

  case PERIPH_PRESENT_NEVER:
    periph_interface2_active = 0;
    break;
  case PERIPH_PRESENT_OPTIONAL:
    periph_interface2_active = settings_current.interface2;
    break;
  case PERIPH_PRESENT_ALWAYS:
    periph_interface2_active = 1;
    break;

  }
  ui_menu_activate( UI_MENU_ITEM_MEDIA_IF1,
		    periph_interface1_active );

  ui_menu_activate( UI_MENU_ITEM_MEDIA_CARTRIDGE_IF2,
		    periph_interface2_active );

  switch( plusd_present ) {
  case PERIPH_PRESENT_NEVER: periph_plusd_active = 0; break;
  case PERIPH_PRESENT_OPTIONAL:
    periph_plusd_active = settings_current.plusd; break;
  case PERIPH_PRESENT_ALWAYS: periph_plusd_active = 1; break;
  }

  switch( beta128_present ) {
  case PERIPH_PRESENT_NEVER: periph_beta128_active = 0; break;
  case PERIPH_PRESENT_OPTIONAL:
    periph_beta128_active = settings_current.beta128; break;
  case PERIPH_PRESENT_ALWAYS: periph_beta128_active = 1;
    break;
  }

  switch( opus_present ) {
  case PERIPH_PRESENT_NEVER: periph_opus_active = 0; break;
  case PERIPH_PRESENT_OPTIONAL:
    periph_opus_active = settings_current.opus; break;
  case PERIPH_PRESENT_ALWAYS: periph_opus_active = 1; break;
  }

  if( ui_mouse_present ) {
    if( settings_current.kempston_mouse ) {
      if( !ui_mouse_grabbed ) ui_mouse_grabbed = ui_mouse_grab( 1 );
    } else {
      if(  ui_mouse_grabbed ) ui_mouse_grabbed = ui_mouse_release( 1 );
    }
  }

  switch( fuller_present ) {
  case PERIPH_PRESENT_NEVER: periph_fuller_active = 0; break;
  case PERIPH_PRESENT_OPTIONAL:
    periph_fuller_active = settings_current.fuller; break;
  case PERIPH_PRESENT_ALWAYS: periph_fuller_active = 1; break;
  }

  switch( melodik_present ) {
  case PERIPH_PRESENT_NEVER: periph_melodik_active = 0; break;
  case PERIPH_PRESENT_OPTIONAL:
    periph_melodik_active = settings_current.melodik; break;
  case PERIPH_PRESENT_ALWAYS: periph_melodik_active = 1; break;
  }

  switch (speccyboot_present ) {
  case PERIPH_PRESENT_NEVER: periph_speccyboot_active = 0; break;
  case PERIPH_PRESENT_OPTIONAL:
    periph_speccyboot_active = settings_current.speccyboot; break;
  case PERIPH_PRESENT_ALWAYS: periph_speccyboot_active = 1; break;
  }

  update_cartridge_menu();
  update_ide_menu();
  if1_update_menu();
  specplus3_765_update_fdd();
  machine_current->memory_map();
}

int
periph_register_paging_events( const char *type_string, int *page_event,
			       int *unpage_event )
{
  *page_event = debugger_event_register( type_string, page_event_string );
  *unpage_event = debugger_event_register( type_string, unpage_event_string );
  if( *page_event == -1 || *unpage_event == -1 ) return 1;

  return 0;
}
