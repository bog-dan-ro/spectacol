/* snapconv.c: Convert snapshot formats
   Copyright (c) 2003-2005 Philip Kendall

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

#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include <libspectrum.h>

#include "compat.h"
#include "utils.h"

#define PROGRAM_NAME "snapconv"
char *progname;

static void
fix_snapshot( libspectrum_snap *snap )
{
  libspectrum_byte a = libspectrum_snap_a( snap );
  libspectrum_byte f = libspectrum_snap_f( snap );
  libspectrum_snap_set_a( snap, f );
  libspectrum_snap_set_f( snap, a );
  a = libspectrum_snap_a_( snap );
  f = libspectrum_snap_f_( snap );
  libspectrum_snap_set_a_( snap, f );
  libspectrum_snap_set_f_( snap, a );
}

static void
show_version( void )
{
  printf(
   PROGRAM_NAME " (" PACKAGE ") " PACKAGE_VERSION "\n"
   "Copyright (c) 2003-2005 Philip Kendall\n"
   "License GPLv2+: GNU GPL version 2 or later "
   "<http://gnu.org/licenses/gpl.html>\n"
   "This is free software: you are free to change and redistribute it.\n"
   "There is NO WARRANTY, to the extent permitted by law.\n" );
}

static void
show_help( void )
{
  printf(
    "Usage: %s [OPTION]... <infile> <outfile>\n"
    "Sinclair ZX Spectrum snapshot converter.\n"
    "\n"
    "Options:\n"
    "  -c             Force all data in the output snapshot to be compressed.\n"
    "  -n             Store uncompressed data in the output snapshot.\n"
    "  -f             Swap the A and F and A' and F' registers. Fix buggy snapshots\n"
    "                   created by an old libspectrum version.\n"
    "  -h, --help     Display this help and exit.\n"
    "  -V, --version  Output version information and exit.\n"
    "\n"
    "Report %s bugs to <%s>\n"
    "%s home page: <%s>\n"
    "For complete documentation, see the manual page of %s.\n",
    progname,
    PROGRAM_NAME, PACKAGE_BUGREPORT, PACKAGE_NAME, PACKAGE_URL, PROGRAM_NAME
  );
}

int
main( int argc, char **argv )
{
  libspectrum_snap *snap;
  libspectrum_id_t type; libspectrum_class_t class;
  unsigned char *buffer; size_t length;
  libspectrum_creator *creator;
  int flags;
  int compress = 0;
  int fix = 0;
  FILE *f;

  int error = 0;
  int c;

  struct option long_options[] = {
    { "help", 0, NULL, 'h' },
    { "version", 0, NULL, 'V' },
    { 0, 0, 0, 0 }
  };

  progname = argv[0];

  while( ( c = getopt_long( argc, argv, "cnfhV", long_options, NULL ) ) != -1 ) {

    switch( c ) {

    case 'c': compress = LIBSPECTRUM_FLAG_SNAPSHOT_ALWAYS_COMPRESS;
      break;

    case 'n': compress = LIBSPECTRUM_FLAG_SNAPSHOT_NO_COMPRESSION;
      break;

    case 'f': fix = 1;
      break;

    case 'h': show_help(); return 0;

    case 'V': show_version(); return 0;

    case '?':
      /* getopt prints an error message to stderr */
      error = 1;
      break;

    default:
      error = 1;
      fprintf( stderr, "%s: unknown option `%c'\n", progname, (char) c );
      break;

    }
  }
  argc -= optind;
  argv += optind;

  if( error ) {
    fprintf( stderr, "Try `%s --help' for more information.\n", progname );
    return error;
  }

  if( argc < 2 ) {
    fprintf( stderr, "%s: usage: %s [-c] [-n] [-f] <infile> <outfile>\n", progname,
	     progname );
    fprintf( stderr, "Try `%s --help' for more information.\n", progname );
    return 1;
  }

  error = init_libspectrum(); if( error ) return error;

  snap = libspectrum_snap_alloc();

  if( read_file( argv[0], &buffer, &length ) ) {
    libspectrum_snap_free( snap );
    return 1;
  }

  error = libspectrum_snap_read( snap, buffer, length, LIBSPECTRUM_ID_UNKNOWN,
				 argv[0] );
  if( error ) {
    libspectrum_snap_free( snap ); free( buffer );
    return error;
  }

  free( buffer );

  if( fix ) fix_snapshot( snap );

  error = libspectrum_identify_file_with_class( &type, &class, argv[1], NULL,
                                                0 );
  if( error ) { libspectrum_snap_free( snap ); return error; }

  if( class != LIBSPECTRUM_CLASS_SNAPSHOT ) {
    fprintf( stderr, "%s: '%s' is not a snapshot file\n", progname, argv[1] );
    libspectrum_snap_free( snap );
    return 1;
  }

  error = get_creator( &creator, "snapconv" );
  if( error ) { libspectrum_snap_free( snap ); return error; }

  length = 0;
  error = libspectrum_snap_write( &buffer, &length, &flags, snap, type,
                                  creator, compress );
  if( error ) {
    libspectrum_creator_free( creator ); libspectrum_snap_free( snap );
    return error;
  }

  if( flags & LIBSPECTRUM_FLAG_SNAPSHOT_MAJOR_INFO_LOSS ) {
    fprintf( stderr,
	     "%s: warning: major information loss during conversion\n",
	     progname );
  } else if( flags & LIBSPECTRUM_FLAG_SNAPSHOT_MINOR_INFO_LOSS ) {
    fprintf( stderr,
	     "%s: warning: minor information loss during conversion\n",
	     progname );
  }
  error = libspectrum_creator_free( creator );
  if( error ) { free( buffer ); libspectrum_snap_free( snap ); return error; }

  error = libspectrum_snap_free( snap );
  if( error ) { free( buffer ); return error; }

  f = fopen( argv[1], "wb" );
  if( !f ) {
    fprintf( stderr, "%s: couldn't open '%s': %s\n", progname, argv[1],
	     strerror( errno ) );
    free( buffer );
    return 1;
  }
    
  if( fwrite( buffer, 1, length, f ) != length ) {
    fprintf( stderr, "%s: error writing to '%s'\n", progname, argv[1] );
    free( buffer );
    fclose( f );
    return 1;
  }

  free( buffer );
  fclose( f );

  return 0;
}
