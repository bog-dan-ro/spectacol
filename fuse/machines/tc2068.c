/* tc2068.c: Timex TC2068 specific routines
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

static int tc2068_reset( void );

const periph_t tc2068_peripherals[] = {
  { 0x00ff, 0x00f4, scld_hsr_read, scld_hsr_write },

  { 0x00ff, 0x00f5, tc2068_ay_registerport_read, ay_registerport_write },
  { 0x00ff, 0x00f6, tc2068_ay_dataport_read, ay_dataport_write },

  { 0x00ff, 0x00ff, scld_dec_read, scld_dec_write },
};

const size_t tc2068_peripherals_count =
  sizeof( tc2068_peripherals ) / sizeof( periph_t );

libspectrum_byte fake_bank[ MEMORY_PAGE_SIZE ];
memory_page fake_mapping;

libspectrum_byte
tc2068_ay_registerport_read( libspectrum_word port, int *attached )
{
  if( machine_current->ay.current_register == 14 ) return 0xff;

  return ay_registerport_read( port, attached );
}

libspectrum_byte
tc2068_ay_dataport_read( libspectrum_word port, int *attached )
{
  if (machine_current->ay.current_register != 14) {
    return ay_registerport_read( port, attached );
  } else {

    libspectrum_byte ret;

    /* In theory, we may need to distinguish cases where some data
       is returned here and were it isn't. In practice, this doesn't
       matter for the TC2068 as it doesn't have a floating bus, so we'll
       get 0xff in both cases anyway */
    *attached = 1;

    ret =   machine_current->ay.registers[7] & 0x40
	  ? machine_current->ay.registers[14]
	  : 0xff;

    if( port & 0x0100 ) ret &= ~joystick_timex_read( port, 0 );
    if( port & 0x0200 ) ret &= ~joystick_timex_read( port, 1 );

    return ret;
  }
}

int
tc2068_init( fuse_machine_info *machine )
{
  machine->machine = LIBSPECTRUM_MACHINE_TC2068;
  machine->id = "2068";

  machine->reset = tc2068_reset;

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
tc2068_reset( void )
{
  size_t i;
  int error;

  error = machine_load_rom( 0, 0, settings_current.rom_tc2068_0,
                            settings_default.rom_tc2068_0, 0x4000 );
  if( error ) return error;
  error = machine_load_rom( 2, -1, settings_current.rom_tc2068_1,
                            settings_default.rom_tc2068_1, 0x2000 );
  if( error ) return error;

  error = periph_setup( tc2068_peripherals, tc2068_peripherals_count );
  if( error ) return error;

  tc2068_common_peripherals();
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

/* The peripherals common to the T[CS]2068 */
void
tc2068_common_peripherals()
{
  specplus3_common_peripherals();

  /* ULA uses full decoding */
  periph_set_present( PERIPH_TYPE_ULA, PERIPH_PRESENT_NEVER );
  periph_set_present( PERIPH_TYPE_ULA_FULL_DECODE, PERIPH_PRESENT_ALWAYS );

  /* ZX Printer and Interface 2 available */
  periph_set_present( PERIPH_TYPE_INTERFACE2, PERIPH_PRESENT_OPTIONAL );
  periph_set_present( PERIPH_TYPE_ZXPRINTER_FULL_DECODE, PERIPH_PRESENT_OPTIONAL );
}

int
tc2068_tc2048_common_reset( void )
{
  memory_current_screen = 5;
  memory_screen_mask = 0xdfff;

  scld_dec_write( 0x00ff, 0x00 );
  scld_hsr_write( 0x00f4, 0x00 );

  tc2068_tc2048_common_display_setup();

  return spec48_common_reset();
}

int
tc2068_memory_map( void )
{
  scld_memory_map();

  memory_romcs_map();

  return 0;
}

void
tc2068_tc2048_common_display_setup( void )
{
  display_dirty = display_dirty_timex;
  display_write_if_dirty = display_write_if_dirty_timex;
  display_dirty_flashing = display_dirty_flashing_timex;

  memory_display_dirty = memory_display_dirty_sinclair;
}
