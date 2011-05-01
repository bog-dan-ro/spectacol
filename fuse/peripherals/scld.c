/* scld.c: Routines for handling the Timex SCLD
   Copyright (c) 2002-2006 Fredrick Meunier, Philip Kendall, Witold Filipczyk

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

   Philip: philip-fuse@shadowmagic.org.uk

   Fred: fredm@spamcop.net

*/

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "compat.h"
#include "dck.h"
#include "display.h"
#include "machine.h"
#include "memory.h"
#include "module.h"
#include "periph.h"
#include "scld.h"
#include "spectrum.h"
#include "ui/ui.h"
#include "z80/z80.h"

scld scld_last_dec;                 /* The last byte sent to Timex DEC port */

libspectrum_byte scld_last_hsr = 0; /* The last byte sent to Timex HSR port */

memory_page timex_exrom[8];
memory_page timex_dock[8];

static void scld_reset( int hard_reset );
static void scld_from_snapshot( libspectrum_snap *snap );
static void scld_to_snapshot( libspectrum_snap *snap );

static module_info_t scld_module_info = {

  scld_reset,
  NULL,
  NULL,
  scld_from_snapshot,
  scld_to_snapshot,

};

static libspectrum_byte scld_dec_read( libspectrum_word port, int *attached );
static libspectrum_byte scld_hsr_read( libspectrum_word port, int *attached );

static const periph_port_t scld_ports[] = {
  { 0x00ff, 0x00f4, scld_hsr_read, scld_hsr_write },
  { 0x00ff, 0x00ff, scld_dec_read, scld_dec_write },
  { 0, 0, NULL, NULL }
};

static const periph_t scld_periph = {
  NULL,
  scld_ports
};

int
scld_init( void )
{
  module_register( &scld_module_info );
  periph_register( PERIPH_TYPE_SCLD, &scld_periph );
  return 0;
}

static libspectrum_byte
scld_dec_read( libspectrum_word port GCC_UNUSED, int *attached )
{
  *attached = 1;

  return scld_last_dec.byte;
}

void
scld_dec_write( libspectrum_word port GCC_UNUSED, libspectrum_byte b )
{
  scld old_dec = scld_last_dec;
  scld new_dec;
  libspectrum_byte ink,paper;

  /* We use new_dec as we don't want to have the new colours, modes etc.
     to take effect until we have updated the critical region */
  new_dec.byte = b;

  /* If we changed the active screen, or change the colour in hires
   * mode, update the critical region and mark the entire display file as
   * dirty so we redraw it on the next pass */
  if( new_dec.mask.scrnmode != old_dec.mask.scrnmode ||
      new_dec.name.hires != old_dec.name.hires ||
      ( new_dec.name.hires &&
           ( new_dec.mask.hirescol != old_dec.mask.hirescol ) ) ) {
    display_update_critical( 0, 0 );
    display_refresh_main_screen();
  }

  /* Commit change to scld_last_dec */
  scld_last_dec = new_dec;

  /* If we just reenabled interrupts, check for a retriggered interrupt */
  if( old_dec.name.intdisable && !scld_last_dec.name.intdisable )
    z80_interrupt();

  if( scld_last_dec.name.altmembank != old_dec.name.altmembank )
    machine_current->memory_map();

  display_parse_attr( hires_get_attr(), &ink, &paper );
  display_set_hires_border( paper );
}

static void
scld_reset( int hard_reset GCC_UNUSED )
{
  scld_last_dec.byte = 0;
}

void
scld_hsr_write( libspectrum_word port GCC_UNUSED, libspectrum_byte b )
{
  scld_last_hsr = b;

  machine_current->memory_map();
}

static libspectrum_byte
scld_hsr_read( libspectrum_word port GCC_UNUSED, int *attached )
{
  *attached = 1;

  return scld_last_hsr;
}

libspectrum_byte
hires_get_attr( void )
{
  return( hires_convert_dec( scld_last_dec.byte ) );
}

libspectrum_byte
hires_convert_dec( libspectrum_byte attr )
{
  scld colour;

  colour.byte = attr;

  switch ( colour.mask.hirescol )
  {
    case BLACKWHITE:   return 0x47;
    case BLUEYELLOW:   return 0x4e;
    case REDCYAN:      return 0x55;
    case MAGENTAGREEN: return 0x5c;
    case GREENMAGENTA: return 0x63;
    case CYANRED:      return 0x6a;
    case YELLOWBLUE:   return 0x71;
    default:	       return 0x78; /* WHITEBLACK */
  }
}

