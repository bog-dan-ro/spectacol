/* dck.c: dock snapshot (Warajevo .DCK) handling routines
   Copyright (c) 2003-2004 Darren Salt, Fredrick Meunier, Philip Kendall

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

   Philip: philip-fuse@shadowmagic.org.uk

   Darren: linux@youmustbejoking.demon.co.uk
   Fred: fredm@spamcop.net

*/

#include <config.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <libspectrum.h>

#include "dck.h"
#include "machine.h"
#include "memory.h"
#include "settings.h"
#include "scld.h"
#include "ui/ui.h"
#include "utils.h"
#include "debugger/debugger.h"

/* Dock cart inserted? */
int dck_active = 0;

int
dck_insert( const char *filename )
{
  int error;

  if ( !( libspectrum_machine_capabilities( machine_current->machine ) &
	  LIBSPECTRUM_MACHINE_CAPABILITY_TIMEX_DOCK ) ) {
    ui_error( UI_ERROR_ERROR, "This machine does not support the dock" );
    return 1;
  }

  error = settings_set_string( &settings_current.dck_file, filename );
  if( error ) return error;

  machine_reset( 0 );

  return 0;
}

void
dck_eject( void )
{
  if ( !( libspectrum_machine_capabilities( machine_current->machine ) &
	  LIBSPECTRUM_MACHINE_CAPABILITY_TIMEX_DOCK ) ) {
    ui_error( UI_ERROR_ERROR, "This machine does not support the dock" );
    return;
  }

  if( settings_current.dck_file ) free( settings_current.dck_file );
  settings_current.dck_file = NULL;

  dck_active = 0;

  ui_menu_activate( UI_MENU_ITEM_MEDIA_CARTRIDGE_DOCK_EJECT, 0 );

  machine_reset( 0 );
}

int
dck_reset( void )
{
  utils_file file;
  size_t num_block = 0;
  libspectrum_dck *dck;
  int error;

  dck_active = 0;

  if( !settings_current.dck_file ) {
    ui_menu_activate( UI_MENU_ITEM_MEDIA_CARTRIDGE_DOCK_EJECT, 0 );
    return 0;
  }

  dck = libspectrum_dck_alloc();

  error = utils_read_file( settings_current.dck_file, &file );
  if( error ) { libspectrum_dck_free( dck, 0 ); return error; }

  error = libspectrum_dck_read2( dck, file.buffer, file.length,
                                 settings_current.dck_file );
  if( error ) {
    utils_close_file( &file ); libspectrum_dck_free( dck, 0 ); return error;
  }

  if( utils_close_file( &file ) ) {
    libspectrum_dck_free( dck, 0 );
    return 1;
  }

  while( dck->dck[num_block] != NULL ) {
    memory_page **mem;
    int i;

    switch( dck->dck[num_block]->bank ) {
    case LIBSPECTRUM_DCK_BANK_HOME:
      mem = memory_map_home;
      break;
    case LIBSPECTRUM_DCK_BANK_DOCK:
      mem = memory_map_dock;
      break;
    case LIBSPECTRUM_DCK_BANK_EXROM:
      mem = memory_map_exrom;
      break;
    default:
      ui_error( UI_ERROR_INFO, "Sorry, bank ID %i is unsupported",
		dck->dck[num_block]->bank );
      libspectrum_dck_free( dck, 0 );
      return 1;
    }

    for( i = 0; i < 8; i++ ) {

      memory_page *page = mem[i];

      switch( dck->dck[num_block]->access[i] ) {

      case LIBSPECTRUM_DCK_PAGE_NULL:
        break;

      case LIBSPECTRUM_DCK_PAGE_ROM:
        page->page = memory_pool_allocate( MEMORY_PAGE_SIZE );
	memcpy( page->page, dck->dck[num_block]->pages[i],
		MEMORY_PAGE_SIZE );
        page->writable = 0;
        page->save_to_snapshot = 1;
        break;

      case LIBSPECTRUM_DCK_PAGE_RAM_EMPTY:
      case LIBSPECTRUM_DCK_PAGE_RAM:
	/* Because the scr and snapshot code depends on the standard
	   memory map being in the RAM[] array, we just copy RAM
	   blocks from the HOME bank into the appropriate page; in
	   other cases, we allocate ourselves a new page to store the
	   contents in */
        if( !(dck->dck[num_block]->bank == LIBSPECTRUM_DCK_BANK_HOME && i>1) ) {
          page->page = memory_pool_allocate( MEMORY_PAGE_SIZE );
          page->writable = 1;
          page->save_to_snapshot = 1;
        }
	
	memcpy( page->page, dck->dck[num_block]->pages[i],
		MEMORY_PAGE_SIZE );
        break;

      }
    }
    num_block++;
  }

  dck_active = 1;

  /* Make the menu item to eject the cartridge active */
  ui_menu_activate( UI_MENU_ITEM_MEDIA_CARTRIDGE_DOCK_EJECT, 1 );

  return libspectrum_dck_free( dck, 0 );
}
