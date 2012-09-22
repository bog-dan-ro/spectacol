/* tapeconv.c: Convert .tzx files to .tap files
   Copyright (c) 2002-2008 Philip Kendall, Fredrick Meunier

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
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <libspectrum.h>

#include "utils.h"

#define DESCRIPTION_BUFFER_LEN 0x10

static int get_type_from_string( libspectrum_id_t *type, const char *string );
static int read_tape( char *filename, libspectrum_tape **tape );
static int update_archive_file( char *archive_file, libspectrum_tape *tape );
static int append_scr_file( char *scr_file, libspectrum_tape *tape );
static int append_inlay_file( char *inlay_file, libspectrum_tape *tape );
static int write_tape( char *filename, libspectrum_tape *tape );
static int beautify_tzx_file( libspectrum_tape *tape );
static int remove_block_type( libspectrum_tape *tape,
                              libspectrum_tape_type id );
static int is_filetype( const char* new_filename, const char* type );

char *progname;

char *archive_file = NULL;
int beautify = 0;
char *scr_file = NULL;
char *inlay_file = NULL;

int
main( int argc, char **argv )
{
  int c, error;
  libspectrum_tape *tzx;

  progname = argv[0];

  error = init_libspectrum(); if( error ) return error;

  while( ( c = getopt( argc, argv, "a:s:i:b" ) ) != -1 ) {

    switch( c ) {

    case 'a': archive_file = optarg ; break;
    case 'b': beautify = 1; break;
    case 's': scr_file = optarg ; break;
    case 'i': inlay_file = optarg ; break;

    }

  }

  argc -= optind;
  argv += optind;

  if( argc < 2 ) {
    fprintf( stderr,
             "%s: usage: %s [-s <scr file>] [-a <archive info tzx>] "
             "[-b] [-i <inlay image>] <infile> <outfile>\n",
             progname,
	     progname );
    return 1;
  }

  if( read_tape( argv[0], &tzx ) ) return 1;

  if( archive_file && update_archive_file( archive_file, tzx ) ) {
    libspectrum_tape_free( tzx );
    return 1;
  }

  if( scr_file && append_scr_file( scr_file, tzx ) ) {
    libspectrum_tape_free( tzx );
    return 1;
  }

  if( inlay_file && append_inlay_file( inlay_file, tzx ) ) {
    libspectrum_tape_free( tzx );
    return 1;
  }

  if( beautify && beautify_tzx_file( tzx ) ) {
    libspectrum_tape_free( tzx );
    return 1;
  }

  if( write_tape( argv[1], tzx ) ) {
    libspectrum_tape_free( tzx );
    return 1;
  }

  libspectrum_tape_free( tzx );

  return 0;
}

static int
get_type_from_string( libspectrum_id_t *type, const char *string )
{
  libspectrum_class_t class;
  int error;

  /* Work out what sort of file we want from the filename; default to
     .tzx if we couldn't guess */
  error = libspectrum_identify_file_with_class( type, &class, string, NULL,
						0 );
  if( error ) return error;

  if( class != LIBSPECTRUM_CLASS_TAPE || *type == LIBSPECTRUM_ID_UNKNOWN )
    *type = LIBSPECTRUM_ID_TAPE_TZX;

  return 0;
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
remove_block_type( libspectrum_tape *tape, libspectrum_tape_type id )
{
  libspectrum_tape_block* block;
  libspectrum_tape_iterator iterator;

  for( block = libspectrum_tape_iterator_init( &iterator, tape );
       block;
       block = libspectrum_tape_iterator_next( &iterator ) ) {
    if( libspectrum_tape_block_type( block ) == id ) {
      libspectrum_tape_remove_block( tape, iterator );

      /* Iterator is invalidated by delete, so start again */
      block = libspectrum_tape_iterator_init( &iterator, tape );
    }
  }

  return 0;
}

static int
update_archive_file( char *archive_file, libspectrum_tape *tape )
{
  libspectrum_tape_block* info_block;
  libspectrum_error error;
  libspectrum_tape *tzx;
  libspectrum_tape_iterator iterator;

  if( read_tape( archive_file, &tzx ) ) return 1;

  /* Get the new archive block */
  info_block = libspectrum_tape_iterator_init( &iterator, tzx );
  while( info_block &&
         libspectrum_tape_block_type( info_block ) !=
           LIBSPECTRUM_TAPE_BLOCK_ARCHIVE_INFO ) {
    info_block = libspectrum_tape_iterator_next( &iterator );
  }

  if( !info_block ) {
    libspectrum_tape_free( tzx );
    return 1;
  }

  /* Remove any existing archive block */
  error = remove_block_type( tape, LIBSPECTRUM_TAPE_BLOCK_ARCHIVE_INFO );
  if( error ) { libspectrum_tape_free( tzx ); return error; }

  /* Finally, put the new info block at the beginning of the block list */
  error = libspectrum_tape_insert_block( tape, info_block, 0 );
  if( error ) { libspectrum_tape_free( tzx ); return error; }

  return 0;
}

