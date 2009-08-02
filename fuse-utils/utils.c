/* utils.c: useful utility functions
   Copyright (c) 2002-2007 Philip Kendall

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

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#ifndef WIN32
#include <sys/utsname.h>
#endif

#include <libspectrum.h>

#include "compat.h"
#include "utils.h"

extern char *progname;

/* The minimum version of libspectrum we need */
static const char *LIBSPECTRUM_MIN_VERSION = "0.2.0";

int
init_libspectrum( void )
{
  if( libspectrum_check_version( LIBSPECTRUM_MIN_VERSION ) ) {
    if( libspectrum_init() ) return 1;
  } else {
    fprintf( stderr, "libspectrum version %s found, but %s required",
	     libspectrum_version(), LIBSPECTRUM_MIN_VERSION );
    return 1;
  }

  return 0;
}

int
get_creator( libspectrum_creator **creator, const char *program )
{
  char *custom;
  int version[4] = { 0, 0, 0, 0 };
  libspectrum_error error;
  size_t i;

#ifndef WIN32
  char osname[ 256 ];
  int sys_error;

  sys_error = compat_osname( osname, sizeof( osname ) );
  if( sys_error ) return 1;
#endif

  *creator = libspectrum_creator_alloc();

  error = libspectrum_creator_set_program( *creator, program );
  if( error ) { libspectrum_creator_free( *creator ); return error; }

  sscanf( VERSION, "%u.%u.%u.%u",
	  &version[0], &version[1], &version[2], &version[3] );
  for( i=0; i<4; i++ ) if( version[i] > 0xff ) version[i] = 0xff;

  error = libspectrum_creator_set_major( *creator,
					 version[0] * 0x100 + version[1] );
  if( error ) { libspectrum_creator_free( *creator ); return error; }

  error = libspectrum_creator_set_minor( *creator,
					 version[2] * 0x100 + version[3] );
  if( error ) { libspectrum_creator_free( *creator ); return error; }

  custom = malloc( 256 );
  if( !custom ) {
    fprintf( stderr, "%s: out of memory at %s:%d\n", progname,
	     __FILE__, __LINE__ );
    libspectrum_creator_free( *creator );
    return 1;
  }

#ifdef WIN32
  snprintf( custom, 256, "libspectrum: %s\nsystem: windows\n",
	    libspectrum_version() );
#else
  snprintf( custom, 256, "libspectrum: %s\nuname: %s\n",
	    libspectrum_version(),
	    osname );
#endif

  error = libspectrum_creator_set_custom( *creator,
					  (libspectrum_byte*)custom,
					  strlen( custom ) );
  if( error ) {
    free( custom ); libspectrum_creator_free( *creator );
    return error;
  }

  return 0;
}

int
read_file( const char *filename, unsigned char **buffer, size_t *length )
{
  int fd; struct stat file_info;
  ssize_t bytes;
  
  if( ( fd = open( filename, O_RDONLY | O_BINARY ) ) == -1 ) {
    fprintf( stderr, "%s: couldn't open `%s': %s\n", progname, filename,
	     strerror( errno ) );
    return 1;
  }

  if( fstat( fd, &file_info) ) {
    fprintf( stderr, "%s: couldn't stat `%s': %s\n", progname, filename,
	     strerror( errno ) );
    close(fd);
    return 1;
  }

  *length = file_info.st_size;

  *buffer = malloc( *length );
  if( !*buffer ) {
    fprintf( stderr, "%s: out of memory allocating %lu bytes\n", progname,
             (unsigned long)*length );
    close(fd);
    return 1;
  }

  bytes = read( fd, *buffer, *length );
  if( bytes == -1 ) {
    fprintf( stderr, "%s: error reading from `%s': %s\n", progname, filename,
             strerror( errno ) );
    free( *buffer );
    close(fd);
    return 1;
  } else if( bytes < *length ) {
    fprintf( stderr, "%s: could read only %lu out of %lu bytes from `%s'\n",
             progname, (unsigned long)bytes, (unsigned long)*length, filename );
    free( *buffer );
    close(fd);
    return 1;
  }
    
  if( close(fd) ) {
    fprintf( stderr, "%s: couldn't close `%s': %s\n", progname, filename,
	     strerror( errno ) );
    free( *buffer );
    return 1;
  }
  
  return 0;
}
