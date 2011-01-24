/* spec16.c: Spectrum 16K specific routines
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
#include <string.h>

#include <libspectrum.h>

#include "joystick.h"
#include "machine.h"
#include "machines.h"
#include "machines_periph.h"
#include "memory.h"
#include "periph.h"
#include "printer.h"
#include "settings.h"
#include "spec48.h"
#include "ula.h"
#include "if1.h"

static int spec16_reset( void );

static libspectrum_byte empty_chunk[ MEMORY_PAGE_SIZE ];
static memory_page empty_mapping;

int spec16_init( fuse_machine_info *machine )
{
  machine->machine = LIBSPECTRUM_MACHINE_16;
  machine->id = "16";

  machine->reset = spec16_reset;

  machine->timex = 0;
  machine->ram.port_from_ula  = spec48_port_from_ula;
  machine->ram.contend_delay  = spectrum_contend_delay_65432100;
  machine->ram.contend_delay_no_mreq = spectrum_contend_delay_65432100;

  memset( empty_chunk, 0xff, MEMORY_PAGE_SIZE );

  empty_mapping.page = empty_chunk;
  empty_mapping.writable = 0;
  empty_mapping.contended = 0;
  empty_mapping.bank = MEMORY_BANK_NONE;
  empty_mapping.source = MEMORY_SOURCE_SYSTEM;

  machine->unattached_port = spectrum_unattached_port;

  machine->shutdown = NULL;

  machine->memory_map = spec48_memory_map;

  return 0;
}

static int
spec16_reset( void )
{
  int error;
  size_t i;

  error = machine_load_rom( 0, 0, settings_current.rom_16, 
                            settings_default.rom_16, 0x4000 );
  if( error ) return error;

  error = periph_setup( NULL, 0 );
  if( error ) return error;

  machines_periph_48();
  periph_update();

  /* ROM 0, RAM 5, nothing, nothing */
  memory_map_home[0] = &memory_map_rom[ 0];
  memory_map_home[1] = &memory_map_rom[ 1];

  memory_map_home[2] = &memory_map_ram[10];
  memory_map_home[3] = &memory_map_ram[11];

  memory_map_home[4] = memory_map_home[5] = &empty_mapping;
  memory_map_home[6] = memory_map_home[7] = &empty_mapping;

  /* The RAM page is present/writable and contended */
  for( i = 2; i <= 3; i++ ) {
    memory_map_home[ i ]->writable = 1;
    memory_map_home[ i ]->contended = 1;
  }

  for( i = 0; i < 8; i++ )
    memory_map_read[i] = memory_map_write[i] = *memory_map_home[i];

  memory_current_screen = 5;
  memory_screen_mask = 0xffff;

  spec48_common_display_setup();

  return 0;
}
