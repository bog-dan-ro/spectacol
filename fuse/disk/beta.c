/* beta.c: Routines for handling the Beta disk interface
   Copyright (c) 2004-2008 Stuart Brady

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

   Philip: pak21-fuse@srcf.ucam.org
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

   Stuart: sdbrady@ntlworld.com

*/

#include <config.h>

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#ifdef HAVE_STRINGS_H
#include <strings.h>            /* Needed for strncasecmp() on QNX6 */
#endif                          /* #ifdef HAVE_STRINGS_H */
#include <limits.h>
#include <sys/stat.h>

#include <libspectrum.h>

#include "beta.h"
#include "compat.h"
#include "event.h"
#include "machine.h"
#include "module.h"
#include "settings.h"
#include "ui/ui.h"
#include "utils.h"
#include "wd_fdc.h"
#include "z80/z80.h"
#include "z80/z80_macros.h"
#include "options.h"	/* needed for get combo options */

#define DISK_TRY_MERGE(heads) ( option_enumerate_diskoptions_disk_try_merge() == 2 || \
				( option_enumerate_diskoptions_disk_try_merge() == 1 && heads == 1 ) )

int beta_available = 0;
int beta_active = 0;
int beta_builtin = 0;

libspectrum_word beta_pc_mask;
libspectrum_word beta_pc_value;

static int beta_index_pulse = 0;

static int index_event;

#define BETA_NUM_DRIVES 4

static wd_fdc *beta_fdc;
static wd_fdc_drive beta_drives[ BETA_NUM_DRIVES ];

const periph_t beta_peripherals[] = {
  { 0x00ff, 0x001f, beta_sr_read, beta_cr_write },
  { 0x00ff, 0x003f, beta_tr_read, beta_tr_write },
  { 0x00ff, 0x005f, beta_sec_read, beta_sec_write },
  { 0x00ff, 0x007f, beta_dr_read, beta_dr_write },
  { 0x00ff, 0x00ff, beta_sp_read, beta_sp_write },
};

const size_t beta_peripherals_count =
  sizeof( beta_peripherals ) / sizeof( periph_t );

static void beta_reset( int hard_reset );
static void beta_memory_map( void );
static void beta_enabled_snapshot( libspectrum_snap *snap );
static void beta_from_snapshot( libspectrum_snap *snap );
static void beta_to_snapshot( libspectrum_snap *snap );
static void beta_event_index( libspectrum_dword last_tstates, int type,
			      void *user_data );

static module_info_t beta_module_info = {

  beta_reset,
  beta_memory_map,
  beta_enabled_snapshot,
  beta_from_snapshot,
  beta_to_snapshot,

};

void
beta_page( void )
{
  beta_active = 1;
  machine_current->ram.romcs = 1;
  machine_current->memory_map();
}

void
beta_unpage( void )
{
  beta_active = 0;
  machine_current->ram.romcs = 0;
  machine_current->memory_map();
}

static void
beta_memory_map( void )
{
  if( !beta_active ) return;

  memory_map_read[0] = memory_map_write[0] = memory_map_romcs[ 0 ];
  memory_map_read[1] = memory_map_write[1] = memory_map_romcs[ 1 ];
}

static void
beta_select_drive( int i )
{
  if( beta_fdc->current_drive != &beta_drives[ i & 0x03 ] ) {
    if( beta_fdc->current_drive != NULL )
      fdd_select( &beta_fdc->current_drive->fdd, 0 );
    beta_fdc->current_drive = &beta_drives[ i & 0x03 ];
    fdd_select( &beta_fdc->current_drive->fdd, 1 );
  }
}

