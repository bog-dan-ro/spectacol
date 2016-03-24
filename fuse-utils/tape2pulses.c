/* tape2pulses.c: Dump pulses in tape files (tzx, tap, etc.) to text
   Copyright (c) 2016 Fredrick Meunier

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

   E-mail: fredm@spamcop.net

*/

#include <config.h>

#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <libspectrum.h>

#include "utils.h"

#define PROGRAM_NAME "tape2pulses"

static void show_help( void );
static void show_version( void );
static int read_tape( char *filename, libspectrum_tape **tape );
static int write_pulses( char *filename, libspectrum_tape *tape );

char *progname;

int
main( int argc, char **argv )
{
  int c, error = 0;
  libspectrum_tape *tzx;

  progname = argv[0];

  struct option long_options[] = {
    { "help", 0, NULL, 'h' },
    { "version", 0, NULL, 'V' },
    { 0, 0, 0, 0 }
  };

  while( ( c = getopt_long( argc, argv, "r:hV", long_options, NULL ) ) != -1 ) {

    switch( c ) {

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
    fprintf( stderr,
             "%s: usage: %s <infile> <outfile>\n",
             progname,
	     progname );
    fprintf( stderr, "Try `%s --help' for more information.\n", progname );
    return 1;
  }

  error = init_libspectrum(); if( error ) return error;

  if( read_tape( argv[0], &tzx ) ) return 1;

  if( write_pulses( argv[1], tzx ) ) {
    libspectrum_tape_free( tzx );
    return 1;
  }

  libspectrum_tape_free( tzx );

  return 0;
}

static void
show_version( void )
{
  printf(
    PROGRAM_NAME " (" PACKAGE ") " PACKAGE_VERSION "\n"
    "Copyright (c) 2016 Fredrick Meunier\n"
    "License GPLv2+: GNU GPL version 2 or later "
    "<http://gnu.org/licenses/gpl.html>\n"
    "This is free software: you are free to change and redistribute it.\n"
    "There is NO WARRANTY, to the extent permitted by law.\n" );
}

static void
show_help( void )
{
  printf(
    "Usage: %s [OPTION] <infile> <outfile>\n"
    "Converts ZX Spectrum tape images to audio files.\n"
    "\n"
    "Options:\n"
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

static int
read_tape( char *filename, libspectrum_tape **tape )
{
  libspectrum_byte *buffer; size_t length;

  if( read_file( filename, &buffer, &length ) ) return 1;

  *tape = libspectrum_tape_alloc();

  if( libspectrum_tape_read( *tape, buffer, length, LIBSPECTRUM_ID_UNKNOWN,
                             filename ) ) {
    free( buffer );
    return 1;
  }

  free( buffer );

  return 0;
}

static int
write_pulses( char *filename, libspectrum_tape *tape )
{
  libspectrum_error error;
  short level = 0; /* The last level output to this block */
  libspectrum_dword pulse_tstates = 0;
  libspectrum_dword balance_tstates = 0;
  int flags = 0;
  FILE *output_file;

  if( strncmp( filename, "-", 1 ) == 0 ) {
    output_file = stdout;
  } else {
    output_file = fopen(filename, "w");
    if(output_file == NULL) {
      fprintf( stderr, "%s: error opening pulse file for writing: %s\n",
               progname, strerror(errno) );
      return 1;
    }
  }

  while( !(flags & LIBSPECTRUM_TAPE_FLAGS_TAPE) ) {
    error = libspectrum_tape_get_next_edge( &pulse_tstates, &flags, tape );
    if( error != LIBSPECTRUM_ERROR_NONE ) {
      return 1;
    }

    /* Invert the microphone state */
    if( pulse_tstates ||
        ( flags & (LIBSPECTRUM_TAPE_FLAGS_STOP |
                   LIBSPECTRUM_TAPE_FLAGS_LEVEL_LOW |
                   LIBSPECTRUM_TAPE_FLAGS_LEVEL_HIGH ) ) ) {

      if( flags & LIBSPECTRUM_TAPE_FLAGS_NO_EDGE ) {
        /* Do nothing */
      } else if( flags & LIBSPECTRUM_TAPE_FLAGS_LEVEL_LOW ) {
        level = 0;
      } else if( flags & LIBSPECTRUM_TAPE_FLAGS_LEVEL_HIGH ) {
        level = 1;
      } else {
        level = !level;
      }

    }

    balance_tstates += pulse_tstates;

    if( flags & LIBSPECTRUM_TAPE_FLAGS_NO_EDGE ) continue;

    fprintf(output_file, "%u : %d\n", balance_tstates, level);

    balance_tstates = 0;
  }

  if( fclose(output_file) == EOF ) {
    fprintf( stderr, "%s: error closing pulse file: %s\n", progname,
             strerror(errno) );
    return 1;
  }

  return 0;
}
