/* spec48.c: Spectrum 48K specific routines
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

#include "disk/beta.h"
#include "joystick.h"
#include "machine.h"
#include "memory.h"
#include "periph.h"
#include "printer.h"
#include "settings.h"
#include "spec128.h"
#include "spec48.h"
#include "spectrum.h"
#include "ula.h"
#include "if1.h"

static int spec48_reset( void );

int
spec48_port_from_ula( libspectrum_word port )
{
  /* All even ports supplied by ULA */
  return !( port & 0x0001 );
}

int spec48_init( fuse_machine_info *machine )
{
  machine->machine = LIBSPECTRUM_MACHINE_48;
  machine->id = "48";

  machine->reset = spec48_reset;

  machine->timex = 0;
  machine->ram.port_from_ula         = spec48_port_from_ula;
  machine->ram.contend_delay	     = spectrum_contend_delay_65432100;
  machine->ram.contend_delay_no_mreq = spectrum_contend_delay_65432100;

  machine->unattached_port = spectrum_unattached_port;

  machine->shutdown = NULL;

  machine->memory_map = spec48_memory_map;

  return 0;
}

static int
spec48_reset( void )
{
  int error;

  error = machine_load_rom( 0, 0, settings_current.rom_48,
                            settings_default.rom_48, 0x4000 );
  if( error ) return error;

  error = periph_setup( NULL, 0 );
  if( error ) return error;

  spec48_common_peripherals();
  periph_update();

  beta_builtin = 0;

  memory_current_screen = 5;
  memory_screen_mask = 0xffff;

  spec48_common_display_setup();

  return spec48_common_reset();
}

void
spec48_common_peripherals( void )
{
  spec128_common_peripherals();

  /* No memory paging or AY chip on the 48K */
  periph_set_present( PERIPH_TYPE_128_MEMORY, PERIPH_PRESENT_NEVER );
  periph_set_present( PERIPH_TYPE_AY, PERIPH_PRESENT_NEVER );

  /* These peripherals valid for the 48K and very similar machines only */
  periph_set_present( PERIPH_TYPE_FULLER, PERIPH_PRESENT_OPTIONAL );
  periph_set_present( PERIPH_TYPE_MELODIK, PERIPH_PRESENT_OPTIONAL );
  periph_set_present( PERIPH_TYPE_ZXPRINTER, PERIPH_PRESENT_OPTIONAL );
}

void
spec48_common_display_setup( void )
{
  display_dirty = display_dirty_sinclair;
  display_write_if_dirty = display_write_if_dirty_sinclair;
  display_dirty_flashing = display_dirty_flashing_sinclair;

  memory_display_dirty = memory_display_dirty_sinclair;
}

int
spec48_common_reset( void )
{
  size_t i;

  /* ROM 0, RAM 5, RAM 2, RAM 0 */
  memory_map_home[0] = &memory_map_rom[ 0];
  memory_map_home[1] = &memory_map_rom[ 1];

  memory_map_home[2] = &memory_map_ram[10];
  memory_map_home[3] = &memory_map_ram[11];

  memory_map_home[4] = &memory_map_ram[ 4];
  memory_map_home[5] = &memory_map_ram[ 5];

  memory_map_home[6] = &memory_map_ram[ 0];
  memory_map_home[7] = &memory_map_ram[ 1];

  /* 0x4000 - 0x7fff contended */
  memory_map_home[2]->contended = memory_map_home[3]->contended = 1;

  /* 0x8000 - 0xffff not contended */
  memory_map_home[ 4]->contended = memory_map_home[ 5]->contended = 0;
  memory_map_home[ 6]->contended = memory_map_home[ 7]->contended = 0;

  /* Mark as present/writeable */
  for( i = 2; i < 8; ++i )
    memory_map_home[i]->writable = 1;

  for( i = 0; i < 8; i++ )
    memory_map_read[i] = memory_map_write[i] = *memory_map_home[i];

  return 0;
}

int
spec48_memory_map( void )
{
  /* By default, 0x0000 to 0x3fff come from the home bank */
  memory_map_read[0] = memory_map_write[0] = *memory_map_home[0];
  memory_map_read[1] = memory_map_write[1] = *memory_map_home[1];

  memory_romcs_map();

  return 0;
}