int
beta_init( void )
{
  int i;
  wd_fdc_drive *d;

  beta_fdc = wd_fdc_alloc_fdc( FD1793, 0, WD_FLAG_BETA128 );
  beta_fdc->current_drive = NULL;

  for( i = 0; i < BETA_NUM_DRIVES; i++ ) {
    d = &beta_drives[ i ];
    fdd_init( &d->fdd, FDD_SHUGART, 0, 0, 0 );	/* drive geometry 'autodetect' */
    d->disk.flag = DISK_FLAG_NONE;
  }
  beta_select_drive( 0 );

  beta_fdc->dden = 1;
  beta_fdc->set_intrq = NULL;
  beta_fdc->reset_intrq = NULL;
  beta_fdc->set_datarq = NULL;
  beta_fdc->reset_datarq = NULL;

  index_event = event_register( beta_event_index, "Beta disk index" );
  if( index_event == -1 ) return 1;

  module_register( &beta_module_info );

  return 0;
}

static void
beta_reset( int hard_reset GCC_UNUSED )
{
  int i;
  wd_fdc_drive *d;
  const fdd_params_t *dt;

  event_remove_type( index_event );

  if( !periph_beta128_active ) {
    beta_active = 0;
    beta_available = 0;
    return;
  }

  beta_available = 1;

  beta_pc_mask = 0xff00;
  beta_pc_value = 0x3d00;
  
  wd_fdc_master_reset( beta_fdc );

  for( i = 0; i < BETA_NUM_DRIVES; i++ ) {
    d = &beta_drives[ i ];

    d->index_pulse = 0;
    d->index_interrupt = 0;
  }

  if( !beta_builtin ) {
    machine_load_rom_bank( memory_map_romcs, 0, 0,
			   settings_current.rom_beta128,
			   settings_default.rom_beta128, 0x4000 );

    memory_map_romcs[ 0 ].writable = 0;
    memory_map_romcs[ 1 ].writable = 0;

    memory_map_romcs[0].source = MEMORY_SOURCE_PERIPHERAL;
    memory_map_romcs[1].source = MEMORY_SOURCE_PERIPHERAL;

    beta_active = 0;

    if( !( machine_current->capabilities &
           LIBSPECTRUM_MACHINE_CAPABILITY_128_MEMORY ) ) {
      beta_pc_mask = 0xfe00;
      beta_pc_value = 0x3c00;

      /* For 48K type machines, the Beta 128 is supposed to be configured
         to start with the Beta ROM paged in (System switch in centre position)
         */
      beta_page();
    }
  }

  /* We can eject disks only if they are currently present */
  dt = &fdd_params[ option_enumerate_diskoptions_drive_beta128a_type() + 1 ];	/* +1 => there is no `Disabled' */
  fdd_init( &beta_drives[ BETA_DRIVE_A ].fdd, FDD_SHUGART,
	    dt->heads, dt->cylinders, 1 );
  ui_menu_activate( UI_MENU_ITEM_MEDIA_DISK_BETA_A, dt->enabled );
  ui_menu_activate( UI_MENU_ITEM_MEDIA_DISK_BETA_A_EJECT,
		    beta_drives[ BETA_DRIVE_A ].fdd.loaded );
  ui_menu_activate( UI_MENU_ITEM_MEDIA_DISK_BETA_A_FLIP_SET,
		    !beta_drives[ BETA_DRIVE_A ].fdd.upsidedown );
  ui_menu_activate( UI_MENU_ITEM_MEDIA_DISK_BETA_A_WP_SET,
		    !beta_drives[ BETA_DRIVE_A ].fdd.wrprot );


  dt = &fdd_params[ option_enumerate_diskoptions_drive_beta128b_type() ];
  fdd_init( &beta_drives[ BETA_DRIVE_B ].fdd, dt->enabled ? FDD_SHUGART : FDD_TYPE_NONE,
	    dt->heads, dt->cylinders, 1 );
  ui_menu_activate( UI_MENU_ITEM_MEDIA_DISK_BETA_B, dt->enabled );
  ui_menu_activate( UI_MENU_ITEM_MEDIA_DISK_BETA_B_FLIP_SET,
		    !beta_drives[ BETA_DRIVE_B ].fdd.upsidedown );
  ui_menu_activate( UI_MENU_ITEM_MEDIA_DISK_BETA_B_EJECT,
		    beta_drives[ BETA_DRIVE_B ].fdd.loaded );
  ui_menu_activate( UI_MENU_ITEM_MEDIA_DISK_BETA_B_WP_SET,
		    !beta_drives[ BETA_DRIVE_B ].fdd.wrprot );


  dt = &fdd_params[ option_enumerate_diskoptions_drive_beta128c_type() ];
  fdd_init( &beta_drives[ BETA_DRIVE_C ].fdd, dt->enabled ? FDD_SHUGART : FDD_TYPE_NONE,
	    dt->heads, dt->cylinders, 1 );
  ui_menu_activate( UI_MENU_ITEM_MEDIA_DISK_BETA_C, dt->enabled );
  ui_menu_activate( UI_MENU_ITEM_MEDIA_DISK_BETA_C_FLIP_SET,
		    !beta_drives[ BETA_DRIVE_C ].fdd.upsidedown );
  ui_menu_activate( UI_MENU_ITEM_MEDIA_DISK_BETA_C_EJECT,
		    beta_drives[ BETA_DRIVE_C ].fdd.loaded );
  ui_menu_activate( UI_MENU_ITEM_MEDIA_DISK_BETA_C_WP_SET,
		    !beta_drives[ BETA_DRIVE_C ].fdd.wrprot );


  dt = &fdd_params[ option_enumerate_diskoptions_drive_beta128d_type() ];
  fdd_init( &beta_drives[ BETA_DRIVE_D ].fdd, dt->enabled ? FDD_SHUGART : FDD_TYPE_NONE,
	    dt->heads, dt->cylinders, 1 );
  ui_menu_activate( UI_MENU_ITEM_MEDIA_DISK_BETA_D, dt->enabled );
  ui_menu_activate( UI_MENU_ITEM_MEDIA_DISK_BETA_D_FLIP_SET,
		    !beta_drives[ BETA_DRIVE_D ].fdd.upsidedown );
  ui_menu_activate( UI_MENU_ITEM_MEDIA_DISK_BETA_D_EJECT,
		    beta_drives[ BETA_DRIVE_D ].fdd.loaded );
  ui_menu_activate( UI_MENU_ITEM_MEDIA_DISK_BETA_D_WP_SET,
		    !beta_drives[ BETA_DRIVE_D ].fdd.wrprot );


  beta_select_drive( 0 );
  machine_current->memory_map();
  beta_event_index( 0, 0, NULL );

  ui_statusbar_update( UI_STATUSBAR_ITEM_DISK, UI_STATUSBAR_STATE_INACTIVE );
}

