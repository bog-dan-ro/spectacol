/* pentagon1024.c: Pentagon 1024 specific routines This machine is expected to
                  be a post-1996 Pentagon (a 1024k v2.2 1024SL?).
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
#include "machines_periph.h"
#include "memory.h"
#include "pentagon.h"
#include "periph.h"
#include "settings.h"
#include "spec128.h"
#include "spec48.h"
#include "ula.h"

static int pentagon1024_reset( void );
static void pentagon1024_memoryport_write( libspectrum_word port GCC_UNUSED,
					   libspectrum_byte b);
static void pentagon1024_v22_memoryport_write( libspectrum_word port GCC_UNUSED,
					      libspectrum_byte b);
static int pentagon1024_memory_map( void );

static const periph_t peripherals[] = {
  { 0x00ff, 0x001f, pentagon_select_1f_read, beta_cr_write },
  { 0x00ff, 0x003f, beta_tr_read, beta_tr_write },
  { 0x00ff, 0x005f, beta_sec_read, beta_sec_write },
  { 0x00ff, 0x007f, beta_dr_read, beta_dr_write },
  { 0x00ff, 0x00ff, beta_sp_read, beta_sp_write },
  { 0xc002, 0x4000, NULL, pentagon1024_memoryport_write  },
  { 0xf008, 0xe000, NULL, pentagon1024_v22_memoryport_write }, /* v2.2 */
};

static const size_t peripherals_count =
  sizeof( peripherals ) / sizeof( periph_t );

int
pentagon1024_init( fuse_machine_info *machine )
{
  int i;

  machine->machine = LIBSPECTRUM_MACHINE_PENT1024;
  machine->id = "pentagon1024";

  machine->reset = pentagon1024_reset;

  machine->timex = 0;
  machine->ram.port_from_ula  = pentagon_port_from_ula;
  machine->ram.contend_delay  = spectrum_contend_delay_none;
  machine->ram.contend_delay_no_mreq = spectrum_contend_delay_none;

  machine->unattached_port = spectrum_unattached_port_none;

  machine->shutdown = NULL;

  machine->memory_map = pentagon1024_memory_map;

  for( i = 0; i < 2; i++ ) beta_memory_map_romcs[i].bank = MEMORY_BANK_ROMCS;

  return 0;
}

static int
pentagon1024_reset(void)
{
  int error;
  int i;

  error = machine_load_rom( 0, 0, settings_current.rom_pentagon1024_0,
                            settings_default.rom_pentagon1024_0, 0x4000 );
  if( error ) return error;
  error = machine_load_rom( 2, 1, settings_current.rom_pentagon1024_1,
                            settings_default.rom_pentagon1024_1, 0x4000 );
  if( error ) return error;
  error = machine_load_rom( 4, 2, settings_current.rom_pentagon1024_3,
                            settings_default.rom_pentagon1024_3, 0x4000 );
  if( error ) return error;
  error = machine_load_rom_bank( beta_memory_map_romcs, 0, 0,
                                 settings_current.rom_pentagon1024_2,
                                 settings_default.rom_pentagon1024_2, 0x4000 );
  if( error ) return error;

  error = spec128_common_reset( 0 );
  if( error ) return error;

  machine_current->ram.last_byte2 = 0;
  machine_current->ram.special = 0;

  /* Mark the last 896K as present/writeable */
  for( i = 16; i < 128; i++ )
    memory_map_ram[i].writable = 1;

  beta_builtin = 1;
  beta_active = 1;

  error = periph_setup( peripherals, peripherals_count );
  if( error ) return error;

  machines_periph_pentagon();
  periph_update();

  spec48_common_display_setup();

  return 0;
}

static void
pentagon1024_memoryport_write( libspectrum_word port GCC_UNUSED,
			       libspectrum_byte b )
{
  if( machine_current->ram.locked ) return;

  machine_current->ram.last_byte = b;
  machine_current->memory_map();

  if( machine_current->ram.last_byte2 & 0x04 ) /* v2.2 */
    machine_current->ram.locked = b & 0x20;
}

static void
pentagon1024_v22_memoryport_write( libspectrum_word port GCC_UNUSED,
				   libspectrum_byte b)
{
  if( machine_current->ram.locked ) return;

  machine_current->ram.last_byte2 = b;
  if( b & 0x01 ) {
    display_dirty = display_dirty_pentagon_16_col;
    display_write_if_dirty = display_write_if_dirty_pentagon_16_col;
    display_dirty_flashing = display_dirty_flashing_pentagon_16_col;
    memory_display_dirty = memory_display_dirty_pentagon_16_col;
  } else {
    spec48_common_display_setup();
  }
  machine_current->memory_map();
}

static int pentagon1024_memory_map( void )
{
  int rom, page, screen;
  size_t i;

  screen = ( machine_current->ram.last_byte & 0x08 ) ? 7 : 5;
  if( memory_current_screen != screen ) {
    display_update_critical( 0, 0 );
    display_refresh_main_screen();
    memory_current_screen = screen;
  }

  if( beta_active && !( machine_current->ram.last_byte & 0x10 ) ) {
    rom = 2;
  } else {
    rom = ( machine_current->ram.last_byte & 0x10 ) >> 4;
  }

  machine_current->ram.current_rom = rom;

  if( machine_current->ram.last_byte2 & 0x08 ) {
    memory_map_home[0] = &memory_map_ram[ 0 ];    /* v2.2 */
    memory_map_home[1] = &memory_map_ram[ 1 ];    /* v2.2 */
    machine_current->ram.special = 1;             /* v2.2 */
  } else {
    spec128_select_rom( rom );
  }

  page = ( machine_current->ram.last_byte & 0x07 );

  if( !( machine_current->ram.last_byte2 & 0x04 ) ) {
    page += ( ( machine_current->ram.last_byte & 0xC0 ) >> 3 ) +
	    ( machine_current->ram.last_byte & 0x20 );
  }

  spec128_select_page( page );
  machine_current->ram.current_page = page;

  for( i = 0; i < 8; i++ )
    memory_map_read[i] = memory_map_write[i] = *memory_map_home[i];

  memory_romcs_map();

  return 0;
}
