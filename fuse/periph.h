/* periph.h: code for handling peripherals
   Copyright (c) 2004-2009 Philip Kendall

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

#ifndef FUSE_PERIPH_H
#define FUSE_PERIPH_H

#include <libspectrum.h>

/*
 * General peripheral list handling routines
 */

typedef libspectrum_byte (*periph_port_read_function)( libspectrum_word port,
						       int *attached );
typedef void (*periph_port_write_function)( libspectrum_word port,
					    libspectrum_byte data );

/* Information about a peripheral */
typedef struct periph_t {

  /* This peripheral responds to all port values where
     <port> & mask == value */
  libspectrum_word mask;
  libspectrum_word value;

  periph_port_read_function read;
  periph_port_write_function write;

} periph_t;

int periph_register( const periph_t *peripheral );
int periph_register_n( const periph_t *peripherals_list, size_t n );
int periph_set_active( int id, int active );
void periph_clear( void );

/* Register debugger page/unpage events for a peripheral */
int periph_register_paging_events( const char *type_string, int *page_event,
				   int *unpage_event );

/*
 * The actual routines to read and write a port
 */

libspectrum_byte readport( libspectrum_word port );
libspectrum_byte readport_internal( libspectrum_word port );

void writeport( libspectrum_word port, libspectrum_byte b );
void writeport_internal( libspectrum_word port, libspectrum_byte b );

/*
 * The more Fuse-specific peripheral handling routines
 */

/* For indicating the (optional) presence or absence of a peripheral */

typedef enum periph_present {

  PERIPH_PRESENT_NEVER,		/* Never present */
  PERIPH_PRESENT_OPTIONAL,	/* Optionally present */
  PERIPH_PRESENT_ALWAYS,	/* Always present */

} periph_present;

/* Is the Kempston interface active */
extern int periph_kempston_active;

/* Is the Interface I active */
extern int periph_interface1_active;

/* Is the Interface II active */
extern int periph_interface2_active;

/* Is the +D active */
extern int periph_plusd_active;

/* Is the Beta 128 active */
extern int periph_beta128_active;

/* Is the Opus active */
extern int periph_opus_active;

/* Is the Fuller Box active */
extern int periph_fuller_active;

/* Is the Melodik active */
extern int periph_melodik_active;

int periph_setup( const periph_t *peripherals_list, size_t n );
void periph_setup_kempston( periph_present present );
void periph_setup_interface1( periph_present present );
void periph_setup_interface2( periph_present present );
void periph_setup_plusd( periph_present present );
void periph_setup_beta128( periph_present present );
void periph_setup_opus( periph_present present );
void periph_setup_fuller( periph_present present );
void periph_setup_melodik( periph_present present );
void periph_update( void );

void periph_register_beta128( void );

#endif				/* #ifndef FUSE_PERIPH_H */