void
scld_memory_map( void )
{
  int i;
  memory_page **exrom_dock;
  
  exrom_dock =
    scld_last_dec.name.altmembank ? memory_map_exrom : memory_map_dock;

  for( i = 0; i < 8; i++ ) {
    if( scld_last_hsr & ( 1 << i ) ) {
      memory_map_read[i] = memory_map_write[i] = *exrom_dock[i];
    } else {
      memory_map_read[i] = memory_map_write[i] = *memory_map_home[i];
    }
  }
}

static int
scld_dock_exrom_from_snapshot( memory_page *dest, int page_num, int writable,
                               void *source )
{
  dest->offset = 0;
  dest->page_num = page_num;
  dest->writable = writable;
  dest->page =
    memory_pool_allocate( MEMORY_PAGE_SIZE * sizeof( libspectrum_byte ) );
  if( !dest->page ) {
    ui_error( UI_ERROR_ERROR, "Out of memory at %s:%d", __FILE__,
              __LINE__ );
    return 1;
  }

  memcpy( dest->page, source, MEMORY_PAGE_SIZE );

  return 0;
}

static void
scld_from_snapshot( libspectrum_snap *snap )
{
  size_t i;
  int capabilities = machine_current->capabilities;

  if( capabilities & ( LIBSPECTRUM_MACHINE_CAPABILITY_TIMEX_MEMORY |
      LIBSPECTRUM_MACHINE_CAPABILITY_SE_MEMORY ) )
    scld_hsr_write( 0x00fd, libspectrum_snap_out_scld_hsr( snap ) );

  if( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_TIMEX_VIDEO )
    scld_dec_write( 0x00ff, libspectrum_snap_out_scld_dec( snap ) );

  if( libspectrum_snap_dock_active( snap ) ) {

    dck_active = 1;

    for( i = 0; i < 8; i++ ) {

      if( libspectrum_snap_dock_cart( snap, i ) ) {
        if( scld_dock_exrom_from_snapshot( memory_map_dock[i], i,
                                           libspectrum_snap_dock_ram( snap, i ),
                                           libspectrum_snap_dock_cart( snap, i ) ) )
          return;
      }

      if( libspectrum_snap_exrom_cart( snap, i ) ) {
        if( scld_dock_exrom_from_snapshot( memory_map_exrom[i], i,
                                           libspectrum_snap_exrom_ram( snap, i ),
                                           libspectrum_snap_exrom_cart( snap, i ) ) )
          return;
      }

    }

    if( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_TIMEX_DOCK )
      ui_menu_activate( UI_MENU_ITEM_MEDIA_CARTRIDGE_DOCK_EJECT, 1 );

    machine_current->memory_map();
  }

}

static void
scld_to_snapshot( libspectrum_snap *snap )
{
  size_t i;
  libspectrum_byte *buffer;

  libspectrum_snap_set_out_scld_hsr( snap, scld_last_hsr );
  libspectrum_snap_set_out_scld_dec( snap, scld_last_dec.byte );

  if( dck_active ) {

    libspectrum_snap_set_dock_active( snap, 1 );

    for( i = 0; i < 8; i++ ) {

      if( memory_map_exrom[i]->save_to_snapshot ||
          memory_map_exrom[i]->writable            ) {
        buffer = malloc( MEMORY_PAGE_SIZE * sizeof( libspectrum_byte ) );
        if( !buffer ) {
          ui_error( UI_ERROR_ERROR, "Out of memory at %s:%d", __FILE__,
                    __LINE__ );
          return;
        }

        libspectrum_snap_set_exrom_ram( snap, i,
                                        memory_map_exrom[i]->writable );
        memcpy( buffer, memory_map_exrom[i]->page, MEMORY_PAGE_SIZE );
        libspectrum_snap_set_exrom_cart( snap, i, buffer );
      }

      if( memory_map_dock[i]->save_to_snapshot ||
	  memory_map_dock[i]->writable            ) {
        buffer = malloc( MEMORY_PAGE_SIZE * sizeof( libspectrum_byte ) );
        if( !buffer ) {
          ui_error( UI_ERROR_ERROR, "Out of memory at %s:%d", __FILE__,
                    __LINE__ );
          return;
        }

        libspectrum_snap_set_dock_ram( snap, i, memory_map_dock[i]->writable );
        memcpy( buffer, memory_map_dock[i]->page, MEMORY_PAGE_SIZE );
        libspectrum_snap_set_dock_cart( snap, i, buffer );
      }

    }

  }
}