static int
append_scr_file( char *scr_file, libspectrum_tape *tape )
{
  libspectrum_tape_block* block;
  char *description;
  libspectrum_byte* scr_data; size_t scr_length;
  libspectrum_byte* custom_block_data; size_t custom_block_length;

  block = libspectrum_tape_block_alloc( LIBSPECTRUM_TAPE_BLOCK_CUSTOM );

  /* Get the description */
  description = malloc( DESCRIPTION_BUFFER_LEN );
  memcpy( description, "Spectrum Screen ", DESCRIPTION_BUFFER_LEN );
  libspectrum_tape_block_set_text( block, description );

  /* Read in the data */
  if( read_file( scr_file, &scr_data, &scr_length ) ) {
    free( description );
    free( block );
    return 1;
  }

  custom_block_length = scr_length + 2;
  custom_block_data = malloc( custom_block_length );

  /* Picture description length 0 == "Loading Screen" */
  custom_block_data[0] = 0;
  /* Border colour 0 == black */
  custom_block_data[1] = 0;
  /* and the SCR itself */
  memcpy( custom_block_data + 2, scr_data, scr_length );

  libspectrum_tape_block_set_data_length( block, custom_block_length );
  libspectrum_tape_block_set_data( block, custom_block_data );

  /* Finally, put the block into the block list */
  libspectrum_tape_append_block( tape, block );

  return 0;
}

static int
beautify_tzx_file( libspectrum_tape *tape )
{
  libspectrum_error error;

  /* Remove any existing concat blocks */
  error = remove_block_type( tape, LIBSPECTRUM_TAPE_BLOCK_CONCAT );
  if( error ) { return error; }

  return 0;
}

static int
is_filetype( const char* new_filename, const char* type )
{
  return ( strlen( new_filename ) >= strlen(type) &&
           !strcasecmp( new_filename + (strlen( new_filename ) - strlen(type) ),
                        type ) );
}
static int
append_inlay_file( char *inlay_file, libspectrum_tape *tape )
{
  libspectrum_tape_block* block;
  char *description;
  libspectrum_byte* jpg_data; size_t jpg_length;
  libspectrum_byte* custom_block_data; size_t custom_block_length;

  block = libspectrum_tape_block_alloc( LIBSPECTRUM_TAPE_BLOCK_CUSTOM );

  /* Get the description */
  description = malloc( DESCRIPTION_BUFFER_LEN );
  memcpy( description, "Picture        ", DESCRIPTION_BUFFER_LEN );
  libspectrum_tape_block_set_text( block, description );

  /* Read in the data */
  if( read_file( inlay_file, &jpg_data, &jpg_length ) ) {
    free( description );
    free( block );
    return 1;
  }

  custom_block_length = jpg_length + 2;
  custom_block_data = malloc( custom_block_length );

  /* Picture format */
  if( is_filetype( inlay_file, ".jpg" ) ) {
    custom_block_data[0] = 1;
  } else if( is_filetype( inlay_file, ".gif" ) ) {
    custom_block_data[0] = 0;
  } else {
    return 1;
  }

  /* Picture description length 0 == "Inlay Card" */
  custom_block_data[1] = 0;
  /* and the JPG itself */
  memcpy( custom_block_data + 2, jpg_data, jpg_length );

  libspectrum_tape_block_set_data_length( block, custom_block_length );
  libspectrum_tape_block_set_data( block, custom_block_data );

  /* Finally, put the block into the block list */
  libspectrum_tape_append_block( tape, block );

  return 0;
}

static int
write_tape( char *filename, libspectrum_tape *tape )
{
  libspectrum_byte *buffer; size_t length;
  FILE *f;
  libspectrum_id_t type;

  if( get_type_from_string( &type, filename ) ) return 1;

  length = 0;

  if( libspectrum_tape_write( &buffer, &length, tape, type ) ) return 1;

  f = fopen( filename, "wb" );
  if( !f ) {
    fprintf( stderr, "%s: couldn't open '%s': %s\n", progname, filename,
	     strerror( errno ) );
    free( buffer );
    return 1;
  }
    
  if( fwrite( buffer, 1, length, f ) != length ) {
    fprintf( stderr, "%s: error writing to '%s'\n", progname, filename );
    free( buffer );
    fclose( f );
    return 1;
  }

  free( buffer );

  if( fclose( f ) ) {
    fprintf( stderr, "%s: couldn't close '%s': %s\n", progname, filename,
	     strerror( errno ) );
    return 1;
  }

  return 0;
}
