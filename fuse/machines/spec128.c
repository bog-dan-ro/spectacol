/* spec128.c: Spectrum 128K specific routines
   Copyright (c) 1999-2011 Philip Kendall

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

#include <libspectrum.h>

#include "ay.h"
#include "compat.h"
#include "disk/beta.h"
#include "joystick.h"
#include "machine.h"
#include "memory.h"
#include "periph.h"
#include "settings.h"
#include "spec128.h"
#include "spec48.h"
#include "specplus3.h"
#include "ula.h"
#include "if1.h"

static int spec128_reset( void );

const periph_t spec128_peripherals[] = {
  { 0x8002, 0x0000, NULL, spec128_memoryport_write },
};

const size_t spec128_peripherals_count =
  sizeof( spec128_peripherals ) / sizeof( periph_t );

int spec128_init( fuse_machine_info *machine )
{
  machine->machine = LIBSPECTRUM_MACHINE_128;
  machine->id = "128";

  machine->reset = spec128_reset;

  machine->timex = 0;
  machine->ram.port_from_ula	     = spec48_port_from_ula;
  machine->ram.contend_delay	     = spectrum_contend_delay_65432100;
  machine->ram.contend_delay_no_mreq = spectrum_contend_delay_65432100;

  machine->unattached_port = spectrum_unattached_port;

  machine->shutdown = NULL;

  machine->memory_map = spec128_memory_map;

  return 0;
}

static int
spec128_reset( void )
{
  int error;

  error = machine_load_rom( 0, 0, settings_current.rom_128_0,
                            settings_default.rom_128_0, 0x4000 );
  if( error ) return error;
  error = machine_load_rom( 2, 1, settings_current.rom_128_1,
                            settings_default.rom_128_1, 0x4000 );
  if( error ) return error;

  error = periph_setup( spec128_peripherals, spec128_peripherals_count );
  if( error ) return error;

  error = spec128_common_reset( 1 );
  if( error ) return error;

  spec128_common_peripherals();
  periph_update();

  beta_builtin = 0;

  spec48_common_display_setup();

  return 0;
}

void
spec128_common_peripherals( void )
{
  specplus3_common_peripherals();

  /* Peripherals available on the 48K and 128K */
  periph_set_present( PERIPH_TYPE_AY, PERIPH_PRESENT_ALWAYS );
  periph_set_present( PERIPH_TYPE_BETA128, PERIPH_PRESENT_OPTIONAL );
  periph_set_present( PERIPH_TYPE_INTERFACE1, PERIPH_PRESENT_OPTIONAL );
  periph_set_present( PERIPH_TYPE_INTERFACE2, PERIPH_PRESENT_OPTIONAL );
  periph_set_present( PERIPH_TYPE_OPUS, PERIPH_PRESENT_OPTIONAL );
  periph_set_present( PERIPH_TYPE_PLUSD, PERIPH_PRESENT_OPTIONAL );
}

int
spec128_common_reset( int contention )
{
  size_t i;

  machine_current->ram.locked=0;
  machine_current->ram.last_byte = 0;

  machine_current->ram.current_page=0;
  machine_current->ram.current_rom=0;

  memory_current_screen = 5;
  memory_screen_mask = 0xffff;

  /* ROM 0, RAM 5, RAM 2, RAM 0 */
  memory_map_home[0] = &memory_map_rom[ 0];
  memory_map_home[1] = &memory_map_rom[ 1];

  memory_map_home[2] = &memory_map_ram[10];
  memory_map_home[3] = &memory_map_ram[11];

  memory_map_home[4] = &memory_map_ram[ 4];
  memory_map_home[5] = &memory_map_ram[ 5];

  memory_map_home[6] = &memory_map_ram[ 0];
  memory_map_home[7] = &memory_map_ram[ 1];

  /* Mark as present/writeable */
  for( i = 0; i < 16; i++ )
    memory_map_ram[i].writable = 1;

  /* Odd pages contended on the 128K/+2; the loop is up to 16 to
     ensure all of the Scorpion's 256Kb RAM is not contended */
  for( i = 0; i < 16; i++ )
    memory_map_ram[ 2 * i     ].contended =
      memory_map_ram[ 2 * i + 1 ].contended = i & 1 ? contention : 0;

  for( i = 0; i < 8; i++ )
    memory_map_read[i] = memory_map_write[i] = *memory_map_home[i];

  return 0;
}

void
spec128_memoryport_write( libspectrum_word port GCC_UNUSED,
			  libspectrum_byte b )
{
  if( machine_current->ram.locked ) return;

  machine_current->ram.last_byte = b;

  machine_current->memory_map();

  machine_current->ram.locked = b & 0x20;
}

void
spec128_select_rom( int rom )
{
  memory_map_home[0] = &memory_map_rom[ 2 * rom     ];
  memory_map_home[1] = &memory_map_rom[ 2 * rom + 1 ];
  machine_current->ram.current_rom = rom;
}

void
spec128_select_page( int page )
{
  memory_map_home[6] = &memory_map_ram[ 2 * page     ];
  memory_map_home[7] = &memory_map_ram[ 2 * page + 1 ];
  machine_current->ram.current_page = page;
}

int
spec128_memory_map( void )
{
  int page, screen, rom;
  size_t i;

  page = machine_current->ram.last_byte & 0x07;
  screen = ( machine_current->ram.last_byte & 0x08 ) ? 7 : 5;
  rom = ( machine_current->ram.last_byte & 0x10 ) >> 4;

  /* If we changed the active screen, mark the entire display file as
     dirty so we redraw it on the next pass */
  if( memory_current_screen != screen ) {
    display_update_critical( 0, 0 );
    display_refresh_main_screen();
    memory_current_screen = screen;
  }

  spec128_select_rom( rom );
  spec128_select_page( page );

  for( i = 0; i < 8; i++ )
    memory_map_read[i] = memory_map_write[i] = *memory_map_home[i];

  memory_romcs_map();

  return 0;
}
