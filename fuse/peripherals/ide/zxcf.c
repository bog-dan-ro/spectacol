/* zxcf.c: ZXCF interface routines
   Copyright (c) 2003-2008 Garry Lancaster and Philip Kendall
		 
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

   E-mail: Philip Kendall <philip-fuse@shadowmagic.org.uk>

*/

#include <config.h>

#include <string.h>

#include <libspectrum.h>

#include "debugger/debugger.h"
#include "ide.h"
#include "machine.h"
#include "memory.h"
#include "module.h"
#include "periph.h"
#include "settings.h"
#include "ui/ui.h"
#include "zxcf.h"

/* Two 8Kb memory chunks accessible by the Z80 when /ROMCS is low */
static memory_page zxcf_memory_map_romcs[2];

/*
  TBD: Allow memory size selection (128K/512K/1024K)
  TBD: Add secondary channel
*/


/* Private function prototype */

static void set_zxcf_bank( int bank );
static libspectrum_byte zxcf_memctl_read( libspectrum_word port,
					  int *attached );
static void zxcf_memctl_write( libspectrum_word port, libspectrum_byte data );
static libspectrum_byte zxcf_ide_read( libspectrum_word port, int *attached );
static void zxcf_ide_write( libspectrum_word port, libspectrum_byte data );

/* Data */

static const periph_t zxcf_peripherals[] = {
  { 0x10f4, 0x10b4, zxcf_memctl_read, zxcf_memctl_write },
  { 0x10f4, 0x00b4, zxcf_ide_read, zxcf_ide_write },
  { 0, 0, NULL, NULL }
};

static int zxcf_writeenable;

static libspectrum_ide_channel *zxcf_idechn;

#define ZXCF_PAGES 64
#define ZXCF_PAGE_LENGTH 0x4000
static libspectrum_byte ZXCFMEM[ ZXCF_PAGES ][ ZXCF_PAGE_LENGTH ];

static libspectrum_byte last_memctl;

static void zxcf_reset( int hard_reset );
static void zxcf_memory_map( void );
static void zxcf_from_snapshot( libspectrum_snap *snap );
static void zxcf_to_snapshot( libspectrum_snap *snap );

static module_info_t zxcf_module_info = {

  zxcf_reset,
  zxcf_memory_map,
  NULL,
  zxcf_from_snapshot,
  zxcf_to_snapshot,

};

/* Debugger events */
static const char *event_type_string = "zxcf";
static int page_event, unpage_event;

/* Housekeeping functions */

int
zxcf_init( void )
{
  int error, i;

  last_memctl = 0x00;
                                
  zxcf_idechn = libspectrum_ide_alloc( LIBSPECTRUM_IDE_DATA16 );

  ui_menu_activate( UI_MENU_ITEM_MEDIA_IDE_ZXCF_EJECT, 0 );

  if( settings_current.zxcf_pri_file ) {
    error = libspectrum_ide_insert( zxcf_idechn, LIBSPECTRUM_IDE_MASTER,
				    settings_current.zxcf_pri_file );
    if( error ) return error;
    ui_menu_activate( UI_MENU_ITEM_MEDIA_IDE_ZXCF_EJECT, 1 );
  }

  module_register( &zxcf_module_info );
  for( i = 0; i < 2; i++ ) zxcf_memory_map_romcs[i].bank = MEMORY_BANK_ROMCS;

  periph_register_type( PERIPH_TYPE_ZXCF, &settings_current.zxcf_active,
                        zxcf_peripherals );
  if( periph_register_paging_events( event_type_string, &page_event,
				     &unpage_event ) )
    return 1;

  return 0;
}

int
zxcf_end( void )
{
  return libspectrum_ide_free( zxcf_idechn );
}

static void
zxcf_reset( int hard_reset GCC_UNUSED )
{
  if( !settings_current.zxcf_active ) return;

  machine_current->ram.romcs = 1;

  set_zxcf_bank( 0 );
  zxcf_writeenable = 0;
  machine_current->memory_map();

  libspectrum_ide_reset( zxcf_idechn );
}

static int
zxcf_commit_wrapper( libspectrum_ide_unit unit )
{
  if( unit != LIBSPECTRUM_IDE_MASTER ) {
    ui_error( UI_ERROR_ERROR, "%s:%d: unit is %d, not LIBSPECTRUM_IDE_MASTER",
	      __FILE__, __LINE__, unit );
    abort();
  }

  return zxcf_commit();
}

int
zxcf_insert( const char *filename )
{
  return ide_insert( filename, zxcf_idechn, LIBSPECTRUM_IDE_MASTER,
		     zxcf_commit_wrapper, &settings_current.zxcf_pri_file,
		     UI_MENU_ITEM_MEDIA_IDE_ZXCF_EJECT );
}

