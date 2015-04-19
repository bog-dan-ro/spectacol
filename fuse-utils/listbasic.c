/* listbasic.c: extract the BASIC listing from a snapshot or tape file
   Copyright (c) 2002 Chris Cowley
                 2003 Philip Kendall, Darren Salt
                 2007 Stuart Brady

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

    Chris: ccowley@grok.co.uk

*/

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <libspectrum.h>

#include "utils.h"

/* argv[0] */
char *progname;

int betabasic = 0;

/* A function used to read memory */
typedef
  libspectrum_byte (*memory_read_fn)( libspectrum_word offset, void *data );

int parse_snapshot_file( const unsigned char *buffer, size_t length,
			 libspectrum_id_t type );
libspectrum_byte read_snap_memory( libspectrum_word address, void *data );

int parse_tape_file( const unsigned char *buffer, size_t length,
		     libspectrum_id_t type );
libspectrum_byte read_tape_block( libspectrum_word offset, void *data );

int extract_basic( libspectrum_word offset, libspectrum_word end,
		   memory_read_fn get_byte, void *data );
int detokenize( libspectrum_word offset, int length,
		memory_read_fn get_byte, void *data );

int main(int argc, char* argv[])
{
  unsigned char *buffer; size_t length;
  libspectrum_id_t type;
  libspectrum_class_t class;

  int c;
  int error = 0;

  progname = argv[0];

  while( ( c = getopt( argc, argv, "b" ) ) != -1 ) {

    switch( c ) {

    case 'b': betabasic = 1; break;

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

  if( error || argc != 1 ) {
    fprintf( stderr, "%s: usage: %s [-b] <file>\n", progname, progname );
    return 1;
  }

  error = init_libspectrum(); if( error ) return error;

  error = read_file( argv[0], &buffer, &length ); if( error ) return error;

  error = libspectrum_identify_file_with_class( &type, &class, argv[0], buffer,
						length );
  if( error ) { free( buffer ); return error; }

  switch( class ) {

  case LIBSPECTRUM_CLASS_SNAPSHOT:
    error = parse_snapshot_file( buffer, length, type );
    if( error ) { free( buffer ); return error; }
    break;

  case LIBSPECTRUM_CLASS_TAPE:
    error = parse_tape_file( buffer, length, type );
    if( error ) { free( buffer ); return error; }
    break;

  case LIBSPECTRUM_CLASS_UNKNOWN:
    fprintf( stderr, "%s: couldn't identify the file type of `%s'\n",
	     progname, argv[0] );
    free( buffer );
    return 1;

  default:
    fprintf( stderr, "%s: `%s' is an unsupported file type\n",
	     progname, argv[0] );
    free( buffer );
    return 1;

  }

  free( buffer );

  return 0;
}

int
parse_snapshot_file( const unsigned char *buffer, size_t length,
		     libspectrum_id_t type )
{
  libspectrum_snap *snap = libspectrum_snap_alloc();
  libspectrum_word program_address;

  int error;

  error = libspectrum_snap_read( snap, buffer, length, type, NULL );
  if( error ) { libspectrum_snap_free( snap ); return error; }

  program_address =
    read_snap_memory( 23635, snap ) | read_snap_memory( 23636, snap ) << 8;

  if( program_address < 23296 || program_address > 65530 ) {
    fprintf( stderr, "%s: invalid program start address 0x%04x\n", progname,
	     program_address );
    libspectrum_snap_free( snap );
    return 1;
  }

  error = extract_basic( program_address, 0, read_snap_memory, snap );
  if( error ) { libspectrum_snap_free( snap ); return error; }

  error = libspectrum_snap_free( snap ); if( error ) return error;

  return 0;
}

libspectrum_byte
read_snap_memory( libspectrum_word address, void *data )
{
  libspectrum_snap *snap = data;

  /* FIXME: assumes a 48K memory model */

  switch( address >> 14 ) {
    
  case 0: /* ROM; can't handle this */
    fprintf( stderr, "%s: attempt to read from ROM\n", progname );
    /* FIXME: find a better way to handle the error return */
    exit( 1 );

  case 1:
    return libspectrum_snap_pages( snap, 5 )[ address & 0x3fff ];

  case 2:
    return libspectrum_snap_pages( snap, 2 )[ address & 0x3fff ];

  case 3:
    return libspectrum_snap_pages( snap, 0 )[ address & 0x3fff ];

  default: /* Should never happen */
    fprintf( stderr, "%s: attempt to read from address %x\n", progname,
	     address );
    abort();

  }
}

int
parse_tape_file( const unsigned char *buffer, size_t length,
		 libspectrum_id_t type )
{
  libspectrum_tape *tape = libspectrum_tape_alloc();
  libspectrum_tape_iterator iterator;
  libspectrum_tape_block *tape_block;
  libspectrum_byte *data;
  libspectrum_word program_length;

  int error;

  error = libspectrum_tape_read( tape, buffer, length, type, NULL );
  if( error ) { libspectrum_tape_free( tape ); return error; }

  for( tape_block = libspectrum_tape_iterator_init( &iterator, tape );
       tape_block;
       tape_block = libspectrum_tape_iterator_next( &iterator ) )
  {

    /* Find a ROM block */
    if( libspectrum_tape_block_type( tape_block )
	!= LIBSPECTRUM_TAPE_BLOCK_ROM             ) continue;

    /* Start assuming this block is a BASIC header; firstly, check
       there is another block after this one to hold the data, and
       if there's not, just finish */
    if( !libspectrum_tape_peek_next_block( tape ) ) break;

    /* If it's a header, it must be 19 bytes long */
    if( libspectrum_tape_block_data_length( tape_block ) != 19 ) continue;

    data = libspectrum_tape_block_data( tape_block );

    /* The first byte should be zero to indicate a header */
    if( data[0] != 0 ) continue;

    /* The second byte should be zero to indicate a BASIC program */
    if( data[1] != 0 ) continue;

    /* The program length is stored at offset 16 */
    program_length = data[16] | data[17] << 8;

    /* Now have a look at the next block */
    tape_block = libspectrum_tape_peek_next_block( tape );

    /* Must be a ROM block */
    if( libspectrum_tape_block_type( tape_block )
	!= LIBSPECTRUM_TAPE_BLOCK_ROM             ) continue;

    /* Must be at least as long as the program */
    if( libspectrum_tape_block_data_length( tape_block ) < program_length )
      continue;

    data = libspectrum_tape_block_data( tape_block );

    /* Must be a data block */
    if( data[0] != 0xff ) continue;

    /* Now, just read the BASIC out */
    error = extract_basic( 1, program_length + 1, read_tape_block,
			   tape_block );
    if( error ) { libspectrum_tape_free( tape ); return error; }

    /* Don't parse this block again */
    libspectrum_tape_iterator_next( &iterator );
  }

  error = libspectrum_tape_free( tape ); if( error ) return error;

  return 0;
}

libspectrum_byte
read_tape_block( libspectrum_word offset, void *data )
{
  libspectrum_tape_block *tape_block = data;

  if( offset > libspectrum_tape_block_data_length( tape_block ) ) {
    fprintf( stderr, "%s: attempt to read past end of block\n", progname );
    exit( 1 );
  }

  return libspectrum_tape_block_data( tape_block )[offset];
}
  
int
extract_basic( libspectrum_word offset, libspectrum_word end,
	       memory_read_fn get_byte, void *data )
{
  int line_number, line_length;
  int error;

  while( !end || offset < end ) {

    line_number = get_byte( offset, data ) << 8 | get_byte( offset + 1, data );
    offset += 2;
    if( line_number >= 16384 ) break;

    line_length = get_byte( offset, data ) | get_byte( offset + 1, data ) << 8;
    offset += 2;

    /* Hide line 0 for Beta BASIC */
    if( line_number != 0 || !betabasic ) {
      printf( "%5d", line_number );
      if( betabasic ) putchar( ' ' );

      error = detokenize( offset, line_length, get_byte, data );
      if( error ) return error;

      printf( "\n" );
    }

    offset += line_length;
  }

  return 0;
}

static char
print_keyword( const char *keyword, char space )
{
  char last_char;

  if( !space && keyword[0] == ' ' ) {
    printf( "%s", keyword + 1 );
  } else {
    printf( "%s", keyword );
  }

  last_char = keyword[ strlen( keyword ) - 1 ];

  return( last_char != ' ' );
}

int
detokenize( libspectrum_word offset, int length,
	    memory_read_fn get_byte, void *data )
{
  int i;
  libspectrum_byte b;
  char keyword_next = 1, quote = 0, rem = 0;
  char space;

  char first; /* set for the first token in each statement */
  char nextfirst = 1;

  static const char keyword[][12] = {
    " SPECTRUM ",
    " PLAY ",	"RND",		"INKEY$",	"PI",
    "FN ",	"POINT ",	"SCREEN$ ",	"ATTR ",
    "AT ",	"TAB ",		"VAL$ ",	"CODE ",
    "VAL ",	"LEN ",		"SIN ",		"COS ",
    "TAN ",	"ASN ",		"ACS ",		"ATN ",
    "LN ",	"EXP ",		"INT ",		"SQR ",
    "SGN ",	"ABS ",		"PEEK ",	"IN ",
    "USR ",	"STR$ ",	"CHR$ ",	"NOT ",
    "BIN ",	" OR ",		" AND ",	"<=",
    ">=",	"<>",		" LINE ",	" THEN ",
    " TO ",	" STEP ",	" DEF FN ",	" CAT ",
    " FORMAT ",	" MOVE ",	" ERASE ",	" OPEN #",
    " CLOSE #",	" MERGE ",	" VERIFY ",	" BEEP ",
    " CIRCLE ",	" INK ",	" PAPER ",	" FLASH ",
    " BRIGHT ",	" INVERSE ",	" OVER ",	" OUT ",
    " LPRINT ",	" LLIST ",	" STOP ",	" READ ",
    " DATA ",	" RESTORE ",	" NEW ",	" BORDER ",
    " CONTINUE "," DIM ",	" REM ",	" FOR ",
    " GO TO ",	" GO SUB ",	" INPUT ",	" LOAD ",
    " LIST ",	" LET ",	" PAUSE ",	" NEXT ",
    " POKE ",	" PRINT ",	" PLOT ",	" RUN ",
    " SAVE ",	" RANDOMIZE ",	" IF ",		" CLS ",
    " DRAW ",	" CLEAR ",	" RETURN ",	" COPY "
  };

  /* Beta BASIC keywords: */
  static const char beta_keyword[][11] = {
    " KEYWORDS ", " DEF PROC ",	" PROC ",	" END PROC ",
    " RENUM ",	  " WINDOW ",	" AUTO ",	" DELETE ",
    " REF ",	  " JOIN ",	" EDIT ",	" KEYIN ",
    " LOCAL ",	  " DEFAULT ",	" DEF KEY ",	" CSIZE ",
    " ALTER ",	  " BLANK ",	" CLOCK ",	" DO ",
    " ELSE ",	  " FILL ",	" GET ",	" ENDIF ",
    " EXIT IF ",  " WHILE ",	" UNTIL ",	" LOOP ",
    " SORT ",	  " ON ERROR ",	" ON ",		" DPOKE ",
    " POP ",	  " ROLL ",	" SCROLL ",	" TRACE ",
    " USING ",
  };

  if( betabasic )
    space = 0;
  else
    space = 1;

  /* The Timex keywords, DELETE, ON ERR, STICK, SOUND, FREE, RESET,
   * are handled below */

  for( i = 0; i < length; i++ ) {

    char nextspace = 1;

    first = nextfirst;
    nextfirst = 0;

    b = get_byte( offset + i, data );

    if( b < 128 ) {

      switch( b ) {

      case 12:
        if( !keyword_next ) continue;
        nextspace = print_keyword( " DELETE ", space );
        break;

      case 14:				/* Skip encoded number */
        i += 5;
        continue;

      /* Skip encoded INK, PAPER, FLASH, BRIGHT, INVERSE, OVER */
      case 16: case 17: case 18: case 19: case 20: case 21:
        i++;
        continue;

      case 22: case 23:			/* Skip encoded AT, TAB */
        i += 2;
        continue;

      case  32:
        if( !betabasic || i != 0 ) putchar( b );
        nextspace = 0;
        nextfirst = first;
        break;

      case  34:
        if( !rem ) quote = !quote;
        putchar( b );
        break;

      case  58:
        if( !rem && !quote ) keyword_next = 2;
        nextfirst = 1;
        putchar( b );
        break;

      case  92:
        printf( "\\\\" );
        break;
/*
      case 96:
        putchar (0xA3);
        break;
*/
      case 123:
        if( keyword_next ) {
          keyword_next = 2;
          nextspace = print_keyword( " ON ERR ", space );
        } else {
          putchar( b );
	}
        break;

      case 124:
        if( keyword_next ) {
          nextspace = print_keyword( " STICK ", space );
	} else {
          putchar( b );
	}
        break;

      case 125:
        if( keyword_next ) {
          nextspace = print_keyword( " SOUND ", space );
        } else {
          putchar( b );
	}
        break;

      case 126:
        if( keyword_next ) {
          nextspace = print_keyword (" FREE ", space);
        } else {
          putchar( b );
	}
        break;

      case 127:
        if( keyword_next ) {
          nextspace = print_keyword (" RESET ", space);
        } else {
          printf( "\\*" ); /* putchar (0xA9); */
	}
        break;
	
      default:
        if( b < 32 ) continue;
        putchar( b );
        break;
      }

    } else if( betabasic && b >= 128 && b <= 164 ) {

      if( b == 164 && first ) {
	nextspace = print_keyword( keyword[ b - 163 ], space );
      } else {
	nextspace = print_keyword( beta_keyword[ b - 128 ], space );

	if( b == 148 ) { /* ELSE */
	  nextfirst = 1;
	  keyword_next = 2;
	}
      }

    } else if( b < 163 + 2 * quote ) {

      switch( b ) {

      case 128: printf( "\\  " ); break; /* Graphics characters */
      case 129: printf( "\\ '" ); break;
      case 130: printf( "\\' " ); break;
      case 131: printf( "\\''" ); break;
      case 132: printf( "\\ ." ); break;
      case 133: printf( "\\ :" ); break;
      case 134: printf( "\\'." ); break;
      case 135: printf( "\\':" ); break;
      case 136: printf( "\\. " ); break;
      case 137: printf( "\\.'" ); break;
      case 138: printf( "\\: " ); break;
      case 139: printf( "\\:'" ); break;
      case 140: printf( "\\.." ); break;
      case 141: printf( "\\.:" ); break;
      case 142: printf( "\\:." ); break;
      case 143: printf( "\\::" ); break;

      default: printf( "\\%c", b - 144 + 'a' ); break; /* UDGs */

      }
    
    } else {
      nextspace = print_keyword( keyword[ b - 163 ], space );
    }

    space = nextspace;

    switch (b) {
    case 203: nextfirst = 1; keyword_next = 1; break;	/* THEN */
    case 234: nextfirst = 1; rem = 1; break;		/* REM */
    default: if( keyword_next ) keyword_next--; break;
    }
  }

  return 0;
}
