/* usource.c: Routines for handling the Currah uSource interface
   Copyright (c) 2007,2011 Stuart Brady

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

   Stuart: stuart.brady@gmail.com

*/

#include <config.h>

#include <libspectrum.h>

#include "compat.h"
#include "machine.h"
#include "memory.h"
#include "module.h"
#include "periph.h"
#include "settings.h"
#include "usource.h"

/* An 8 KiB memory chunk accessible by the Z80 when /ROMCS is low
 * (mirrored in the second 8 KiB when active) */
static memory_page usource_memory_map_romcs[ MEMORY_PAGES_IN_8K ];
static int usource_memory_source;

int usource_active = 0;
int usource_available = 0;

static void usource_toggle_write( libspectrum_word port,
				  libspectrum_byte val );
static libspectrum_byte usource_toggle_read( libspectrum_word port,
					     int *attached );

static void usource_reset( int hard_reset );
static void usource_memory_map( void );

static module_info_t usource_module_info = {

  usource_reset,
  usource_memory_map,
  NULL, /* enabled_snapshot */
  NULL, /* from_snapshot */
  NULL, /* to_snapshot */

};

static const periph_port_t usource_ports[] = {
  { 0xffff, 0x2bae, usource_toggle_read, usource_toggle_write },

  { 0, 0, NULL, NULL }
};

static const periph_t usource_periph = {
  &settings_current.usource,
  usource_ports,
  1,
  NULL
};

int
usource_init( void )
{
  int i;

  module_register( &usource_module_info );

  usource_memory_source = memory_source_register( "uSource" );
  for( i = 0; i < MEMORY_PAGES_IN_8K; i++ )
    usource_memory_map_romcs[i].source = usource_memory_source;

  periph_register( PERIPH_TYPE_USOURCE, &usource_periph );

  return 0;
}

void
usource_end( void )
{
  usource_available = 0;
}

static void
usource_reset( int hard_reset GCC_UNUSED )
{
  usource_active = 0;
  usource_available = 0;

  if( !periph_is_active( PERIPH_TYPE_USOURCE ) )
    return;

  if( machine_load_rom_bank( usource_memory_map_romcs, 0,
			     settings_current.rom_usource,
			     settings_default.rom_usource, 0x2000 ) ) {
    settings_current.usource = 0;
    periph_activate_type( PERIPH_TYPE_USOURCE, 0 );
    return;
  }

  machine_current->ram.romcs = 0;

  usource_available = 1;
}

void
usource_toggle( void )
{
  usource_active = !usource_active;
  machine_current->ram.romcs = usource_active;
  machine_current->memory_map();
}

static void
usource_memory_map( void )
{
  if( !usource_active ) return;

  memory_map_romcs_8k( 0x0000, usource_memory_map_romcs );
  memory_map_romcs_8k( 0x2000, usource_memory_map_romcs );
}

static libspectrum_byte
usource_toggle_read( libspectrum_word port GCC_UNUSED,
		     int *attached GCC_UNUSED )
{
  usource_toggle();

  return 0xff;
}

static void
usource_toggle_write( libspectrum_word port GCC_UNUSED, libspectrum_byte val )
{
  usource_toggle();
}

int
usource_unittest( void )
{
  int r = 0;

  usource_active = 1;
  usource_memory_map();

  r += unittests_assert_8k_page( 0x0000, usource_memory_source, 0 );
  r += unittests_assert_8k_page( 0x2000, usource_memory_source, 0 );
  r += unittests_assert_16k_ram_page( 0x4000, 5 );
  r += unittests_assert_16k_ram_page( 0x8000, 2 );
  r += unittests_assert_16k_ram_page( 0xc000, 0 );

  usource_active = 0;
  machine_current->memory_map();

  r += unittests_paging_test_48( 2 );

  return r;
}
