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
