/* unittests.c: unit testing framework for Fuse
   Copyright (c) 2008 Philip Kendall

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

#include "fuse.h"
#include "machine.h"
#include "machines/spec128.h"
#include "mempool.h"
#include "peripherals/ula.h"
#include "settings.h"
#include "unittests.h"

static int
contention_test( void )
{
  libspectrum_dword i, checksum = 0, target;
  int error = 0;

  for( i = 0; i < ULA_CONTENTION_SIZE; i++ ) {
    /* Naive, but it will do for now */
    checksum += ula_contention[ i ] * ( i + 1 );
  }

  if( settings_current.late_timings ) {
    switch( machine_current->machine ) {
    case LIBSPECTRUM_MACHINE_16:
    case LIBSPECTRUM_MACHINE_48:
    case LIBSPECTRUM_MACHINE_SE:
      target = 2308927488UL;
      break;
    case LIBSPECTRUM_MACHINE_128:
    case LIBSPECTRUM_MACHINE_PLUS2:
      target = 2335248384UL;
      break;
    case LIBSPECTRUM_MACHINE_PLUS2A:
    case LIBSPECTRUM_MACHINE_PLUS3:
    case LIBSPECTRUM_MACHINE_PLUS3E:
      target = 3113840640UL;
      break;
    case LIBSPECTRUM_MACHINE_TC2048:
    case LIBSPECTRUM_MACHINE_TC2068:
      target = 2307959808UL;
      break;
    case LIBSPECTRUM_MACHINE_TS2068:
      target = 1976561664UL;
      break;
    case LIBSPECTRUM_MACHINE_PENT:
    case LIBSPECTRUM_MACHINE_PENT512:
    case LIBSPECTRUM_MACHINE_PENT1024:
    case LIBSPECTRUM_MACHINE_SCORP:
      target = 0;
      break;
    default:
      target = -1;
      break;
    }
  } else {
    switch( machine_current->machine ) {
    case LIBSPECTRUM_MACHINE_16:
    case LIBSPECTRUM_MACHINE_48:
    case LIBSPECTRUM_MACHINE_SE:
      target = 2308862976UL;
      break;
    case LIBSPECTRUM_MACHINE_128:
    case LIBSPECTRUM_MACHINE_PLUS2:
      target = 2335183872UL;
      break;
    case LIBSPECTRUM_MACHINE_PLUS2A:
    case LIBSPECTRUM_MACHINE_PLUS3:
    case LIBSPECTRUM_MACHINE_PLUS3E:
      target = 3113754624UL;
      break;
    case LIBSPECTRUM_MACHINE_TC2048:
    case LIBSPECTRUM_MACHINE_TC2068:
      target = 2307895296UL;
      break;
    case LIBSPECTRUM_MACHINE_TS2068:
      target = 1976497152UL;
      break;
    case LIBSPECTRUM_MACHINE_PENT:
    case LIBSPECTRUM_MACHINE_PENT512:
    case LIBSPECTRUM_MACHINE_PENT1024:
    case LIBSPECTRUM_MACHINE_SCORP:
      target = 0;
      break;
    default:
      target = -1;
      break;
    }
  }

  if( checksum != target ) {
    fprintf( stderr, "%s: contention test: checksum = %u, expected = %u\n", fuse_progname, checksum, target );
    error = 1;
  }

  return error;
}

static int
floating_bus_test( void )
{
  libspectrum_dword checksum = 0, target;
  libspectrum_word offset;
  int error = 0;

  for( offset = 0; offset < 8192; offset++ )
    RAM[ memory_current_screen ][ offset ] = offset % 0x100;

  for( tstates = 0; tstates < ULA_CONTENTION_SIZE; tstates++ )
    checksum += machine_current->unattached_port() * ( tstates + 1 );

  if( settings_current.late_timings ) {
    switch( machine_current->machine ) {
    case LIBSPECTRUM_MACHINE_16:
    case LIBSPECTRUM_MACHINE_48:
      target = 3426156480UL;
      break;
    case LIBSPECTRUM_MACHINE_128:
    case LIBSPECTRUM_MACHINE_PLUS2:
      target = 2852995008UL;
      break;
    case LIBSPECTRUM_MACHINE_PLUS2A:
    case LIBSPECTRUM_MACHINE_PLUS3:
    case LIBSPECTRUM_MACHINE_PLUS3E:
    case LIBSPECTRUM_MACHINE_TC2048:
    case LIBSPECTRUM_MACHINE_TC2068:
    case LIBSPECTRUM_MACHINE_TS2068:
    case LIBSPECTRUM_MACHINE_SE:
    case LIBSPECTRUM_MACHINE_PENT:
    case LIBSPECTRUM_MACHINE_PENT512:
    case LIBSPECTRUM_MACHINE_PENT1024:
    case LIBSPECTRUM_MACHINE_SCORP:
      target = 4261381056UL;
      break;
    default:
      target = -1;
      break;
    }
  } else {
    switch( machine_current->machine ) {
    case LIBSPECTRUM_MACHINE_16:
    case LIBSPECTRUM_MACHINE_48:
      target = 3427723200UL;
      break;
    case LIBSPECTRUM_MACHINE_128:
    case LIBSPECTRUM_MACHINE_PLUS2:
      target = 2854561728UL;
      break;
    case LIBSPECTRUM_MACHINE_PLUS2A:
    case LIBSPECTRUM_MACHINE_PLUS3:
    case LIBSPECTRUM_MACHINE_PLUS3E:
    case LIBSPECTRUM_MACHINE_TC2048:
    case LIBSPECTRUM_MACHINE_TC2068:
    case LIBSPECTRUM_MACHINE_TS2068:
    case LIBSPECTRUM_MACHINE_SE:
    case LIBSPECTRUM_MACHINE_PENT:
    case LIBSPECTRUM_MACHINE_PENT512:
    case LIBSPECTRUM_MACHINE_PENT1024:
    case LIBSPECTRUM_MACHINE_SCORP:
      target = 4261381056UL;
      break;
    default:
      target = -1;
      break;
    }
  }

  if( checksum != target ) {
    fprintf( stderr, "%s: floating bus test: checksum = %u, expected = %u\n", fuse_progname, checksum, target );
    error = 1;
  }

  return error;
}