int
zxcf_commit( void )
{
  int error;

  error = libspectrum_ide_commit( zxcf_idechn, LIBSPECTRUM_IDE_MASTER );

  return error;
}

int
zxcf_eject( void )
{
  return ide_eject( zxcf_idechn, LIBSPECTRUM_IDE_MASTER, zxcf_commit_wrapper,
		    &settings_current.zxcf_pri_file,
		    UI_MENU_ITEM_MEDIA_IDE_ZXCF_EJECT );
}

static void
set_zxcf_bank( int bank )
{
  memory_page *page;
  size_t i, offset;

  for( i = 0; i < 2; i++ ) {

    page = &zxcf_memory_map_romcs[i];
    offset = i & 1 ? MEMORY_PAGE_SIZE : 0x0000;
    
    page->page = &ZXCFMEM[ bank ][ offset ];
    page->writable = zxcf_writeenable;
    page->contended = 0;
    
    page->page_num = bank;
    page->offset = offset;
  }
}  

/* Port read/writes */

static libspectrum_byte
zxcf_memctl_read( libspectrum_word port GCC_UNUSED, int *attached )
{
  *attached = 1;

  return 0xff;
}

static void
zxcf_memctl_write( libspectrum_word port GCC_UNUSED, libspectrum_byte data )
{
  int was_paged = machine_current->ram.romcs;

  last_memctl = data;

  /* Bit 7 MEMOFF: 0=mem on, 1 =mem off */
  machine_current->ram.romcs = ( data & 0x80 ) ? 0 : 1;

  /* Bit 6 /MWRPROT: 0=mem protected, 1=mem writable */
  zxcf_writeenable = ( data & 0x40 ) ? 1 : 0;
  
  /* Bits 5-0: MEMBANK */
  set_zxcf_bank( data & 0x3f );

  machine_current->memory_map();

  if( machine_current->ram.romcs != was_paged )
    debugger_event( machine_current->ram.romcs ? page_event : unpage_event );
}

libspectrum_byte
zxcf_last_memctl( void )
{
  return last_memctl;
}

static libspectrum_byte
zxcf_ide_read( libspectrum_word port, int *attached )
{
  libspectrum_ide_register idereg = ( port >> 8 ) & 0x07;
  
  *attached = 1;

  return libspectrum_ide_read( zxcf_idechn, idereg ); 
}

static void
zxcf_ide_write( libspectrum_word port, libspectrum_byte data )
{
  libspectrum_ide_register idereg;
  
  idereg = ( port >> 8 ) & 0x07;
  libspectrum_ide_write( zxcf_idechn, idereg, data ); 
}

static void
zxcf_memory_map( void )
{
  if( !settings_current.zxcf_active ) return;

  if( !settings_current.zxcf_upload ) {
    memory_map_read[0] = zxcf_memory_map_romcs[0];
    memory_map_read[1] = zxcf_memory_map_romcs[1];
  }

  memory_map_write[0] = zxcf_memory_map_romcs[0];
  memory_map_write[1] = zxcf_memory_map_romcs[1];
}

static void
zxcf_from_snapshot( libspectrum_snap *snap )
{
  size_t i;

  if( !libspectrum_snap_zxcf_active( snap ) ) return;

  settings_current.zxcf_active = 1;
  settings_current.zxcf_upload = libspectrum_snap_zxcf_upload( snap );

  zxcf_memctl_write( 0x10bf, libspectrum_snap_zxcf_memctl( snap ) );

  for( i = 0; i < libspectrum_snap_zxcf_pages( snap ); i++ )
    if( libspectrum_snap_zxcf_ram( snap, i ) )
      memcpy( ZXCFMEM[ i ], libspectrum_snap_zxcf_ram( snap, i ),
	      ZXCF_PAGE_LENGTH );
}

static void
zxcf_to_snapshot( libspectrum_snap *snap )
{
  size_t i;
  libspectrum_byte *buffer;

  if( !settings_current.zxcf_active ) return;

  libspectrum_snap_set_zxcf_active( snap, 1 );
  libspectrum_snap_set_zxcf_upload( snap, settings_current.zxcf_upload );
  libspectrum_snap_set_zxcf_memctl( snap, last_memctl );
  libspectrum_snap_set_zxcf_pages( snap, ZXCF_PAGES );

  for( i = 0; i < ZXCF_PAGES; i++ ) {

    buffer = malloc( ZXCF_PAGE_LENGTH * sizeof( libspectrum_byte ) );
    if( !buffer ) {
      ui_error( UI_ERROR_ERROR, "Out of memory at %s:%d", __FILE__, __LINE__ );
      return;
    }

    memcpy( buffer, ZXCFMEM[ i ], ZXCF_PAGE_LENGTH );
    libspectrum_snap_set_zxcf_ram( snap, i, buffer );
  }
}