void
beta_end( void )
{
  beta_available = 0;
}

libspectrum_byte
beta_sr_read( libspectrum_word port GCC_UNUSED, int *attached )
{
  if( !beta_active ) return 0xff;

  *attached = 1;
  return wd_fdc_sr_read( beta_fdc );
}

void
beta_cr_write( libspectrum_word port GCC_UNUSED, libspectrum_byte b )
{
  if( !beta_active ) return;

  wd_fdc_cr_write( beta_fdc, b );
}

libspectrum_byte
beta_tr_read( libspectrum_word port GCC_UNUSED, int *attached )
{
  if( !beta_active ) return 0xff;

  *attached = 1;
  return wd_fdc_tr_read( beta_fdc );
}

void
beta_tr_write( libspectrum_word port GCC_UNUSED, libspectrum_byte b )
{
  if( !beta_active ) return;

  wd_fdc_tr_write( beta_fdc, b );
}

libspectrum_byte
beta_sec_read( libspectrum_word port GCC_UNUSED, int *attached )
{
  if( !beta_active ) return 0xff;

  *attached = 1;
  return wd_fdc_sec_read( beta_fdc );
}

void
beta_sec_write( libspectrum_word port GCC_UNUSED, libspectrum_byte b )
{
  if( !beta_active ) return;

  wd_fdc_sec_write( beta_fdc, b );
}