#define TEST_ASSERT(x) do { if( !(x) ) { printf("Test assertion failed at %s:%d: %s\n", __FILE__, __LINE__, #x ); return 1; } } while( 0 )

static int
mempool_test( void )
{
  int pool1, pool2;
  int initial_pools = mempool_get_pools();

  pool1 = mempool_register_pool();

  TEST_ASSERT( mempool_get_pools() == initial_pools + 1 );
  TEST_ASSERT( mempool_get_pool_size( pool1 ) == 0 );

  mempool_alloc( pool1, 23 );

  TEST_ASSERT( mempool_get_pool_size( pool1 ) == 1 );

  mempool_alloc( pool1, 42 );

  TEST_ASSERT( mempool_get_pool_size( pool1 ) == 2 );

  mempool_free( pool1 );

  TEST_ASSERT( mempool_get_pool_size( pool1 ) == 0 );

  pool2 = mempool_register_pool();

  TEST_ASSERT( mempool_get_pools() == initial_pools + 2 );
  TEST_ASSERT( mempool_get_pool_size( pool2 ) == 0 );

  mempool_alloc( pool1, 23 );

  TEST_ASSERT( mempool_get_pool_size( pool2 ) == 0 );

  mempool_alloc( pool2, 42 );
  
  TEST_ASSERT( mempool_get_pool_size( pool2 ) == 1 );

  mempool_free( pool2 );

  TEST_ASSERT( mempool_get_pool_size( pool1 ) == 1 );
  TEST_ASSERT( mempool_get_pool_size( pool2 ) == 0 );
  
  mempool_free( pool1 );

  TEST_ASSERT( mempool_get_pool_size( pool1 ) == 0 );
  TEST_ASSERT( mempool_get_pool_size( pool2 ) == 0 );

  return 0;
}

static int
assert_16k_page( libspectrum_word base, int source, int page )
{
  int base_index = base / MEMORY_PAGE_SIZE;
  int i;

  for( i = 0; i < 0x4000 / MEMORY_PAGE_SIZE; i++ ) {
    TEST_ASSERT( memory_map_read[ base_index + i ].source == source );
    TEST_ASSERT( memory_map_read[ base_index + i ].page_num == page );
    TEST_ASSERT( memory_map_write[ base_index + i ].source == source );
    TEST_ASSERT( memory_map_write[ base_index + i ].page_num == page );
  }

  return 0;
}

static int
assert_16k_rom_page( libspectrum_word base, int page )
{
  return assert_16k_page( base, memory_source_rom, page );
}

static int
assert_16k_ram_page( libspectrum_word base, int page )
{
  return assert_16k_page( base, memory_source_ram, page );
}

static int
assert_16k_pages( int rom, int ram4000, int ram8000, int ramc000 )
{
  int r = 0;

  r += assert_16k_rom_page( 0x0000, rom );
  r += assert_16k_ram_page( 0x4000, ram4000 );
  r += assert_16k_ram_page( 0x8000, ram8000 );
  r += assert_16k_ram_page( 0xc000, ramc000 );

  return r;
}

static int
paging_test( void )
{
  int r = 0;

  if( machine_current->machine != LIBSPECTRUM_MACHINE_128 )
    return 0;

  TEST_ASSERT( machine_current->ram.locked == 0 );

  r += assert_16k_pages( 0, 5, 2, 0 );
  TEST_ASSERT( memory_current_screen == 5 );

  spec128_memoryport_write( 0x7ffd, 0x07 );
  r += assert_16k_pages( 0, 5, 2, 7 );
  TEST_ASSERT( memory_current_screen == 5 );

  spec128_memoryport_write( 0x7ffd, 0x08 );
  r += assert_16k_pages( 0, 5, 2, 0 );
  TEST_ASSERT( memory_current_screen == 7 );

  spec128_memoryport_write( 0x7ffd, 0x10 );
  r += assert_16k_pages( 1, 5, 2, 0 );
  TEST_ASSERT( memory_current_screen == 5 );

  spec128_memoryport_write( 0x7ffd, 0x1f );
  r += assert_16k_pages( 1, 5, 2, 7 );
  TEST_ASSERT( memory_current_screen == 7 );

  spec128_memoryport_write( 0x7ffd, 0x20 );
  r += assert_16k_pages( 0, 5, 2, 0 );
  TEST_ASSERT( memory_current_screen == 5 );

  TEST_ASSERT( machine_current->ram.locked != 0 );

  spec128_memoryport_write( 0x7ffd, 0x1f );
  r += assert_16k_pages( 0, 5, 2, 0 );
  TEST_ASSERT( memory_current_screen == 5 );

  return 0;
}

int
unittests_run( void )
{
  int r = 0;

  r += contention_test();
  r += floating_bus_test();
  r += mempool_test();
  r += paging_test();

  return r;
}
