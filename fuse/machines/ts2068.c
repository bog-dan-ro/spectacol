/* ts2068.c: Timex TS2068 specific routines
   Copyright (c) 1999-2011 Philip Kendall, Fredrick Meunier, Witold Filipczyk, Darren Salt

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

#include <string.h>

#include <libspectrum.h>

#include "dck.h"
#include "joystick.h"
#include "machine.h"
#include "machines.h"
#include "periph.h"
#include "printer.h"
#include "scld.h"
#include "spec48.h"
#include "settings.h"
#include "tc2068.h"
#include "ui/ui.h"
#include "ula.h"

static int ts2068_reset( void );

int
ts2068_init( fuse_machine_info *machine )
{
  machine->machine = LIBSPECTRUM_MACHINE_TS2068;
  machine->id = "ts2068";

  machine->reset = ts2068_reset;

  machine->timex = 1;
  machine->ram.port_from_ula	     = tc2048_port_from_ula;
  machine->ram.contend_delay	     = spectrum_contend_delay_65432100;
  machine->ram.contend_delay_no_mreq = spectrum_contend_delay_65432100;

  memset( fake_bank, 0xff, MEMORY_PAGE_SIZE );

  fake_mapping.page = fake_bank;
  fake_mapping.writable = 0;
  fake_mapping.contended = 0;
  fake_mapping.bank = MEMORY_BANK_DOCK;
  fake_mapping.source = MEMORY_SOURCE_SYSTEM;
  fake_mapping.offset = 0x0000;

  machine->unattached_port = spectrum_unattached_port_none;

  machine->shutdown = NULL;

  machine->memory_map = tc2068_memory_map;

  return 0;
}

static int
ts2068_reset( void )
{
  size_t i;
  int error;

  error = machine_load_rom( 0, 0, settings_current.rom_ts2068_0,
                            settings_default.rom_ts2068_0, 0x4000 );
  if( error ) return error;
  error = machine_load_rom( 2, -1, settings_current.rom_ts2068_1,
                            settings_default.rom_ts2068_1, 0x2000 );
  if( error ) return error;

  error = periph_setup( tc2068_peripherals, tc2068_peripherals_count );
  if( error ) return error;
  periph_set_present( PERIPH_TYPE_DIVIDE, PERIPH_PRESENT_OPTIONAL );
  periph_set_present( PERIPH_TYPE_KEMPSTON_MOUSE, PERIPH_PRESENT_OPTIONAL );
  periph_set_present( PERIPH_TYPE_SIMPLEIDE, PERIPH_PRESENT_OPTIONAL );
  periph_set_present( PERIPH_TYPE_SPECCYBOOT, PERIPH_PRESENT_OPTIONAL );
  periph_set_present( PERIPH_TYPE_ZXATASP, PERIPH_PRESENT_OPTIONAL );
  periph_set_present( PERIPH_TYPE_ZXCF, PERIPH_PRESENT_OPTIONAL );
  periph_update();

  for( i = 0; i < 8; i++ ) {

    timex_dock[i] = fake_mapping;
    timex_dock[i].page_num = i;
    memory_map_dock[i] = &timex_dock[i];

    timex_exrom[i] = memory_map_rom[2];
    timex_exrom[i].bank = MEMORY_BANK_EXROM;
    timex_exrom[i].page_num = i;
    memory_map_exrom[i] = &timex_exrom[i];

  }

  error = dck_reset();
  if( error ) {
    ui_error( UI_ERROR_INFO, "Ignoring Timex dock file '%s'",
            settings_current.dck_file );
  }

  return tc2068_tc2048_common_reset();
}