libspectrum_byte
beta_dr_read( libspectrum_word port GCC_UNUSED, int *attached )
{
  if( !beta_active ) return 0xff;

  *attached = 1;
  return wd_fdc_dr_read( beta_fdc );
}

void
beta_dr_write( libspectrum_word port GCC_UNUSED, libspectrum_byte b )
{
  if( !beta_active ) return;

  wd_fdc_dr_write( beta_fdc, b );
}

void
beta_sp_write( libspectrum_word port GCC_UNUSED, libspectrum_byte b )
{
  if( !beta_active ) return;
  
  /* reset 0x04 and then set it to reset controller */
  beta_select_drive( b & 0x03 );
  /* 0x08 = block hlt, normally set */
  wd_fdc_set_hlt( beta_fdc, ( ( b & 0x08 ) ? 1 : 0 ) );
  fdd_set_head( &beta_fdc->current_drive->fdd, ( ( b & 0x10 ) ? 0 : 1 ) );
  /* 0x20 = density, reset = FM, set = MFM */
  beta_fdc->dden = b & 0x20 ? 1 : 0;
}

libspectrum_byte
beta_sp_read( libspectrum_word port GCC_UNUSED, int *attached )
{
  libspectrum_byte b;

  if( !beta_active ) return 0xff;

  *attached = 1;
  b = 0;

  if( beta_fdc->intrq )
    b |= 0x80;

  if( beta_fdc->datarq )
    b |= 0x40;

/* we should reset beta_datarq, but we first need to raise it for each byte
 * transferred in wd_fdc.c */

  return b;
}

int
beta_disk_insert( beta_drive_number which, const char *filename,
		      int autoload )
{
  int error;
  wd_fdc_drive *d;
  const fdd_params_t *dt;

  if( which >= BETA_NUM_DRIVES ) {
    ui_error( UI_ERROR_ERROR, "beta_disk_insert: unknown drive %d",
	      which );
    fuse_abort();
  }

  d = &beta_drives[ which ];

  /* Eject any disk already in the drive */
  if( d->fdd.loaded ) {
    /* Abort the insert if we want to keep the current disk */
    if( beta_disk_eject( which, 0 ) ) return 0;
  }

  if( filename ) {
    error = disk_open( &d->disk, filename, 0, DISK_TRY_MERGE( d->fdd.fdd_heads ) );
    if( error != DISK_OK ) {
      ui_error( UI_ERROR_ERROR, "Failed to open disk image: %s",
				disk_strerror( d->disk.status ) );
      return 1;
    }
  } else {
    switch( which ) {
    case 0:
      dt = &fdd_params[ option_enumerate_diskoptions_drive_beta128a_type() + 1 ];	/* +1 => there is no `Disabled' */
      break;
    case 1:
      dt = &fdd_params[ option_enumerate_diskoptions_drive_beta128b_type() ];
      break;
    case 2:
      dt = &fdd_params[ option_enumerate_diskoptions_drive_beta128c_type() ];
      break;
    case 3:
    default:
      dt = &fdd_params[ option_enumerate_diskoptions_drive_beta128d_type() ];
      break;
    }
    error = disk_new( &d->disk, dt->heads, dt->cylinders, DISK_DENS_AUTO, DISK_UDI );
    if( error != DISK_OK ) {
      ui_error( UI_ERROR_ERROR, "Failed to create disk image: %s",
				disk_strerror( d->disk.status ) );
      return 1;
    }
  }

  fdd_load( &d->fdd, &d->disk, 0 );

  /* Set the 'eject' item active */
  switch( which ) {
  case BETA_DRIVE_A:
    ui_menu_activate( UI_MENU_ITEM_MEDIA_DISK_BETA_A_EJECT, 1 );
    ui_menu_activate( UI_MENU_ITEM_MEDIA_DISK_BETA_A_FLIP_SET,
		      !beta_drives[ BETA_DRIVE_A ].fdd.upsidedown );
    ui_menu_activate( UI_MENU_ITEM_MEDIA_DISK_BETA_A_WP_SET,
		      !beta_drives[ BETA_DRIVE_A ].fdd.wrprot );
    break;
  case BETA_DRIVE_B:
    ui_menu_activate( UI_MENU_ITEM_MEDIA_DISK_BETA_B_EJECT, 1 );
    ui_menu_activate( UI_MENU_ITEM_MEDIA_DISK_BETA_B_FLIP_SET,
		      !beta_drives[ BETA_DRIVE_B ].fdd.upsidedown );
    ui_menu_activate( UI_MENU_ITEM_MEDIA_DISK_BETA_B_WP_SET,
		      !beta_drives[ BETA_DRIVE_B ].fdd.wrprot );
    break;
  case BETA_DRIVE_C:
    ui_menu_activate( UI_MENU_ITEM_MEDIA_DISK_BETA_C_EJECT, 1 );
    ui_menu_activate( UI_MENU_ITEM_MEDIA_DISK_BETA_C_FLIP_SET,
		      !beta_drives[ BETA_DRIVE_C ].fdd.upsidedown );
    ui_menu_activate( UI_MENU_ITEM_MEDIA_DISK_BETA_C_WP_SET,
		      !beta_drives[ BETA_DRIVE_C ].fdd.wrprot );
    break;
  case BETA_DRIVE_D:
    ui_menu_activate( UI_MENU_ITEM_MEDIA_DISK_BETA_D_EJECT, 1 );
    ui_menu_activate( UI_MENU_ITEM_MEDIA_DISK_BETA_D_FLIP_SET,
		      !beta_drives[ BETA_DRIVE_D ].fdd.upsidedown );
    ui_menu_activate( UI_MENU_ITEM_MEDIA_DISK_BETA_D_WP_SET,
		      !beta_drives[ BETA_DRIVE_D ].fdd.wrprot );
    break;
  }

  if( filename && autoload ) {
    PC = 0;
    machine_current->ram.last_byte |= 0x10;   /* Select ROM 1 */
    beta_page();
  }

  return 0;
}

