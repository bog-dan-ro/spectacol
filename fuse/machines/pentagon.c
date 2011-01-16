/* pentagon.c: Pentagon 128K specific routines, this is intended to be a 1991
               era Pentagon machine with Beta 128 and AY as described in the
               Russian Speccy FAQ and emulated on most Spectrum emulators.
   Copyright (c) 1999-2011 Philip Kendall and Fredrick Meunier

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

#include "compat.h"
#include "disk/beta.h"
#include "joystick.h"
#include "machine.h"
#include "machines.h"
#include "memory.h"
#include "pentagon.h"
#include "periph.h"
#include "settings.h"
#include "spec48.h"
#include "spec128.h"
#include "specplus3.h"
#include "ula.h"

static int pentagon_reset( void );

const periph_t pentagon_peripherals[] = {
  { 0x00ff, 0x001f, pentagon_select_1f_read, beta_cr_write },
  { 0x00ff, 0x003f, beta_tr_read, beta_tr_write },
  { 0x00ff, 0x005f, beta_sec_read, beta_sec_write },
  { 0x00ff, 0x007f, beta_dr_read, beta_dr_write },
  { 0x00ff, 0x00fe, ula_read, ula_write },
  { 0x00ff, 0x00ff, pentagon_select_ff_read, beta_sp_write },
  { 0xc002, 0xc000, ay_registerport_read, ay_registerport_write },
  { 0xc002, 0x8000, NULL, ay_dataport_write },
  { 0x8002, 0x0000, NULL, spec128_memoryport_write },
};

const size_t pentagon_peripherals_count =
  sizeof( pentagon_peripherals ) / sizeof( periph_t );

libspectrum_byte
pentagon_select_1f_read( libspectrum_word port, int *attached )
{
  libspectrum_byte data;
  int tmpattached = 0;

  data = beta_sr_read( port, &tmpattached );
  if( !tmpattached && settings_current.joy_kempston )
    data = joystick_kempston_read( port, &tmpattached );

  if( tmpattached ) {
    *attached = 1;
    return data;
  }

  return 0xff;
}

libspectrum_byte
pentagon_select_ff_read( libspectrum_word port, int *attached )
{
  libspectrum_byte data;
  int tmpattached = 0;
  
  data = beta_sp_read( port, &tmpattached );
  if( !tmpattached )
    data = spectrum_unattached_port();

  *attached = 1;
  return data;
}

int
pentagon_port_from_ula( libspectrum_word port GCC_UNUSED )
{
  /* No contended ports */
  return 0;
}

int
pentagon_init( fuse_machine_info *machine )
{
  int i;

  machine->machine = LIBSPECTRUM_MACHINE_PENT;
  machine->id = "pentagon";

  machine->reset = pentagon_reset;

  machine->timex = 0;
  machine->ram.port_from_ula  = pentagon_port_from_ula;
  machine->ram.contend_delay  = spectrum_contend_delay_none;
  machine->ram.contend_delay_no_mreq = spectrum_contend_delay_none;

  machine->unattached_port = spectrum_unattached_port_none;

  machine->shutdown = NULL;

  machine->memory_map = spec128_memory_map;
  for( i = 0; i < 2; i++ ) beta_memory_map_romcs[i].bank = MEMORY_BANK_ROMCS;

  return 0;
}

static int
pentagon_reset(void)
{
  int error;

  error = machine_load_rom( 0, 0, settings_current.rom_pentagon_0,
                            settings_default.rom_pentagon_0, 0x4000 );
  if( error ) return error;
  error = machine_load_rom( 2, 1, settings_current.rom_pentagon_1,
                            settings_default.rom_pentagon_1, 0x4000 );
  if( error ) return error;
  error = machine_load_rom_bank( beta_memory_map_romcs, 0, 0,
                                 settings_current.rom_pentagon_2,
                                 settings_default.rom_pentagon_2, 0x4000 );
  if( error ) return error;

  error = spec128_common_reset( 0 );
  if( error ) return error;

  error = periph_setup( pentagon_peripherals, pentagon_peripherals_count );
  if( error ) return error;

  periph_update();

  beta_builtin = 1;
  beta_active = 1;

  machine_current->ram.last_byte2 = 0;
  machine_current->ram.special = 0;

  spec48_common_display_setup();

  return 0;
}

void
pentagon_common_peripherals( void )
{
  specplus3_common_peripherals();

  /* Built-in Betadisk 128 interface, which also handles Kempston joystick
     as they share a port */
  periph_set_present( PERIPH_TYPE_BETA128, PERIPH_PRESENT_ALWAYS );
  periph_set_present( PERIPH_TYPE_KEMPSTON, PERIPH_PRESENT_NEVER );
}

