/* tc2048.c: Timex TC2048 specific routines
   Copyright (c) 1999-2011 Philip Kendall, Fredrick Meunier

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

#include "machine.h"
#include "machines.h"
#include "machines_periph.h"
#include "memory.h"
#include "periph.h"
#include "peripherals/disk/beta.h"
#include "peripherals/scld.h"
#include "settings.h"
#include "spec48.h"
#include "tc2068.h"

static int tc2048_reset( void );

int
tc2048_port_from_ula( libspectrum_word port )
{
  /* Ports F4 (HSR), FE (SCLD) and FF (DEC) supplied by ULA */
  port &= 0xff;

  return( port == 0xf4 || port == 0xfe || port == 0xff );
}

int tc2048_init( fuse_machine_info *machine )
{
  machine->machine = LIBSPECTRUM_MACHINE_TC2048;
  machine->id = "2048";

  machine->reset = tc2048_reset;

  machine->timex = 1;
  machine->ram.port_from_ula	     = tc2048_port_from_ula;
  machine->ram.contend_delay	     = spectrum_contend_delay_65432100;
  machine->ram.contend_delay_no_mreq = spectrum_contend_delay_65432100;

  memset( fake_bank, 0xff, MEMORY_PAGE_SIZE );

  fake_mapping.page = fake_bank;
  fake_mapping.writable = 0;
  fake_mapping.contended = 0;
  fake_mapping.offset = 0x0000;

  machine->unattached_port = spectrum_unattached_port_none;

  machine->shutdown = NULL;

  machine->memory_map = tc2068_memory_map;

  return 0;
}

static int
tc2048_reset( void )
{
  size_t i;
  int error;

  error = machine_load_rom( 0, 0, settings_current.rom_tc2048,
                            settings_default.rom_tc2048, 0x4000 );
  if( error ) return error;

  periph_clear();
  machines_periph_48();

  /* ULA uses full decoding */
  periph_set_present( PERIPH_TYPE_ULA, PERIPH_PRESENT_NEVER );
  periph_set_present( PERIPH_TYPE_ULA_FULL_DECODE, PERIPH_PRESENT_ALWAYS );

  /* As does the ZX Printer */
  periph_set_present( PERIPH_TYPE_ZXPRINTER, PERIPH_PRESENT_NEVER );
  periph_set_present( PERIPH_TYPE_ZXPRINTER_FULL_DECODE, PERIPH_PRESENT_OPTIONAL );

  /* SCLD always present */
  periph_set_present( PERIPH_TYPE_SCLD, PERIPH_PRESENT_ALWAYS );

  /* TC2048 has a built-in Kempston joystick, which uses the "loose"
     decoding */
  periph_set_present( PERIPH_TYPE_KEMPSTON, PERIPH_PRESENT_NEVER );
  periph_set_present( PERIPH_TYPE_KEMPSTON_LOOSE, PERIPH_PRESENT_ALWAYS );

  /* SpeccyBoot doesn't seem to work on the TC2048 */
  periph_set_present( PERIPH_TYPE_SPECCYBOOT, PERIPH_PRESENT_NEVER );

  periph_update();

  beta_builtin = 0;

  for( i = 0; i < 8; i++ ) {

    timex_dock[i] = fake_mapping;
    timex_dock[i].bank= MEMORY_BANK_DOCK;
    timex_dock[i].page_num = i;
    timex_dock[i].source= MEMORY_SOURCE_SYSTEM;
    memory_map_dock[i] = &timex_dock[i];

    timex_exrom[i] = fake_mapping;
    timex_exrom[i].bank = MEMORY_BANK_EXROM;
    timex_exrom[i].page_num = i;
    timex_exrom[i].source= MEMORY_SOURCE_SYSTEM;
    memory_map_exrom[i] = &timex_exrom[i];

  }

  return tc2068_tc2048_common_reset();
}