int
beta_disk_flip( beta_drive_number which, int flip )
{
  wd_fdc_drive *d;

  if( which >= BETA_NUM_DRIVES )
    return 1;

  d = &beta_drives[ which ];

  if( !d->fdd.loaded )
    return 1;

  fdd_flip( &d->fdd, flip );

  /* Set the 'flip' item */
  switch( which ) {
  case BETA_DRIVE_A:
    ui_menu_activate( UI_MENU_ITEM_MEDIA_DISK_BETA_A_FLIP_SET,
		      !beta_drives[ BETA_DRIVE_A ].fdd.upsidedown );
    break;
  case BETA_DRIVE_B:
    ui_menu_activate( UI_MENU_ITEM_MEDIA_DISK_BETA_B_FLIP_SET,
		      !beta_drives[ BETA_DRIVE_B ].fdd.upsidedown );
    break;
  case BETA_DRIVE_C:
    ui_menu_activate( UI_MENU_ITEM_MEDIA_DISK_BETA_C_FLIP_SET,
		      !beta_drives[ BETA_DRIVE_C ].fdd.upsidedown );
    break;
  case BETA_DRIVE_D:
    ui_menu_activate( UI_MENU_ITEM_MEDIA_DISK_BETA_D_FLIP_SET,
		      !beta_drives[ BETA_DRIVE_D ].fdd.upsidedown );
    break;
  }
  return 0;
}

