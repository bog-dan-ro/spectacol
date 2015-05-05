/* rzxcheck.c: Check the signature on an RZX file
   Copyright (c) 2002-2003 Philip Kendall

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
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <libspectrum.h>

#include "utils.h"

#define PROGRAM_NAME "rzxcheck"

char *progname;			/* argv[0] */

static void
show_version( void )
{
  printf(
    PROGRAM_NAME " (" PACKAGE ") " PACKAGE_VERSION "\n"
    "Copyright (c) 2002-2003 Philip Kendall\n"
    "License GPLv2+: GNU GPL version 2 or later "
    "<http://gnu.org/licenses/gpl.html>\n"
    "This is free software: you are free to change and redistribute it.\n"
    "There is NO WARRANTY, to the extent permitted by law.\n" );
}

static void
show_help( void )
{
  printf(
    "Usage: %s [OPTION] <rzxfile>\n"
    "Verifies the digital signature found in a ZX Spectrum RZX file.\n"
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

int
main( int argc, char **argv )
{
  unsigned char *buffer; size_t length;

  const char *rzxfile;

  libspectrum_rzx *rzx;

  libspectrum_error error;
  libspectrum_dword keyid = 0;
  libspectrum_signature signature;
  struct rzx_key *key;

  int c;
  int bad_option = 0;

  struct option long_options[] = {
    { "version", 0, NULL, 'V' },
    { "help", 0, NULL, 'h' },
    { 0, 0, 0, 0 }
  };

  progname = argv[0];

  while( ( c = getopt_long( argc, argv, "Vh", long_options, NULL ) ) != -1 ) {

    switch( c ) {

    case 'V': show_version(); exit( 0 );

    case 'h': show_help(); exit( 0 );

    case '?':
      /* getopt prints an error message to stderr */
      bad_option = 1;
      break;

    default:
      bad_option = 1;
      fprintf( stderr, "%s: unknown option `%c'\n", progname, (char) c );
      break;

    }
  }
  argc -= optind;
  argv += optind;

  if( bad_option ) {
    fprintf( stderr, "Try `%s --help' for more information.\n", progname );
    return bad_option;
  }

  if( argc < 1 ) {
    fprintf( stderr, "%s: usage: %s <rzxfile>\n", progname, progname );
    fprintf( stderr, "Try `%s --help' for more information.\n", progname );
    return 2;
  }

  if( init_libspectrum() ) return 16;

  rzxfile = argv[0];

  rzx = libspectrum_rzx_alloc();

  if( read_file( rzxfile, &buffer, &length ) ) return 16;

  if( libspectrum_rzx_read( rzx, buffer, length ) ) {
    free( buffer );
    return 16;
  }

  keyid = libspectrum_rzx_get_keyid( rzx );
  if( !keyid ) {
    printf( "%s: no key ID found in '%s'\n", progname, rzxfile );
    libspectrum_rzx_free( rzx );
    free( buffer );
    return 16;
  }

  for( key = known_keys; key->id; key++ )
    if( keyid == key->id ) break;

  if( !key->id ) {
    printf( "%s: don't know anything about key ID %08x\n", progname,
	    keyid );
    libspectrum_rzx_free( rzx );
    free( buffer );
    return 16;
  }

  error = libspectrum_rzx_get_signature( rzx, &signature );
  if( error ) {
    libspectrum_rzx_free( rzx );
    free( buffer );
    return 16;
  }

  error = libspectrum_verify_signature( &signature, &( key->key ) );
  if( error && error != LIBSPECTRUM_ERROR_SIGNATURE ) {
    libspectrum_signature_free( &signature );
    libspectrum_rzx_free( rzx );
    free( buffer );
    return 16;
  }

  free( buffer );

  libspectrum_rzx_free( rzx ); free( rzx );
  libspectrum_signature_free( &signature );

  if( error == LIBSPECTRUM_ERROR_SIGNATURE ) {
    printf( "%s: BAD signature with key %08x (%s) in '%s'\n", progname,
	    key->id, key->description, rzxfile );
    return 1;
  } else {
    printf( "%s: good signature with key %08x (%s) in '%s'\n", progname,
	    key->id, key->description, rzxfile );
    return 0;
  }

}
