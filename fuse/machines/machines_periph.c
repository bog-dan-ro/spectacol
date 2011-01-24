/* machines_periph.c: various machine-specific peripherals
   Copyright (c) 2011 Philip Kendall

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

#include "periph.h"
#include "spec128.h"
#include "specplus3.h"

static const periph_t spec128_peripherals[] = {
  { 0x8002, 0x0000, NULL, spec128_memoryport_write },
  { 0, 0, NULL, NULL }
};

static const periph_t specplus3_memory_peripherals[] = {
  { 0xc002, 0x4000, NULL, spec128_memoryport_write },
  { 0xf002, 0x1000, NULL, specplus3_memoryport2_write },
  { 0, 0, NULL, NULL }
};

void
machines_periph_init( void )
{
  periph_register_type( PERIPH_TYPE_128_MEMORY, NULL, spec128_peripherals );
  periph_register_type( PERIPH_TYPE_PLUS3_MEMORY, NULL,
                        specplus3_memory_peripherals );
}

/* Peripherals generally available on all machines; the Timex machines and
   Russian clones remove some items from this list */
static void
base_peripherals( void )
{
  periph_set_present( PERIPH_TYPE_DIVIDE, PERIPH_PRESENT_OPTIONAL );
  periph_set_present( PERIPH_TYPE_KEMPSTON, PERIPH_PRESENT_OPTIONAL );
  periph_set_present( PERIPH_TYPE_KEMPSTON_MOUSE, PERIPH_PRESENT_OPTIONAL );
  periph_set_present( PERIPH_TYPE_SIMPLEIDE, PERIPH_PRESENT_OPTIONAL );
  periph_set_present( PERIPH_TYPE_SPECCYBOOT, PERIPH_PRESENT_OPTIONAL );
  periph_set_present( PERIPH_TYPE_ULA, PERIPH_PRESENT_ALWAYS );
  periph_set_present( PERIPH_TYPE_ZXATASP, PERIPH_PRESENT_OPTIONAL );
  periph_set_present( PERIPH_TYPE_ZXCF, PERIPH_PRESENT_OPTIONAL );
}

/* Peripherals available on the 48K and 128K */
void
base_peripherals_48_128( void )
{
  base_peripherals();
  periph_set_present( PERIPH_TYPE_BETA128, PERIPH_PRESENT_OPTIONAL );
  periph_set_present( PERIPH_TYPE_INTERFACE1, PERIPH_PRESENT_OPTIONAL );
  periph_set_present( PERIPH_TYPE_INTERFACE2, PERIPH_PRESENT_OPTIONAL );
  periph_set_present( PERIPH_TYPE_OPUS, PERIPH_PRESENT_OPTIONAL );
  periph_set_present( PERIPH_TYPE_PLUSD, PERIPH_PRESENT_OPTIONAL );
}

/* The set of peripherals available on the 48K and similar machines */
void
machines_periph_48( void )
{
  base_peripherals_48_128();
  periph_set_present( PERIPH_TYPE_FULLER, PERIPH_PRESENT_OPTIONAL );
  periph_set_present( PERIPH_TYPE_MELODIK, PERIPH_PRESENT_OPTIONAL );
  periph_set_present( PERIPH_TYPE_ZXPRINTER, PERIPH_PRESENT_OPTIONAL );
}

/* The set of peripherals available on the 128K and similar machines */
void
machines_periph_128( void )
{
  base_peripherals_48_128();
  periph_set_present( PERIPH_TYPE_AY, PERIPH_PRESENT_ALWAYS );
  periph_set_present( PERIPH_TYPE_128_MEMORY, PERIPH_PRESENT_ALWAYS );
}

/* The set of peripherals available on the +3 and similar machines */
void
machines_periph_plus3( void )
{
  base_peripherals();
  periph_set_present( PERIPH_TYPE_AY, PERIPH_PRESENT_ALWAYS );
  periph_set_present( PERIPH_TYPE_PLUS3_MEMORY, PERIPH_PRESENT_ALWAYS );
}

/* The set of peripherals available on the TC2068 and TS2068 */
void
machines_periph_timex( void )
{
  base_peripherals();

  /* ULA uses full decoding */
  periph_set_present( PERIPH_TYPE_ULA, PERIPH_PRESENT_NEVER );
  periph_set_present( PERIPH_TYPE_ULA_FULL_DECODE, PERIPH_PRESENT_ALWAYS );

  /* SCLD always present */
  periph_set_present( PERIPH_TYPE_SCLD, PERIPH_PRESENT_ALWAYS );

  /* ZX Printer and Interface 2 available */
  periph_set_present( PERIPH_TYPE_INTERFACE2, PERIPH_PRESENT_OPTIONAL );
  periph_set_present( PERIPH_TYPE_ZXPRINTER_FULL_DECODE, PERIPH_PRESENT_OPTIONAL );
}

/* The set of peripherals available on the Pentagon and Scorpion */
void
machines_periph_pentagon( void )
{
  base_peripherals();

  /* 128K-style memory paging available */
  periph_set_present( PERIPH_TYPE_128_MEMORY, PERIPH_PRESENT_ALWAYS );

  /* AY available */
  periph_set_present( PERIPH_TYPE_AY, PERIPH_PRESENT_ALWAYS );

  /* ULA uses full decoding */
  periph_set_present( PERIPH_TYPE_ULA, PERIPH_PRESENT_NEVER );
  periph_set_present( PERIPH_TYPE_ULA_FULL_DECODE, PERIPH_PRESENT_ALWAYS );

  /* Built-in Betadisk 128 interface, which also handles Kempston joystick
     as they share a port */
  periph_set_present( PERIPH_TYPE_BETA128, PERIPH_PRESENT_ALWAYS );
  periph_set_present( PERIPH_TYPE_KEMPSTON, PERIPH_PRESENT_NEVER );
}
