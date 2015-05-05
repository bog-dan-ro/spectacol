/* createhdf.c: Create an empty .hdf file
   Copyright (c) 2004-2006 Philip Kendall

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

   Philip Kendall <philip-fuse@shadowmagic.org.uk>

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

#include "compat.h"
#include "ide.h"

#define PROGRAM_NAME "createhdf"

const char *progname;

#define CHUNK_LENGTH 1 << 20

static void
show_version( void )
{
  printf(
    PROGRAM_NAME " (" PACKAGE ") " PACKAGE_VERSION "\n"
    "Copyright (c) 2004-2006 Philip Kendall\n"
    "License GPLv2+: GNU GPL version 2 or later "
    "<http://gnu.org/licenses/gpl.html>\n"
    "This is free software: you are free to change and redistribute it.\n"
    "There is NO WARRANTY, to the extent permitted by law.\n" );
}

static void
show_help( void )
{
  printf(
    "Usage: %s [OPTION]... <cylinders> <heads> <sectors> <file>\n"
    "Creates a blank image of an IDE hard disk in .hdf format.\n"
    "\n"
    "Options:\n"
    "  -c             Create .hdf image in compact mode, where only the low byte\n"
    "                   of every word is stored in the image.\n"
    "  -s             Do not create .hdf image as a sparse file, i.e., empty\n"
    "                   space synthesised by the operating system.\n"
    "  -v <version>   Specifies the version of .hdf image to be created (values\n"
    "                   1.0 or 1.1). Defaults to creating version 1.1 files.\n"
    "  -h, --help     Display this help and exit.\n"
    "  -V, --version  Output version information and exit.\n"
    "\n"
    "Non-option arguments:\n"
    "  cylinders      Specifies the number of cylinders in the image.\n"
    "  heads          Specifies the number of heads in the image.\n"
    "  sectors        Specifies the number of sectors in the image.\n"
    "  file           Specifies the file to which the image should be written.\n"
    "\n"
    "Report %s bugs to <%s>\n"
    "%s home page: <%s>\n"
    "For complete documentation, see the manual page of %s.\n",
    progname,
    PROGRAM_NAME, PACKAGE_BUGREPORT, PACKAGE_NAME, PACKAGE_URL, PROGRAM_NAME
  );
}

int
parse_options( int argc, char **argv, size_t *cylinders, size_t *heads,
	       size_t *sectors, int *compact, int *sparse,
	       const char **filename, enum hdf_version_t *version )
{
  int c;
  int error = 0;

  struct option long_options[] = {
    { "help", 0, NULL, 'h' },
    { "version", 0, NULL, 'V' },
    { 0, 0, 0, 0 }
  };

  while( ( c = getopt_long( argc, argv, "csv:hV", long_options,
                            NULL ) ) != -1 ) {

    switch( c ) {

    case 'c': *compact = 1; break;
    case 's': *sparse = 0; break;
    case 'v':
      if( strcasecmp( optarg, "1.0" ) == 0 ) {
	*version = HDF_VERSION_10;
      } else if( strcasecmp( optarg, "1.1" ) == 0 ) {
	*version = HDF_VERSION_11;
      } else {
        error = 1;
        fprintf( stderr, "%s: version not supported\n", progname );
      }
      break;

    case 'h':
      show_help();
      exit( 0 );

    case 'V':
      show_version();
      exit( 0 );

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

  if( error ) return error;

  if( argc - optind != 4 ) {
    fprintf( stderr,
	     "%s: usage: %s [-c] [-s] [-v<version>] <cylinders> <heads> <sectors> <hdf>\n",
	     progname, progname );
    return 1;
  }

  *cylinders = atoi( argv[ optind++ ] );
  *heads     = atoi( argv[ optind++ ] );
  *sectors   = atoi( argv[ optind++ ] );
  *filename  = argv[ optind++ ];

  return 0;
}

int
write_header( int fd, size_t cylinders, size_t heads, size_t sectors,
	      int compact, const char *filename, enum hdf_version_t version )
{
  char *hdf_header, *identity;
  size_t bytes, data_offset;

  data_offset = hdf_data_offset( version );

  hdf_header = calloc( 1, data_offset );
  if( !hdf_header ) {
    fprintf( stderr, "%s: out of memory at line %d\n", progname, __LINE__ );
    return 1;
  }

  memcpy( &hdf_header[ HDF_SIGNATURE_OFFSET ], HDF_SIGNATURE,
	  HDF_SIGNATURE_LENGTH );
  hdf_header[ HDF_VERSION_OFFSET ] = hdf_version( version );
  hdf_header[ HDF_COMPACT_OFFSET ] = compact;
  hdf_header[ HDF_DATA_OFFSET_OFFSET     ] = ( data_offset      ) & 0xff;
  hdf_header[ HDF_DATA_OFFSET_OFFSET + 1 ] = ( data_offset >> 8 ) & 0xff;

  identity = &hdf_header[ HDF_IDENTITY_OFFSET ];

  identity[ IDENTITY_CYLINDERS_OFFSET     ] = ( cylinders      ) & 0xff;
  identity[ IDENTITY_CYLINDERS_OFFSET + 1 ] = ( cylinders >> 8 ) & 0xff;

  identity[ IDENTITY_HEADS_OFFSET         ] = ( heads          ) & 0xff;
  identity[ IDENTITY_HEADS_OFFSET     + 1 ] = ( heads     >> 8 ) & 0xff;

  identity[ IDENTITY_SECTORS_OFFSET       ] = ( sectors        ) & 0xff;
  identity[ IDENTITY_SECTORS_OFFSET   + 1 ] = ( sectors   >> 8 ) & 0xff;

  bytes = write( fd, hdf_header, data_offset );
  if( bytes != data_offset ) {
    fprintf( stderr,
	     "%s: could write only %lu header bytes out of %lu to '%s'\n",
	     progname, (unsigned long)bytes, (unsigned long)data_offset,
	     filename );
    free( hdf_header );
    return 1;
  }

  free( hdf_header );

  return 0;
}

int
write_data( int fd, size_t cylinders, size_t heads, size_t sectors,
	    int compact, int sparse, const char *filename,
	    enum hdf_version_t version )
{
  size_t data_size;

  data_size = cylinders * heads * sectors * ( compact ? 0x100 : 0x200 );

  if( sparse ) {

    if( ftruncate( fd, data_size + hdf_data_offset( version ) ) ) {
      fprintf( stderr, "%s: error truncating '%s': %s\n", progname, filename,
	       strerror( errno ) );
      return 1;
    }

  } else {

    char buffer[ CHUNK_LENGTH ];
    size_t total_written;

    memset( buffer, 0, CHUNK_LENGTH );
    total_written = 0;

    while( data_size ) {

      size_t bytes_to_write, bytes_written;

      bytes_to_write = data_size > CHUNK_LENGTH ? CHUNK_LENGTH : data_size;

      bytes_written = write( fd, buffer, bytes_to_write );
      if( bytes_written != bytes_to_write ) {
	fprintf( stderr, "%s: could write only %lu bytes out of %lu to '%s'",
		 progname, (unsigned long)total_written + bytes_written,
		 (unsigned long)data_size, filename );
	return 1;
      }

      total_written += bytes_to_write;
      data_size -= bytes_to_write;
    }
  }

  return 0;
}

int
main( int argc, char **argv )
{
  size_t cylinders, heads, sectors;
  int compact, sparse;
  enum hdf_version_t version;
  const char *filename;
  int fd;
  int error;

  progname = argv[0];

  compact = 0;
  sparse = 1;
  version = HDF_VERSION_11;
  error = parse_options( argc, argv, &cylinders, &heads, &sectors, &compact,
			 &sparse, &filename, &version );
  if( error ) {
    fprintf( stderr, "Try `%s --help' for more information.\n", progname );
    return error;
  }

  fd = open( filename, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY,
			S_IRUSR | S_IWUSR |
			S_IRGRP | S_IWGRP |
			S_IROTH | S_IWOTH   );
  if( fd == -1 ) {
    fprintf( stderr, "%s: error opening '%s': %s\n", progname, filename,
	     strerror( errno ) );
    return 1;
  }

  error = write_header( fd, cylinders, heads, sectors, compact, filename,
			version );
  if( error ) return error;

  error = write_data( fd, cylinders, heads, sectors, compact, sparse,
		      filename, version );
  if( error ) return error;

  return 0;
}