int
beta_disk_writeprotect( beta_drive_number which, int wrprot )
{
  wd_fdc_drive *d;

  if( which >= BETA_NUM_DRIVES )
    return 1;

  d = &beta_drives[ which ];

  if( !d->fdd.loaded )
    return 1;

  fdd_wrprot( &d->fdd, wrprot );

  /* Set the 'writeprotect' item */
  switch( which ) {
  case BETA_DRIVE_A:
    ui_menu_activate( UI_MENU_ITEM_MEDIA_DISK_BETA_A_WP_SET,
		      !beta_drives[ BETA_DRIVE_A ].fdd.wrprot );
    break;
  case BETA_DRIVE_B:
    ui_menu_activate( UI_MENU_ITEM_MEDIA_DISK_BETA_B_WP_SET,
		      !beta_drives[ BETA_DRIVE_B ].fdd.wrprot );
    break;
  case BETA_DRIVE_C:
    ui_menu_activate( UI_MENU_ITEM_MEDIA_DISK_BETA_C_WP_SET,
		      !beta_drives[ BETA_DRIVE_C ].fdd.wrprot );
    break;
  case BETA_DRIVE_D:
    ui_menu_activate( UI_MENU_ITEM_MEDIA_DISK_BETA_D_WP_SET,
		      !beta_drives[ BETA_DRIVE_D ].fdd.wrprot );
    break;
  }
  return 0;
}

int
beta_disk_eject( beta_drive_number which, int write )
{
  wd_fdc_drive *d;
  char drive;
  
  if( which >= BETA_NUM_DRIVES )
    return 1;
    
  d = &beta_drives[ which ];
  
  if( !d->fdd.loaded )
    return 0;
    
  if( write ) {
  
    if( ui_beta_disk_write( which ) ) return 1;

  } else {
  
    if( d->disk.dirty ) {
      ui_confirm_save_t confirm;

      switch( which ) {
	case BETA_DRIVE_A: drive = 'A'; break;
	case BETA_DRIVE_B: drive = 'B'; break;
	case BETA_DRIVE_C: drive = 'C'; break;
	case BETA_DRIVE_D: drive = 'D'; break;
	default: drive = '?'; break;
      }

      confirm = ui_confirm_save(
	"Disk in Beta drive %c: has been modified.\n"
	"Do you want to save it?",
	drive
      );

      switch( confirm ) {

      case UI_CONFIRM_SAVE_SAVE:
	if( ui_beta_disk_write( which ) ) return 1;
	break;

      case UI_CONFIRM_SAVE_DONTSAVE: break;
      case UI_CONFIRM_SAVE_CANCEL: return 1;

      }
    }
  }

  fdd_unload( &d->fdd );
  disk_close( &d->disk );

  /* Set the 'eject' item inactive */
  switch( which ) {
  case BETA_DRIVE_A:
    ui_menu_activate( UI_MENU_ITEM_MEDIA_DISK_BETA_A_EJECT, 0 );
    break;
  case BETA_DRIVE_B:
    ui_menu_activate( UI_MENU_ITEM_MEDIA_DISK_BETA_B_EJECT, 0 );
    break;
  case BETA_DRIVE_C:
    ui_menu_activate( UI_MENU_ITEM_MEDIA_DISK_BETA_C_EJECT, 0 );
    break;
  case BETA_DRIVE_D:
    ui_menu_activate( UI_MENU_ITEM_MEDIA_DISK_BETA_D_EJECT, 0 );
    break;
  }
  return 0;
}

int
beta_disk_write( beta_drive_number which, const char *filename )
{
  wd_fdc_drive *d = &beta_drives[ which ];
  int error;

  d->disk.type = DISK_TYPE_NONE;
  error = disk_write( &d->disk, filename );

  if( error != DISK_OK ) {
    ui_error( UI_ERROR_ERROR, "couldn't write '%s' file: %s", filename,
	      disk_strerror( error ) );
    return 1;
  }

  return 0;
}

fdd_t *
beta_get_fdd( beta_drive_number which )
{
  return &( beta_drives[ which ].fdd );
}

static void
beta_event_index( libspectrum_dword last_tstates, int type, void *user_data )
{
  int error;
  int next_tstates;
  int i;

  beta_index_pulse = !beta_index_pulse;
  for( i = 0; i < BETA_NUM_DRIVES; i++ ) {
    wd_fdc_drive *d = &beta_drives[ i ];

    d->index_pulse = beta_index_pulse;
/* disabled, until we have better timing emulation,
 * to avoid interrupts while reading/writing data */
    if( !beta_index_pulse && d->index_interrupt ) {
      wd_fdc_set_intrq( beta_fdc );
      d->index_interrupt = 0;
    }
  }
  next_tstates = ( beta_index_pulse ? 10 : 190 ) *
    machine_current->timings.processor_speed / 1000;
  error = event_add( last_tstates + next_tstates, index_event );
  if( error ) return;
}

