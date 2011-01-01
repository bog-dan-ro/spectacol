/* speccyboot.c: SpeccyBoot Ethernet emulation
   See http://speccyboot.sourceforge.net/
   
   Copyright (c) 2009-2010 Patrik Persson, Philip Kendall
   
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

#include "config.h"

#include "compat.h"
#include "machine.h"
#include "memory.h"
#include "nic/enc28j60.h"
#include "module.h"
#include "settings.h"

#include "speccyboot.h"

/* Determine whether a bit has gone from high to low (or low to high) */
#define GONE_LO(prev, curr, mask)     (((prev) & (mask)) && !((curr) & (mask)))
#define GONE_HI(prev, curr, mask)     (((curr) & (mask)) && !((prev) & (mask)))

static nic_enc28j60_t *nic;

/* ---------------------------------------------------------------------------
 * Spectrum I/O register state (IN/OUT from/to 0x9f)
 * ------------------------------------------------------------------------ */

/* Individual bits when writing the register (OUT instructions) */
#define OUT_BIT_SPI_SCK                 (0x01)
#define OUT_BIT_ETH_CS                  (0x08)
#define OUT_BIT_ROM_CS                  (0x20)
#define OUT_BIT_ETH_RST                 (0x40)
#define OUT_BIT_SPI_MOSI                (0x80)

static libspectrum_byte out_register_state;
static libspectrum_byte in_register_state;

static libspectrum_byte
speccyboot_register_read( libspectrum_word port GCC_UNUSED, int *attached );

static void
speccyboot_register_write( libspectrum_word port GCC_UNUSED, libspectrum_byte val );

const periph_t speccyboot_peripherals[] = {
  { 0x00e0, 0x0080, speccyboot_register_read, speccyboot_register_write }
};

const size_t speccyboot_peripherals_count =
  sizeof ( speccyboot_peripherals ) / sizeof( periph_t );

/* ---------------------------------------------------------------------------
 * ROM paging state
 * ------------------------------------------------------------------------ */

static int speccyboot_rom_active = 0;  /* SpeccyBoot ROM paged in? */
static memory_page speccyboot_memory_map_romcs;

static void
speccyboot_memory_map( void )
{
  if ( !speccyboot_rom_active ) return;

  memory_map_read[0] = memory_map_write[0] = speccyboot_memory_map_romcs;
}

static void
speccyboot_reset( int hard_reset GCC_UNUSED )
{
  static int tap_opened = 0;

  if ( machine_load_rom_bank( &speccyboot_memory_map_romcs, 0, 0,
                              settings_current.rom_speccyboot,
                              settings_default.rom_speccyboot, 0x2000 ) )
    return;

  speccyboot_memory_map_romcs.source = MEMORY_SOURCE_PERIPHERAL;
  speccyboot_memory_map_romcs.writable = 0;

  out_register_state = 0xff;  /* force transitions to low */
  speccyboot_register_write( 0 /* unused argument */, 0 );

  /*
   * Open TAP. If this fails, SpeccyBoot emulation won't work.
   *
   * This is done here rather than in speccyboot_init() to ensure any
   * error messages are only displayed if SpeccyBoot emulation is
   * actually requested.
   */
  if( periph_speccyboot_active && !tap_opened ) {
    nic_enc28j60_init( nic );
    tap_opened = 1;
  }
}

static libspectrum_byte
speccyboot_register_read( libspectrum_word port GCC_UNUSED, int *attached )
{
  if( !periph_speccyboot_active ) return 0xff;

  *attached = 1;
  return in_register_state;
}

static void
speccyboot_register_write( libspectrum_word port GCC_UNUSED, libspectrum_byte val )
{
  if ( !periph_speccyboot_active ) return;

  nic_enc28j60_poll( nic );

  if ( GONE_LO( out_register_state, val, OUT_BIT_ETH_RST ) )
    nic_enc28j60_reset( nic );

  if ( !(val & OUT_BIT_ETH_CS) ) {

    if ( GONE_LO( out_register_state, val, OUT_BIT_ETH_CS ) )
      nic_enc28j60_set_spi_state( nic, SPI_CMD );

    /*
     * NOTE: the ENC28J60 data sheet (figure 4-2) specifies that MISO
     * is updated when SCK goes low, but we instead set it on the
     * subsequent transition to high. This is to simplify the internal
     * state logic for SPI RBM commands.
     *
     * In this design, we ignore the final SCK transition to low at
     * the end of a transaction. That SCK transition, if occurring at
     * the end of a RBM transaction, would otherwise trigger another
     * read operation, and ERDPT would be increased one time too many.
     *
     * The SpeccyBoot stack reads MISO just after setting SCK high, so
     * it works fine with this simplification.
     */
    if ( GONE_HI( out_register_state, val, OUT_BIT_SPI_SCK ) ) {
      in_register_state = 0xfe | nic_enc28j60_spi_produce_bit( nic );
      nic_enc28j60_spi_consume_bit( nic, (out_register_state & OUT_BIT_SPI_MOSI) ? 1 : 0 );
    }
  }
   
  /* Update ROM paging status when the ROM_CS bit is cleared or set */
  if ( GONE_LO( out_register_state, val, OUT_BIT_ROM_CS ) ) {
    speccyboot_rom_active = 1;
    machine_current->ram.romcs = 1;
    machine_current->memory_map();
  } else if ( GONE_HI( out_register_state, val, OUT_BIT_ROM_CS ) ) {
    speccyboot_rom_active = 0;
    machine_current->ram.romcs = 0;
    machine_current->memory_map();
  }

  out_register_state = val;
}

int
speccyboot_init( void )
{
  nic = nic_enc28j60_alloc();

  static module_info_t speccyboot_module_info = {
    speccyboot_reset,
    speccyboot_memory_map,
    NULL,
    NULL,
    NULL
  };

  module_register( &speccyboot_module_info );

  speccyboot_memory_map_romcs.bank = MEMORY_BANK_ROMCS;

  return 0;
}