static void
beta_enabled_snapshot( libspectrum_snap *snap )
{
  if( libspectrum_snap_beta_active( snap ) )
    settings_current.beta128 = 1;
}

static void
beta_from_snapshot( libspectrum_snap *snap )
{
  if( !libspectrum_snap_beta_active( snap ) ) return;

  beta_active = libspectrum_snap_beta_paged( snap );

  if( beta_active ) {
    beta_page();
  } else {
    beta_unpage();
  }

  if( libspectrum_snap_beta_custom_rom( snap ) &&
      libspectrum_snap_beta_rom( snap, 0 ) &&
      machine_load_rom_bank_from_buffer(
                             memory_map_romcs, 0, 0,
                             libspectrum_snap_beta_rom( snap, 0 ),
                             MEMORY_PAGE_SIZE * 2,
                             1 ) )
    return;

  /* ignore drive count for now, there will be an issue with loading snaps where
     drives have been disabled
  libspectrum_snap_beta_drive_count( snap )
   */

  beta_fdc->direction = libspectrum_snap_beta_direction( snap );

  beta_cr_write ( 0x001f, 0 );
  beta_tr_write ( 0x003f, libspectrum_snap_beta_track ( snap ) );
  beta_sec_write( 0x005f, libspectrum_snap_beta_sector( snap ) );
  beta_dr_write ( 0x007f, libspectrum_snap_beta_data  ( snap ) );
  beta_sp_write ( 0x00ff, libspectrum_snap_beta_system( snap ) );
}

void
beta_to_snapshot( libspectrum_snap *snap )
{
  int attached;
  wd_fdc *f = beta_fdc;
  libspectrum_byte *buffer;
  int drive_count = 0;

  if( !periph_beta128_active ) return;

  libspectrum_snap_set_beta_active( snap, 1 );

  if( memory_map_romcs[0].source == MEMORY_SOURCE_CUSTOMROM ) {
    size_t rom_length = MEMORY_PAGE_SIZE * 2;

    buffer = malloc( rom_length );
    if( !buffer ) {
      ui_error( UI_ERROR_ERROR, "Out of memory at %s:%d", __FILE__, __LINE__ );
      return;
    }

    memcpy( buffer, memory_map_romcs[0].page, MEMORY_PAGE_SIZE );
    memcpy( buffer + MEMORY_PAGE_SIZE, memory_map_romcs[1].page,
	    MEMORY_PAGE_SIZE );

    libspectrum_snap_set_beta_rom( snap, 0, buffer );
    libspectrum_snap_set_beta_custom_rom( snap, 1 );
  }

  drive_count++; /* Drive A is not removable */
  if( option_enumerate_diskoptions_drive_beta128b_type() > 0 ) drive_count++;
  if( option_enumerate_diskoptions_drive_beta128c_type() > 0 ) drive_count++;
  if( option_enumerate_diskoptions_drive_beta128d_type() > 0 ) drive_count++;
  libspectrum_snap_set_beta_drive_count( snap, drive_count );

  libspectrum_snap_set_beta_paged ( snap, beta_active );
  libspectrum_snap_set_beta_direction( snap, beta_fdc->direction );
  libspectrum_snap_set_beta_status( snap, beta_sr_read( 0x001f, &attached ) );
  libspectrum_snap_set_beta_track ( snap, f->track_register );
  libspectrum_snap_set_beta_sector( snap, f->sector_register );
  libspectrum_snap_set_beta_data  ( snap, f->data_register );
  libspectrum_snap_set_beta_system( snap, beta_sp_read( 0x00ff, &attached ) );
}
