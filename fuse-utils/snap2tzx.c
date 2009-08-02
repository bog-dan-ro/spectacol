/* snap2tzx.c: Snapshot to .tzx tape image converter
   Copyright (c) 1997-2001 ThunderWare Research Center, written by
                           Martijn van der Heide,
		 2003 Tomaz Kac,
		 2003 Philip Kendall

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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <libspectrum.h>

#include "compat.h"
#include "utils.h"

char *progname;			/* argv[0] */

typedef struct settings_t {

  const char *input_filename;	/* Snapshot file to convert */
  const char *external_filename; /* External screenshot */
  char output_filename[512];	/* Target filename */

  char loader_name[9];		/* Name displayed by BASIC loader */
  char game_name[33];		/* Name displayed while game is loading */
  char info1[33], info2[33];	/* Extra lines displayed while loading */

  int speed;			/* Loader speed */
  int load_colour;		/* Border colour */
  libspectrum_byte bright;	/* Should final attributes line be bright? */

} settings_t;

static libspectrum_byte WorkBuffer[65535];

static int verbose = 0;

static void
print_error( char *format, ... )
{
  va_list ap;

  fprintf( stderr, "%s: ", progname );

  va_start( ap, format );
  vfprintf( stderr, format, ap );
  va_end( ap );

  fprintf( stderr, "\n" );
}

static void
print_verbose( char *format, ... )
{
  va_list ap;

  if( !verbose ) return;

  va_start( ap, format );
  vprintf( format, ap );
  va_end( ap );
}

static void crunch_z80 (libspectrum_byte *BufferIn, libspectrum_word BlLength, libspectrum_byte *BufferOut, size_t *CrunchedLength)

/**********************************************************************************************************************************/
/* Pre   : `BufferIn' points to the uncrunched spectrum block, `BlLength' holds the length, `BufferOut' points to the result.     */
/*         `CrunchedLength' is the length after crunching.                                                                        */
/*         The version of Z80 file must have been determined.                                                                     */
/* Post  : The block has been crunched from `BufferIn' to `BufferOut'.                                                            */
/*         If the crunched size is larger than (or equal to) the non-crunched size, the block is returned directly with           */
/*         `CrunchedLength' set to 0x0000.                                                                                        */
/*         The crunched block is returned reversed (for backward loading).                                                        */
/* Import: None.                                                                                                                  */
/**********************************************************************************************************************************/

{
  libspectrum_word register IndexIn;
  libspectrum_word register IndexOut       = 0;
  libspectrum_word          LengthOut;
  libspectrum_word register RunTimeLength;
  int          CrunchIt       = 0;
  libspectrum_byte          RepeatedCode;
  libspectrum_byte          PrevByte       = 0x00;
  libspectrum_byte         *BufferTemp;

  

  BufferTemp	= WorkBuffer + (libspectrum_word)32768;                                                        /* Use the top 16Kb of Workbuffer */
  for (IndexIn = 0 ; IndexIn < BlLength - 4 ; IndexIn ++)                                     /* Crunch it into the second buffer */
  {
    if (*(BufferIn + IndexIn) == 0xED &&                                                                     /* Exceptional cases */
        *(BufferIn + IndexIn + 1) == 0xED)
      CrunchIt = 1;
    else if (*(BufferIn + IndexIn) == *(BufferIn + IndexIn + 1) &&
             *(BufferIn + IndexIn) == *(BufferIn + IndexIn + 2) &&
             *(BufferIn + IndexIn) == *(BufferIn + IndexIn + 3) &&
             *(BufferIn + IndexIn) == *(BufferIn + IndexIn + 4) &&                                        /* At least 5 repeats ? */
             PrevByte != 0xED)                                                                         /* Not right after a 0xED! */
      CrunchIt = 1;
    if (CrunchIt)                                                                                             /* Crunch a block ? */
    {
      CrunchIt = 0;
      RunTimeLength = 1;
      RepeatedCode = *(BufferIn + IndexIn);
      while (RunTimeLength < 255 && RunTimeLength + IndexIn < BlLength && *(BufferIn + RunTimeLength + IndexIn) == RepeatedCode)
        RunTimeLength ++;
      *(BufferTemp + (IndexOut ++)) = 0xED;
      *(BufferTemp + (IndexOut ++)) = 0xED;
      *(BufferTemp + (IndexOut ++)) = (libspectrum_byte) RunTimeLength;
      *(BufferTemp + (IndexOut ++)) = RepeatedCode;
      IndexIn += RunTimeLength - 1;
      PrevByte = 0x00;
    }
    else
      PrevByte = *(BufferTemp + (IndexOut ++)) = *(BufferIn + IndexIn);                                          /* No, just copy */
  }
  while (IndexIn < BlLength)
    *(BufferTemp + (IndexOut ++)) = *(BufferIn + (IndexIn ++));                                                 /* Copy last ones */
  if (IndexOut >= BlLength)                                                              /* Compressed larger than uncompressed ? */
  {
    memcpy (BufferTemp, BufferIn, BlLength);                                                /* Return the inputblock uncompressed */
    LengthOut = BlLength;
    IndexOut = 0x0000;                                                                                  /* Signal: not compressed */
  }
  else
    LengthOut = IndexOut;
  BufferTemp = WorkBuffer + (libspectrum_word)32768 + LengthOut - 1;                                       /* Point to the last crunched byte */
  for (IndexIn = 0 ; IndexIn < LengthOut ; IndexIn ++)                             /* Now reverse the result for backward loading */
    *(BufferOut + IndexIn) = *(BufferTemp - IndexIn);
  *CrunchedLength = IndexOut;
}

int test_decz80(libspectrum_byte *source, int final_len, int source_len)
{
  /* source is not reversed !!! */

	int dst_pnt = 0;
	int src_pnt = final_len-source_len;

	int overwrite = 0;

	libspectrum_byte b1;
	libspectrum_byte b2;
	libspectrum_byte times;
	libspectrum_byte value;

	int off = src_pnt;

	while (dst_pnt < final_len && !overwrite)
	{
		b1 = source[src_pnt-off];
		src_pnt++;
		b2 = source[src_pnt-off];

		if (b1 == 0xED && b2 == 0xED)
		{
		  /* Repeat */
			times = source[(src_pnt-off)+1];
			value = source[(src_pnt-off)+2];
			dst_pnt += times;
			src_pnt+=3;
		}
		else
		{
			dst_pnt++;
		}

		if (dst_pnt > src_pnt)
		{
			overwrite = 1;
		}
	}
	return overwrite;
}

static int
test_rev_decz80( libspectrum_byte *source, int final_len, int source_len )
{
  /* source is reversed !!! */

	int dst_pnt = final_len-1;
	int src_pnt = source_len-1;

	int overwrite = 0;

	libspectrum_byte b1;
	libspectrum_byte b2;
	libspectrum_byte times;
	libspectrum_byte value;

	while (dst_pnt >=0 && !overwrite)
	{
		b1 = source[src_pnt];
		src_pnt--;
		b2 = source[src_pnt];

		if (b1 == 0xED && b2 == 0xED)
		{
		  /* Repeat */
			times = source[src_pnt-1];
			value = source[src_pnt-2];
			dst_pnt -= times;
			src_pnt-=3;
		}
		else
		{
			dst_pnt--;
		}

		if (dst_pnt < src_pnt)
		{
			overwrite = 1;
		}
	}
	return overwrite;
}

static void reverse_block (libspectrum_byte *BufferOut, libspectrum_byte *BufferIn)
{
  libspectrum_word register  Cnt;
  libspectrum_byte          *BIn;
  libspectrum_byte          *BOut;

  for (BIn = BufferIn, BOut = BufferOut + 16383, Cnt = 0 ; Cnt < 16384 ; Cnt ++)
    *(BOut --) = *(BIn ++);
}

static int
initialise_settings( settings_t *settings )
{
  settings->input_filename = NULL;
  settings->external_filename = NULL;

  settings->loader_name[0] = '\0';
  settings->game_name[0] = '\0';

  /* 32 spaces */
  strncpy( settings->info1, "                                ", 33 );
  strncpy( settings->info2, "                                ", 33 );

  settings->speed = 3;
  settings->load_colour = -1;
  settings->bright = 0x00;

  return 0;
}

/* Replace any occurences of '~' in 'buffer' with the copyright symbol
   (0x7f in the Spectrum's character set) */
static void
change_copyright( char *buffer )
{
  for( ; *buffer; buffer++ ) if( *buffer == '~' ) *buffer = 0x7f;
}

static void
centre_name( char *name )
{
  int i, num;

  for( i=31; i >=0 && name[i]==' '; i-- );
  num = 31-i;

  if( num>1 ) {
    for( i=31; i >= num/2; i-- ) name[i] = name[i-(num/2)];
    for( i=0; i < num/2; i++ ) name[i]=' ';
  }
}

static int
setup_string( char *dest, size_t length, const char *src )
{
  snprintf( dest, length + 1, "%*s", (int)-length, src );
  change_copyright( dest );
  centre_name( dest );

  return 0;
}

static void
print_usage( int title )
{
  char *buffer, *buffer2;

  buffer = strdup( progname );
  if( !buffer ) {
    print_error( "out of memory at %s:%d", __FILE__, __LINE__ );
    return;
  }

  buffer2 = compat_basename( buffer );
  
  if( title ) {
    printf(
	    "\n"
	    "snap2tzx: snapshot to TZX converter\n"
	    "\n"
	    "Copyright (c) 1997-2001 ThunderWare Research Center,\n"
	    "              2003 Tomaz Kac,\n"
	    "              2003 Philip Kendall.\n"
	    "\n"
	    "This program is distributed in the hope that it will be useful, but WITHOUT\n"
	    "ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or\n"
	    "FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for\n"
	    "more details.\n"
	  );
  }

  printf(
	 "\n"
	  "Usage: %s [options] <snapshot>\n"
	  "\n"
	  "Options:\n"
	  "\n"
	  "  -1 s  Show string 's' when loading (max 32 characters)\n"
	  "  -2 s  Another line when loading (max 32 characters)\n"
	  "  -b n  Border colour\n"
	  "  -g s  Use 's' as the game name when loading (max 32 characters)\n"
	  "  -l s  Use 's' as the BASIC filename (max 8 characters)\n"
	  "  -o f  Output to file 'f'\n"
	  "  -r    Make final attribute line BRIGHT\n"
	  "  -s n  Set loading speed: 0: 1500 1: 2250 2:3000 3: 6000 bps\n"
	  "  -v    Verbose output\n"
	  "  -$ f  Use .scr file 'f' as the loading screen\n"
	  "\n"
	  "In string parameters, the copyright symbol will be substituted for '~'.\n"
	  "\n",
	  buffer2
	);
}

static int
parse_args( settings_t *settings, int argc, char **argv )
{
  int c, got_game_name, got_loader_name, got_out_filename, error;
  char *buffer, *buffer2, *last_dot;

  error = initialise_settings( settings ); if( error ) return error;

  got_game_name = got_loader_name = got_out_filename = 0;

  while( ( c = getopt( argc, argv, "1:2:b:g:l:o:rs:v$:" ) ) != EOF ) {
    switch (c) {

    case '1':
      error = setup_string( settings->info1, 32, optarg );
      if( error ) return error;
      break;

    case '2':
      error = setup_string( settings->info2, 32, optarg );
      if( error ) return error;
      break;

      /* Border Colour when loading */
    case 'b':
      settings->load_colour = atoi( optarg );
      if( settings->load_colour < 0 || settings->load_colour > 7 ) {
	print_error( "invalid border colour (%d)", settings->load_colour );
	return 1;
      }
      break;

      /* Game Name (When Loader is loaded) */
    case 'g':
      error = setup_string( settings->game_name, 32, optarg );
      if( error ) return error;
      got_game_name = 1;
      break;

      /* Loader Name (Loading: Name) */
    case 'l':
      snprintf( settings->loader_name, 9, "%-8s", optarg );
      got_loader_name = 1;
      break;
      
      /* Output Filename */
    case 'o':
      strncpy( settings->output_filename, optarg, 511 );
      settings->output_filename[511] = 0;
      got_out_filename = 1;
      break;

    case 'r': settings->bright = 0x40; break;

      /* Speed value  */
    case 's':
      settings->speed = atoi( optarg );
      if( settings->speed < 0 || settings->speed > 3 ) {
	print_error( "invalid speed (%d)", settings->speed );
	return 1;
      }
      break;

      /* Verbose output */
    case 'v': verbose = 1; break;

      /* External Loading Screen */
    case '$': settings->external_filename = optarg; break;

    case '?':
      print_error( "unknown option" );
      print_usage( 0 );
      return 1;

    }
  }

  if( argv[optind] == NULL ) {
    print_usage( 1 );
    return 1;
  }

  /* Snapshot filename */
  settings->input_filename = argv[ optind ];

  /* Get the basename without the last extension. Not always going to
     be exactly what we want ('gamename.z80.gz') but will be close to
     right in most cases */
  
  buffer = strdup( settings->input_filename );
  if( !buffer ) {
    print_error( "out of memory at %s:%d", __FILE__, __LINE__ );
    return 1;
  }

  buffer2 = compat_basename( buffer );
  last_dot = strrchr( buffer2, '.' ); if( last_dot ) *last_dot = '\0';

  if( !got_out_filename ) {

    /* 4 characters less than the size of the buffer so we will always
       have room to append ".tzx" */
    strncpy( settings->output_filename, buffer2, 507 );
    settings->output_filename[507] = '\0';
    strcat( settings->output_filename, ".tzx" );
  }

  if( !got_loader_name ) snprintf( settings->loader_name, 9, "%-8s", buffer2 );
  if( !got_game_name ) {
    error = setup_string( settings->game_name, 32, buffer2 );
    if( error ) return error;
  }

  free( buffer );
  
  return 0;
}

static int
load_snap( libspectrum_snap **snap, const char *filename )
{
  int error;
  unsigned char *buffer; size_t length;

  *snap = libspectrum_snap_alloc();

  if( read_file( filename, &buffer, &length ) ) {
    libspectrum_snap_free( *snap );
    return 1;
  }

  error = libspectrum_snap_read( *snap, buffer, length, LIBSPECTRUM_ID_UNKNOWN,
				 filename );
  if( error ) {
    libspectrum_snap_free( *snap ); free( buffer );
    return error;
  }

  free( buffer );

  return 0;
}

/* Length of the BASIC part before the loader code */
#define basic_length 224-87+12-1+41+41	/* == 230 == 0xe6 */

static const libspectrum_byte SpectrumBASICData[ basic_length + 1 ] = {

	/* 0 {INK 255}{PAPER 255}BORDER PI-PI : PAPER PI-PI : INK PI-PI : CLS
		PRINT "{AT 6,0}{INK 5}
		{AT 12,9}{INK 6}{PAPER 2}{FLASH 1} NOW LOADING {AT 0,0}{FLASH 0}{PAPER 0}{INK 0}":
		RANDOMIZE USR (PEEK VAL "23627"+VAL "256"*PEEK VAL "23628") */
	"\x00\x00\xFF\xFF\x10\xFF\x11\xFF\xE7\xA7\x2D\xA7\x3A\xDA\xA7\x2D\xA7\x3A\xD9\xA7\x2D\xA7\x3A\xFB\x3A"
	"\xF5\"\x16\x06\x00\x10\x05""                                "
	"\x16\x0B\x0A\x10\x06\x11\x02\x12\x01 IS LOADING \x16\x00\x00\x12\x00\x11\x00\x10\x00\""
	"\x3A\xF5\"\x16\x13\x00\x10\x04""                                ""\""
	"\x3A\xF5\"\x16\x15\x00\x10\x04""                                ""\""
	"\x3A\xF9\xC0(\xBE\xB0\"23627\"+\xB0\"256\"*\xBE\xB0\"23628\")\x0D"

	/* Variables area - the 768 byte loader block is	appended after this piece */
	"\xCD\x52\x00"         /* VARSTART  CALL 0052,RET_INSTR   ; Determine on which address we are now             */
	"\x3B"                 /* RETRET    DEC  SP                                                                   */
	"\x3B"                 /*           DEC  SP                                                                   */
	"\xE1"                 /*           POP  HL                                                                   */
	"\x01\x12\x00"         /*           LD   BC,0012                                                              */
	"\x09"                 /*           ADD  HL,BC            ; Find start of the 768 byte loader                 */
	"\x11\x00\xBD"         /*           LD   DE,+BD00                                                             */
	"\x01\x00\x03"         /*           LD   BC,+0300                                                             */
	"\xED\xB0"             /*           LDIR                  ; Move it into place                                */
	"\xC3\x00\xBD" };      /*           JP   BD00,LDSTART     ; And run it!                                       */

/* The offsets into SpectrumBASICData where the user-settable strings
   are stored */
static const size_t basic_game_name_offset = 32;
static const size_t basic_info1_offset = 103;
static const size_t basic_info2_offset = 144;

/**********************************************************************************************************************************/
/* The custom loader. Any speed can be handled by it.                                                                             */
/* Notice that the EAR bit value is NOT rotated as opposed to the ROM and is kept at 0x40!                                        */
/* ROM speed: a = 22, b = 25 (bd = 27) => delay = 32*22 = 704, sampler = 59*25 = 1475 => delay/sampler = 1/2                      */
/* Max speed: a = 1, b = 0 (bd = 2) => 279 + 32 = 311 (hb0 = 156 T) => 468 average bit => 7479 bps                                */
/* (see below) The pilot pulse must be between 1453 + 16a T and 3130 + 16a T (a=known)                                            */
/* (see below) The first sync pulse must be 840 + 16a T maximum (a=known)                                                         */
/* Average bit length = 3,500,000 / speed : (a=x, b=2x - so delay/sampler is kept at 1/2)                                         */
/*  1364 bps => 2566 T => hb0 = 855 T, hb1 = 1710 T, avg = 1283 (2565) T       <-- ROM speed                                      */
/*              279 + 32a + 43b = 2565 => 32x + 86x = 2286 => 118x = 2286 => x = 19.4                                             */
/*              279 + 32*20 + 43*39 = 2596 (2565 needed), so a = 20, b = 39, bd = 41                                              */
/*              PilotMin = 1453 + 16a = 1453 + 16*20 = 1773 T                                                                     */
/*              PilotMax = 3130 + 16a = 3130 + 16*20 = 3450 T => 2168 T                                                           */
/*              Sync0 = 840 + 16a = 840 + 16*20 = 1160 T => 667 T                                                                 */
/*  2250 bps => 1556 T => hb0 = 518 T, hb1 = 1036 T, avg = 777 (1554) T                                                           */
/*              279 + 32a + 43b = 1554 => 32x + 86x = 1275 => 118x = 1275 => x = 10.8                                             */
/*              279 + 32*11 + 43*22 = 1577 (1554 needed), so a = 11, b = 22, bd = 24                                              */
/*              PilotMin = 1453 + 16a = 1453 + 16*11 = 1629 T                                                                     */
/*              PilotMax = 3130 + 16a = 3130 + 16*11 = 3306 T => 2000 T                                                           */
/*              Sync0 = 840 + 16a = 840 + 16*11 = 1016 T => 600 T                                                                 */
/*  3000 bps => 1167 T => hb0 = 389 T, hb1 = 778 T, avg = 584 (1167) T                                                            */
/*              279 + 32a + 43b = 1167 => 32x + 86x = 888 => 118x = 888 => x = 7.5                                                */
/*              279 + 32*7 + 43*16 = 1191 (1167 needed), so a = 7, b = 16, bd = 18                                                */
/*              PilotMin = 1453 + 16a = 1453 + 16*7 = 1565 T                                                                      */
/*              PilotMax = 3130 + 16a = 3130 + 16*7 = 3242 T => 1900 T                                                            */
/*              Sync0 = 840 + 16a = 840 + 16*7 = 952 T => 550 T                                                                   */
/*  6000 bps => 584 T => hb0 = 195 T, hb1 = 390 T, avg = 293 (585) T                                                              */
/*              279 + 32a + 43b = 585 => 32x + 86x = 306 => 118x = 306 => x = 2.6                                                 */
/*              279 + 32*3 + 43*5 = 590 (585 needed), so a = 3, b = 5, bd = 7                                                     */
/*              PilotMin = 1453 + 16a = 1453 + 16*3 = 1501 T                                                                      */
/*              PilotMax = 3130 + 16a = 3130 + 16*3 = 3178 T => 1700 T                                                            */
/*              Sync0 = 840 + 16a = 840 + 16*3 = 888 T => 450 T                                                                   */
/**********************************************************************************************************************************/

typedef struct turbo_variables_t {

  /* Variables actually used in the loader */

  libspectrum_byte compare;	/* Variable 'bd' + starting value (+80) */
  libspectrum_byte delay;	/* Variable 'a' */

  /* Variables used in the TZX header */
  libspectrum_dword pilot_length;	/* length of a pilot pulse. Number
					   of pulses is calculated to make
					   the pilot 1 second long */
  libspectrum_dword sync_length;	/* both equal */
  libspectrum_dword bit_length;		/* reset length; set length is
					   twice this */

} turbo_variables_t;

static const turbo_variables_t turbo_variables[] = {

  { 0x80 + 41, 20, 2168, 667, 855 },	/* ROM values => 1364 bps */
  { 0x80 + 24, 11, 2000, 600, 518 },	/* 2250 bps */
  { 0x80 + 18,  7, 1900, 550, 389 },	/* 3000 bps */
  { 0x80 +  7,  3, 1700, 450, 200 },	/* 6000 bps */

};

#define turbo_loader_length 144

/* 'compare'    is at offset 94
   'delay'      is at offset 118
   'xor colour' is at offset 134 */

static const libspectrum_byte turbo_loader[ turbo_loader_length + 1 ] = 
  { "\xCD\x5B\xBF"        /* LD_BLOCK  CALL LD_BYTES         */
    "\xD8"                /*           RET  C                */
    "\xCF"                /*           RST  0008,ERROR_1     */
    "\x1A"                /*           DEFB +1A ; R Tape Loading Error */
    "\x14"                /* LD_BYTES  INC  D                */
    "\x08"                /*           EX   AF,AF'           */
    "\x15"                /*           DEC  D                */
    "\x3E\x08"            /*           LD   A,+08            (Border BLACK + MIC off) */
    "\xD3\xFE"            /*           OUT  (+FE),A          */
    "\xDB\xFE"            /*           IN   A,(+FE)          */
    "\xE6\x40"            /*           AND  +40              */
    "\x4F"                /*           LD   C,A              (BLACK/chosen colour) */
    "\xBF"                /*           CP   A                */
    "\xC0"                /* LD_BREAK  RET  NZ               */
    "\xCD\xCA\xBF"        /* LD_START  CALL LD_EDGE_1        */
    "\x30\xFA"            /*           JR   NC,LD_BREAK      */
    "\x26\x00"            /*           LD   H,+80            */

    /* Each subsequent 2 edges take:   65 + LD_EDGE_2 T = 283 + 32a + 43b T, bd = b + 2                      */
    /* Minimum:                                                                                              */
    /*   ROM => b = (+C6 - +9C - 2) + 1 = 41, a=22 => 427 + 32a + 59b T => 427 + 32*22 + 59*41 T = 3550 T    */
    /*          283 + 32a + 43b = 3432, a=20 => 283 + 32*20 + 43b = 3550 => 43b = 2627 => b = 61, bd = 63    */
    /*   b = 61, so each 2 leader pulses must fall within:                                                   */
    /*   283 + 32a + 43*61 T = 2906 + 32a T                                                                  */
    /*   One leader pulse must therefore be (2906 + 32a) / 2 = 1453 + 16a T minimum                          */
    /* Maximum, at timeout:                                                                                  */
    /*   ROM => b = +100 - +9C - 2 = 98, a=22 => 427 + 32a + 59b T => 427 + 32*22 + 59*98 T = 6913 T         */
    /*          283 + 32a + 43b = 6913, a=20 => 283 + 32*20 + 43b = 6913 => 43b = 6002 => b = 139, bd = 141  */
    /*   b = 139, so each 2 leader pulses must fall within:                                                  */
    /*   283 + 32a + 43*139 T = 6260 + 32a T                                                                 */
    /*   One leader pulse must therefore be (6260 + 32a) / 2 = 3130 + 16a T maximum                          */
    /* Notice that, as opposed to the ROM, we don't need the leader to be over a second. Only 256 pulses are */
    /* required (128 * 2 edges, counted in H).                                                               */

    "\x06\x75"            /* LD_LEADER LD   B,+73            7       */
    "\xCD\xC6\xBF"        /*           CALL LD_EDGE_2        17      */
    "\x30\xF1"            /*           JR   NC,LD_BREAK      7 (/12) */
    "\x3E\xB0"            /*           LD   A,+B2            7       */
    "\xB8"                /*           CP   B                4       */
    "\x30\xED"            /*           JR   NC,LD_START      7 (/12) */
    "\x24"                /*           INC  H                4       */
    "\x20\xF1"            /*           JR   NZ,LD_LEADER     7/12    */

    /* Each subsequent edge takes:   54 + LD_EDGE_1 T = 152 + 16a + 43b T, bd = b + 1                        */
    /* Sync0 maximum:                                                                                        */
    /*   ROM => b = +D4 - +C9 - 1 = 10, a=22 => 224 + 16a + 59b T => 224 + 16*22 + 59*10 T = 1166 T          */
    /*          152 + 16a + 43b = 1166, a=20 => 152 + 16*20 + 43b = 1166 => 43b = 694 => b = 16, bd = 17     */
    /*   b = 16, so the (first) sync pulse must be less than:                                                */
    /*   152 + 16a + 43*16 T = 840 + 16a T                                                                   */
    /* Leader maximum (at timeout):                                                                          */
    /*   ROM => b = +100 - +C9 - 1 = 56, a=22 => 224 + 16a + 59b T => 224 + 16*22 + 59*56 T = 3880 T         */
    /*          152 + 16a + 43b = 3880, a=20 => 152 + 16*20 + 43b = 3880 => 43b = 3408 => b = 79, bd = 80    */
    /*   Notice that this leader maximum is higher than above, so is not needed to be considered             */

    "\x06\xB0"            /* LD_SYNC   LD   B,+B0            7       */
    "\xCD\xCA\xBF"        /*           CALL LD_EDGE_1        17      */
    "\x30\xE2"            /*           JR   NC,LD_BREAK      7 (/12) */
    "\x78"                /*           LD   A,B              4       */
    "\xFE\xC1"            /*           CP   +C1              7       */
    "\x30\xF4"            /*           JR   NC,LD_SYNC       7/12    */

    "\xCD\xCA\xBF"        /*           CALL LD_EDGE_1        17      */
    "\xD0"                /*           RET  NC               5 (/11) */
    "\x26\x00"            /*           LD   H,+00            7       */
    "\x06\x80"            /*           LD   B,+80            7       */
    "\x18\x17"            /*           JR   LD_MARKER        12   = 48S */

    "\x08"                /* LD_LOOP   EX   AF,AF'           4       */
    "\x20\x05"            /*           JR   NZ,LD_FLAG       7/12 = 16F */
    "\xDD\x75\x00"        /*           LD   (IX+00),L        19      */
    "\x18\x09"            /*           JR   LD_NEXT          12   = 42D */

    "\xCB\x11"            /* LD_FLAG   RL   C                4       */
    "\xAD"                /*           XOR  L                4       */
    "\xC0"                /*           RET  NZ               5 (/11) */
    "\x79"                /*           LD   A,C              4       */
    "\x1F"                /*           RRA                   4       */
    "\x4F"                /*           LD   C,A              4       */
    "\x18\x03"            /*           JR   LD_FLNEXT        12   = 37F */

    "\xDD\x2B"            /* LD_NEXT   DEC  IX               10      (We're loading BACKWARD!) */
    "\x1B"                /*           DEC  DE               6       */
    "\x08"                /* LD_FLNEXT EX   AF,AF'           4       */
    "\x06\x82"            /*           LD   B,+82            7       */
    "\x2E\x01"            /* LD_MARKER LD   L,+01            7    = 34D/18F/7S */

    /* The very first (flag) bit takes (S) = 48 + 7 = 55 + LD_EDGE_2 T = 273 + 32a + 43b T                       */
    /* (D) 55 + 32 + 42 + 34 = 163, so                                                                           */
    /* Each first (data) bit takes: 163 + LD_EDGE_2 - 2b T = 295 + 32a + 43b T (ROM = 405 + 32a + 59b T)         */
    /* Each subsequent bit takes:   60 + LD_EDGE_2       T = 278 + 32a + 43b T (ROM = 422 + 32a + 59b T)         */

    "\xCD\xC6\xBF"        /* LD_8_BITS CALL LD_EDGE_2        17      */
    "\xD0"                /*           RET  NC               5 (/11) */
    "\x3E"                /*           LD   A,COMPARE        7    (variable 'bd') */

    "\x00"				/* COMPARE !!! */

    "\xB8"                /*           CP   B                4       */
    "\xCB\x15"            /*           RL   L                8       */
    "\x06\x80"            /*           LD   B,+80            7       */
    "\x30\xF3"            /*           JR   NC,LD_8_BITS     7/12 = 55D/60N */

    "\x7C"                /*           LD   A,H              4       */
    "\xAD"                /*           XOR  L                4       */
    "\x67"                /*           LD   H,A              4       */
    "\x7A"                /*           LD   A,D              4       */
    "\xB3"                /*           OR   E                4       */
    "\x20\xD3"            /*           JR   NZ,LD_LOOP       (7/) 12 = 32D */
    "\x7C"                /*           LD   A,H                      */
    "\xFE\x01"            /*           CP   +01                      */
    "\xC9"                /*           RET                           */

    /* The LD_EDGE_2 routine takes:  22 + 2 * (98 + 16a) + 43b T = 218 + 32a + 43b T (ROM = 362 + 32a + 59b T), bd = b + 2 */

    "\xCD\xCA\xBF"        /* LD_EDGE_2 CALL LD_EDGE_1        17      */
    "\xD0"                /*           RET  NC               5 (/11) */

    /* The LD_EDGE_1 routine takes:  98 + 16a + 43b T (ROM = 170 + 16a + 59b T, a=22), bd = b + 1 */

    "\x3E"                /* LD_EDGE_1 LD   A,DELAY          7    (variable 'a') */

    "\x00" /* DELAY !!! */

    "\x3D"                /* LD_DELAY  DEC  A                4       */
    "\x20\xFD"            /*           JR   NZ,LD_DELAY      7/12    */
    "\xA7"                /*           AND  A                4       */

    "\x04"                /* LD_SAMPLE INC  B                4    (variable 'b') */
    "\xC8"                /*           RET  Z                5 (/11) */
    "\xDB\xFE"            /*           IN   A,(+FE)          11      */
    "\xA9"                /*           XOR  C                4       */
    "\xE6\x40"            /*           AND  +40              7       */
    "\x28\xF7"            /*           JR   Z,LD_SAMPLE      7/12 = 43 */

    "\x79"                /*           LD   A,C              4       */
    "\xEE"                /*           XOR  COLOUR           7       */

    "\x00" /* XOR COLOUR !!! */

    "\x4F"                /*           LD   C,A              4       */
    "\xE6\x07"            /*           AND  +07              7       */
    "\xF6\x08"            /*           OR   +08              7       */
    "\xD3\xFE"            /*           OUT  (+FE),A          11      */
    "\x37"                /*           SCF                   4       */
    "\xC9" };            /*           RET                   10      */

typedef struct page_t     /* Each page as found in a Z80 file */
{
  libspectrum_byte page;	/* Spectrum value, not value from .z80 file */
  libspectrum_word address;     /* Normal (non-paged) start address of this
				   16Kb chunk */
} page_t;

static const page_t PageOrder48S[4] = {

  { 255, 0xc000 },  /* external loading screen */
  {   2, 0x8000 },
  {   0, 0xc000 },
  {   5, 0x4000 }   /* must be decompressed */

};

static const page_t PageOrder128S[9] = {

  { 255, 0xc000 },	/* external loading screen */
  {   2, 0x8000 },
  {   1, 0xc000 },
  {   3, 0xc000 },
  {   4, 0xc000 },
  {   6, 0xc000 },
  {   7, 0xc000 },
  {   0, 0xc000 },
  {   5, 0x4000 }	/* must be decompressed */
};

static const page_t PageOrder48N[3] = {

  {   5, 0xc000 },	/* loading screen from memory */
  {   2, 0x8000 },
  {   0, 0xc000 }

};

static const page_t PageOrder128N[8] = {

  {   5, 0xc000 },	/* loading screen from memory */
  {   2, 0x8000 },
  {   1, 0xc000 },
  {   3, 0xc000 },
  {   4, 0xc000 },
  {   6, 0xc000 },
  {   7, 0xc000 },
  {   0, 0xc000 }

};

static const size_t loader_length = 0x300;

static const libspectrum_byte loader_data[ 0x300 ] =
{0xF3,0x31,0x00,0xC0,0xCD,0x91,0xBE,0xDD,0x21,0x6C,0xBE,0x21,0xBB,0xBB,0x11
,0xCC,0xCC,0x01,0xDD,0xDD,0xD9,0xFD,0x21,0xFF,0xFF,0xDD,0x7E,0x00,0xA7,0x28
,0x50,0xDD,0x66,0x01,0x2E,0xFF,0xDD,0x5E,0x02,0xDD,0x56,0x03,0x01,0xFD,0x7F
,0xED,0x79,0x01,0x04,0x00,0xDD,0x09,0xDD,0xE5,0xD5,0x7A,0xB3,0x20,0x0D,0x11
,0x00,0x40,0xDD,0x7E,0xFC,0xFE,0x12,0x20,0x03,0x11,0x00,0x3D,0xE5,0xDD,0xE1
,0x3E,0xAA,0x37,0xCD,0x55,0xBF,0xC1,0xDD,0xE5,0xE1,0x23,0xCD,0x44,0xBE,0x11
,0x01,0x00,0xFD,0x19,0x30,0x0B,0x21,0x00,0xC0,0x11,0x00,0x40,0x01,0x00,0x40
,0xED,0xB0,0xDD,0xE1,0x18,0xAA,0xFD,0x21,0xAA,0xAA,0x21,0xE0,0x5A,0x06,0x20
,0x3E,0xFF,0x77,0x23,0x10,0xFC,0x21,0xC8,0xBD,0x11,0xE0,0x57,0x0E,0x20,0xED
,0xB0,0x11,0xE0,0x56,0x0E,0x1F,0xED,0xB0,0x11,0xE0,0x55,0x0E,0x09,0xED,0xB0
,0x11,0xE0,0x54,0x0E,0x20,0xED,0xB0,0x11,0xE0,0x53,0x0E,0x14,0xED,0xB0,0x3E
,0xCC,0x3C,0x20,0x11,0x31,0xEA,0x52,0xDD,0x21,0x00,0xBD,0x11,0x00,0x03,0x3E
,0x55,0xBC,0x08,0xC3,0xE0,0x57,0x21,0x00,0xBD,0x11,0x01,0xBD,0x01,0xFF,0x02
,0x36,0x00,0xC3,0xE0,0x57,0xCD,0x62,0x05,0x31,0xE0,0x54,0x16,0x10,0x01,0xFD
,0xFF,0x15,0xED,0x51,0xF1,0x06,0xBF,0xED,0x79,0x7A,0xA7,0x20,0xF1,0x06,0xFF
,0x3E,0xBB,0xED,0x79,0xC3,0xE3,0x56,0xCD,0x62,0x05,0x31,0xE0,0x53,0xF1,0xD3
,0xFE,0xC1,0xF1,0xED,0x79,0xDD,0xE1,0xF1,0x08,0xE1,0x7C,0xED,0x47,0x7D,0xED
,0x4F,0xE1,0xD1,0xC1,0xF1,0xC3,0xE0,0x55,0x31,0xBB,0xAA,0xED,0x56,0xF3,0xC3
,0xDD,0xCC,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x00,0x01,0xFD,0x7F,0x00,0x02,0x03,0x04,0x05,0x06,0x07
,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10,0x7C,0xE6,0xC0,0x57,0x1E,0x00
,0x78,0xB1,0xC8,0x7E,0xFE,0xED,0x20,0x05,0x23,0xBE,0x28,0x05,0x2B,0xED,0xA0
,0x18,0xEF,0x23,0xC5,0x46,0x23,0x7E,0x23,0x12,0x13,0x10,0xFC,0xC1,0x0B,0x0B
,0x0B,0x0B,0x18,0xDE,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x21,0xFE,0x5A,0x36
,0x0D,0x23,0x36,0x0D,0x06,0x08,0x21,0xB4,0xBE,0xDD,0x21,0xFE,0x50,0x56,0xDD
,0x72,0x00,0x23,0x56,0xDD,0x72,0x01,0x23,0x11,0x00,0x01,0xDD,0x19,0x10,0xEF
,0xC9,0x00,0x00,0x7F,0xFE,0x48,0x02,0x08,0xF8,0x08,0x80,0x08,0x82,0x3E,0xFE
,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
,0x00,0x00,0x00};
/* end binary data. size = 768 bytes */

static libspectrum_byte
calc_checksum( libspectrum_byte *data, size_t length )
{
  int i;
  libspectrum_byte checksum;

  for( i=0, checksum = 0; i < length; i++, data++ ) checksum ^= *data;

  return checksum;
}

static int
add_rom_block( libspectrum_tape *tape, const libspectrum_byte flag,
	       const libspectrum_byte *data, const size_t length )
{
  libspectrum_byte *buffer;
  libspectrum_tape_block *block;
  buffer = malloc( ( length + 2 ) * sizeof( libspectrum_byte ) );
  if( !buffer ) {
    print_error( "out of memory at %s:%d", __FILE__, __LINE__ );
    return 1;
  }

  block = libspectrum_tape_block_alloc( LIBSPECTRUM_TAPE_BLOCK_ROM );

  libspectrum_tape_block_set_pause( block, 100 );
  libspectrum_tape_block_set_data_length( block, length + 2 );
  libspectrum_tape_block_set_data( block, buffer );

  buffer[0] = flag;
  memcpy( buffer + 1, data, length );
  buffer[ length + 1 ] = calc_checksum( buffer, length + 1 );

  libspectrum_tape_append_block( tape, block );

  return 0;
}

static int
create_main_header( libspectrum_tape *tape, const char *loader_name )
{
  libspectrum_byte header[17], *ptr;
  size_t i, length;
  int error;

  ptr = header;

  *ptr++ = '\0';		/* BASIC program */

  /* Filename */
  *ptr++ = 0x11; *ptr++ = 0x05;	/* PAPER 5 */

  length = strlen( loader_name );
  /* Character 0x08 is cursor left */
  for( i = 0; i < 8; i++ ) *ptr++ = ( i <= length ? loader_name[i] : 0x08 );

  /* Length of data block */
  length = basic_length + loader_length;
  *ptr++ = length & 0xff; *ptr++ = length >> 8;

  /* Autostart line number */
  *ptr++ = '\0'; *ptr++ = '\0';

  /* Start of the variables area */
  length = basic_length - 21;
  *ptr++ = length & 0xff; *ptr++ = length >> 8;

  error = add_rom_block( tape, 0x00, header, 17 ); if( error ) return error;

  return 0;
}

static void
set_loader_speed( libspectrum_byte *buffer, int speed,
		  libspectrum_byte border_colour )
{
  libspectrum_byte xor_colour;
  const turbo_variables_t *ptr;

  ptr = &turbo_variables[ speed ];

  buffer[ 94] = ptr->compare;
  buffer[118] = ptr->delay;

  /* In general, use black/<final border colour> as the colours for the
     loading stripes, unless the final colour is black, in which case we
     (somewhat arbitrarily) use black/blue */
  xor_colour = ( border_colour == 0 ? 0x41 : 0x40 | border_colour );

  buffer[134] = xor_colour;
}

static const size_t table_offset = 0x0253;

static const size_t extra_block_flag_offset = 0x018c;
static const size_t extra_block_clean1_offset = 0x01af;
static const size_t extra_block_clean2_offset = 0x01cf;

#define POS_HL2		0x00C		/*2 H'L' */
#define POS_DE2		0x00F		/*2 D'E' */
#define POS_BC2		0x012		/*2 B'C' */
#define POS_IY		0x071		/*2 IY */
#define POS_LAST_ATTR	0x079		/*  Last Line Attribute value */
#define POS_48K_1	0x0B9		/*  0x56 = 48k , 0x57 = 128k */
#define POS_48K_2	0x0C7		/*  as POS_48K_1 */
#define POS_LAST_AY	0x0E2		/*  Last AY out byte */
#define POS_SP		0x108		/*2 SP */
#define POS_IM		0x10B		/*  IM: 0=0x46 , 1=0x56 , 2=0x5E */
#define POS_DIEI	0x10C		/*  DI=0xF3 , EI=0xFB */
#define POS_PC		0x10E		/*2 PC */
#define POS_AYREG	0x110		/*2 16 AY registers, 1st byte = 0x00 */
#define POS_BORDER	0x131		/*  Border Colour */
#define POS_PAGE	0x135		/*  Current 128 page (0x10 if 48k) */
#define POS_IX		0x136		/*2 IX */
#define POS_F2		0x138		/*  F' */
#define POS_A2		0x139		/*  A' */
#define POS_R		0x13A		/*  R */
#define POS_I		0x13B		/*  I */
#define POS_HL		0x13C		/*2 HL */
#define POS_DE		0x13E		/*2 DE */
#define POS_BC		0x140		/*2 BC */
#define POS_F		0x142		/*  F */
#define POS_A		0x143		/*  A */

static void
write_word( libspectrum_byte *dest, libspectrum_word src )
{
  *dest = src & 0xff; *(dest+1) = src >> 8;
}

static int
snap_mode_128( libspectrum_snap *snap )
{
  libspectrum_machine machine;
  int capabilities;

  machine = libspectrum_snap_machine( snap );
  capabilities = libspectrum_machine_capabilities( machine );

  return( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_128_MEMORY );
}

static int
create_loader( libspectrum_byte *buffer, libspectrum_snap *snap,
	       const settings_t *settings )
{
  int border_colour;
  size_t i, ppay;
  libspectrum_byte machine, attributes;

  border_colour = settings->load_colour;

  if( border_colour == -1 )
    border_colour = libspectrum_snap_out_ula( snap ) & 0x07;

  /* Place the loader data into the buffer */
  memcpy( buffer, loader_data, loader_length );

  /* And then put the turbo loader code itself in */
  memcpy( buffer + 341 + 256, turbo_loader, turbo_loader_length );

  /* Fill the loader with appropriate speed values and colour */
  set_loader_speed( buffer + 341 + 256, settings->speed, border_colour );

  /* Set the registers etc */

  write_word( buffer + POS_HL2, libspectrum_snap_hl_( snap ) );
  write_word( buffer + POS_DE2, libspectrum_snap_de_( snap ) );
  write_word( buffer + POS_BC2, libspectrum_snap_bc_( snap ) );
  write_word( buffer + POS_IY, libspectrum_snap_iy( snap ) );

  attributes = border_colour | ( border_colour << 3 ) | settings->bright;
  buffer[ POS_LAST_ATTR ] = attributes;

  machine = snap_mode_128( snap ) ? 0x57 : 0x56;
  buffer[ POS_48K_1 ] = buffer[ POS_48K_2 ] = machine;

  buffer[ POS_LAST_AY ] = libspectrum_snap_out_ay_registerport( snap );

  write_word( buffer + POS_SP, libspectrum_snap_sp( snap ) );

  switch( libspectrum_snap_im( snap ) ) {
  case 0: buffer[ POS_IM ] = 0x46; break;
  case 1: buffer[ POS_IM ] = 0x56; break;
  case 2: buffer[ POS_IM ] = 0x5E; break;
  }

  buffer[ POS_DIEI ] = libspectrum_snap_iff1( snap ) ? 0xfb : 0xf3;

  write_word( buffer + POS_PC, libspectrum_snap_pc( snap ) );

  buffer[ POS_BORDER ] = border_colour;

  buffer[ POS_PAGE ] = snap_mode_128( snap ) ?
    libspectrum_snap_out_128_memoryport( snap ) :
    0x10;

  write_word( buffer + POS_IX, libspectrum_snap_ix( snap ) );

  buffer[ POS_F2 ] = libspectrum_snap_f_( snap );
  buffer[ POS_A2 ] = libspectrum_snap_a_( snap );

  buffer[ POS_R ] = libspectrum_snap_r( snap ) - 0x0a;
  buffer[ POS_I ] = libspectrum_snap_i( snap );

  write_word( buffer + POS_HL, libspectrum_snap_hl( snap ) );
  write_word( buffer + POS_DE, libspectrum_snap_de( snap ) );
  write_word( buffer + POS_BC, libspectrum_snap_bc( snap ) );

  buffer[ POS_F ] = libspectrum_snap_f( snap );
  buffer[ POS_A ] = libspectrum_snap_a( snap );

  for( i=0, ppay = POS_AYREG + 1; i < 16; i++, ppay += 2 )
    buffer[ ppay ] = libspectrum_snap_ay_registers( snap, 15 - i );

  return 0;
}

static int
add_loader_block( libspectrum_tape *tape, libspectrum_byte **loader,
		  libspectrum_snap *snap, const settings_t *settings )
{
  libspectrum_tape_block *block;
  libspectrum_byte *basic;
  size_t length;
  int error2;

  length = basic_length + loader_length + 2;

  *loader = malloc( length * sizeof( libspectrum_byte ) );
  if( !(*loader) ) {
    print_error( "out of memory at %s:%d", __FILE__, __LINE__ );
    return 1;
  }

  block = libspectrum_tape_block_alloc( LIBSPECTRUM_TAPE_BLOCK_ROM );

  libspectrum_tape_block_set_pause( block, 100 );
  libspectrum_tape_block_set_data_length( block, length );
  libspectrum_tape_block_set_data( block, (*loader) );

  (*loader)[0] = 0xff;		/* Data block */

  /* The BASIC data starts just after the flag byte */
  basic = *loader + 1;

  /* Put the BASIC code into the buffer */
  memcpy( basic, SpectrumBASICData, basic_length );

  /* Copy the game name and info lines into the BASIC code */
  memcpy( basic + basic_game_name_offset, settings->game_name, 32 );
  memcpy( basic + basic_info1_offset, settings->info1, 32 );
  memcpy( basic + basic_info2_offset, settings->info2, 32 );

  /* Put the loader into the buffer */
  error2 = create_loader( basic + basic_length, snap, settings );
  if( error2 ) { libspectrum_tape_block_free( block ); return error2; }
  
  /* We don't calculate the checksum yet as we need to do it after
     we've put the page lengths in the data */

  /* But put the block into the tape anyway */
  libspectrum_tape_append_block( tape, block );

  return 0;
}

static int
create_turbo_header( libspectrum_tape_block *block, int speed )
{
  const turbo_variables_t *ptr;
  libspectrum_dword pilot_length;

  ptr = &turbo_variables[ speed ];

  pilot_length = ptr->pilot_length;

  libspectrum_tape_block_set_pilot_length( block, pilot_length );
  libspectrum_tape_block_set_pilot_pulses( block, 3500000UL / pilot_length );
  libspectrum_tape_block_set_sync1_length( block, ptr->sync_length );
  libspectrum_tape_block_set_sync2_length( block, ptr->sync_length );
  libspectrum_tape_block_set_bit0_length( block, ptr->bit_length );
  libspectrum_tape_block_set_bit1_length( block, ptr->bit_length * 2 );
  libspectrum_tape_block_set_bits_in_last_byte( block, 8 );
  libspectrum_tape_block_set_pause( block, 100 );

  return 0;
}

static int
get_loading_screen( libspectrum_byte *screen, const char *filename )
{
  FILE *f;
  size_t count;

  f = fopen( filename, "rb" );
  if( !f ) {
    print_error( "error opening loading screen '%s': %s", filename,
		 strerror( errno ) );
    return 1;
  }

  count = fread( screen, 1, 6912, f );
  if( count != 6912 ) {
    if( count == -1 ) {
      print_error( "error reading from '%s: %s", filename, strerror( errno ) );
    } else {
      print_error( "could read only %lu bytes from '%s'", (unsigned long)count,
		   filename );
    }
    return 1;
  }

  if( fclose( f ) ) {
    print_error( "error closing '%s': %s", filename, strerror( errno ) );
    return 1;
  }

  return 0;
}

static int
add_page( libspectrum_tape *tape, libspectrum_snap *snap, int page,
	  libspectrum_word address, libspectrum_byte *loader,
	  size_t *loader_table_entry, const settings_t *settings )
{
  libspectrum_tape_block *block;
  libspectrum_error error;
  size_t page_length, compressed_length, reverse_offset;
  libspectrum_byte table_byte, *buffer;

  block = libspectrum_tape_block_alloc( LIBSPECTRUM_TAPE_BLOCK_TURBO );

  error = create_turbo_header( block, settings->speed ); if( error ) {
    libspectrum_tape_block_free( block );
    return error;
  }

  reverse_offset = 0;

  if( page == 255 ) {

    /* External loading screen */

    libspectrum_byte screen[ 0x1b00 ];

    error = get_loading_screen( screen, settings->external_filename );
    if( error ) { libspectrum_tape_block_free( block ); return error; }

    page_length = 0x1b00;

    crunch_z80( screen, page_length, WorkBuffer, &compressed_length );

    table_byte = 0x10;

    /* compressed_length == 0 => block could not be compressed */
    if( !compressed_length ) {
      print_verbose(
        "- adding separate loading screen (uncompressed) length: %04x\n",
	page_length
      );
    } else {
      print_verbose(
        "- adding separate loading screen   (compressed) length: %04x\n",
	compressed_length
      );
    }

  } else {

    libspectrum_byte *data; 
    int empty;
    size_t i;
    
    /* If this page wasn't in the snapshot, just move on to the next one */
    data = libspectrum_snap_pages( snap, page );
    if( !data ) return 0;

    page_length = 16384;

    if( snap_mode_128( snap ) ) {

      if( !(*loader_table_entry) ) {
	table_byte = 0x10;
      } else {
	table_byte = page | 0x10;
      }

    } else {
      table_byte = 0x10;
    }			
				
    /* Have to special case the page at 0x8000 as it contains the loader */
    if( address == 0x8000 ) {
      page_length -= loader_length;
      table_byte = 0x12;
    }

    for( empty = 1, i = 0; i < page_length; i++ )
      if( data[i] ) { empty = 0; break; }
    if( empty ) return 0;

    /* Now crunch the block */
    crunch_z80( data, page_length, WorkBuffer, &compressed_length );

    if( settings->external_filename && page == 5 ) {

      /* Check if external screen is loading and last page selected
	 If so then check if it would overwrite loading screen - if so
	 then load decrunched */
      if( compressed_length > page_length ) {
	reverse_block( WorkBuffer, data );
	compressed_length = 0;		/* Not compressed */
      }

    }

    if( compressed_length != 0 ) {

      /* Check if it would overwrite itself */
      if( test_rev_decz80( WorkBuffer, page_length, compressed_length ) ) {
	reverse_block( WorkBuffer, data );
	reverse_offset = 0x4000 - page_length;
	compressed_length = 0;	/* Not compressed */
      }

    }

    if( compressed_length == 0 ) {
      print_verbose(
        "- adding memory page %d           (uncompressed) length: %04x\n",
	page, page_length
      );
    } else {
      print_verbose(
	"- adding memory page %d             (compressed) length: %04x\n",
	page, compressed_length
      );
    }

  }

  /* Fill in the table data */
  address += page_length - 1;

  loader[ table_offset + ( *loader_table_entry * 4 )     ] = table_byte;
  loader[ table_offset + ( *loader_table_entry * 4 ) + 1 ] = address >> 8;
  write_word( &loader[ table_offset + ( *loader_table_entry * 4 ) + 2 ],
	      compressed_length );

  if( !compressed_length ) compressed_length = page_length;

  libspectrum_tape_block_set_data_length( block, compressed_length + 2 );

  buffer = malloc( ( compressed_length + 2 ) * sizeof( libspectrum_byte ) );
  if( !buffer ) {
    print_error( "out of memory at %s:%d", __FILE__, __LINE__ );
    libspectrum_tape_block_free( block );
    return 1;
  }

  buffer[0] = 0xaa;
  memcpy( &buffer[1], WorkBuffer + reverse_offset, compressed_length );
  buffer[ compressed_length + 1 ] =
    calc_checksum( buffer, compressed_length + 1 );

  libspectrum_tape_block_set_data( block, buffer );

  (*loader_table_entry)++;

  libspectrum_tape_append_block( tape, block );

  return 0;
}

/* Add a block to overwrite our loader code if needed */
static int
add_extra_block( libspectrum_tape *tape, libspectrum_snap *snap,
		 const page_t *page_order, size_t num_pages,
		 libspectrum_byte *loader )
{
  libspectrum_byte *page;
  int empty, extra_block_needed, error;
  size_t i, j;
    
  extra_block_needed = 0;

  for( i = 0; i < num_pages; i++ ) {

    if( page_order[i].address != 0x8000 ) continue;

    page = libspectrum_snap_pages( snap, page_order[i].page );

    for( empty = 1, j = 0x4000 - loader_length; j < 0x4000; j++ )
      if( page[j] ) { empty = 0; break; }

    /* No need to load the data in if it is all zero */
    if( empty ) break;

    print_verbose( "- adding extra ROM loading block\n" );
    loader[ extra_block_flag_offset ] = 0xff;
    extra_block_needed = 1;

    error = add_rom_block( tape, 0x55, page + 0x4000 - loader_length,
			   loader_length );
    if( error ) return error;
  } 

  if( !extra_block_needed ) {

    /* Unset the 768 load flag */
    loader[ extra_block_flag_offset ] = 0x00;

    /* Change the loader code to a simple LDIR */
    loader[ extra_block_clean1_offset     ] = 0xed;
    loader[ extra_block_clean1_offset + 1 ] = 0xb0;
    loader[ extra_block_clean1_offset + 2 ] = 0x00;
    loader[ extra_block_clean2_offset     ] = 0xed;
    loader[ extra_block_clean2_offset + 1 ] = 0xb0;
    loader[ extra_block_clean2_offset + 2 ] = 0x00;

  }

  return 0;
}

static int
create_main_data( libspectrum_tape *tape, libspectrum_snap *snap,
		  const settings_t *settings )
{
  int i, num_pages, error;
  const page_t *page_order;
  libspectrum_byte *loader;
  size_t loader_table_entry;

  error = add_loader_block( tape, &loader, snap, settings );
  if( error ) return error;

  if( snap_mode_128( snap ) ) {

    if( settings->external_filename ) {
      num_pages = 9;
      page_order = PageOrder128S;
    } else {
      num_pages = 8;
      page_order = PageOrder128N;
    }

  } else {
		
    if( settings->external_filename ) {
      num_pages = 4;
      page_order = PageOrder48S;
    } else {
      num_pages = 3;
      page_order = PageOrder48N;
    }

  }

  loader_table_entry = 0;

  for( i = 0; i < num_pages; i++ ) {
    error =
      add_page( tape, snap, page_order[i].page, page_order[i].address, loader,
		&loader_table_entry, settings );
    if( error ) return error;
  }

  error = add_extra_block( tape, snap, page_order, num_pages, loader );
  if( error ) return error;

  /* And finally, put in the checksum for the loader block */
  loader[ basic_length + loader_length + 1 ] =
    calc_checksum( loader, basic_length + loader_length + 1 );

  return 0;
}

static int
write_tape( libspectrum_tape *tape, const char *filename )
{
  libspectrum_byte *buffer;
  FILE *f;
  size_t length, written;
  int error;

  length = 0;

  error = libspectrum_tape_write( &buffer, &length, tape,
                                  LIBSPECTRUM_ID_TAPE_TZX );
  if( error ) return error;

  f = fopen( filename, "wb" );
  if( !f ) {
    print_error( "couldn't open output file '%s': %s", filename,
		 strerror( errno ) );
    free( buffer );
    return 1;
  }

  written = fwrite( buffer, 1, length, f );
  if( written != length ) {
    if( written == -1 ) {
      print_error( "error writing to '%s': %s", filename, strerror( errno ) );
    } else {
      print_error( "could write only %lu of %lu bytes to '%s'",
		   (unsigned long)written, (unsigned long)length, filename );
    }
    free( buffer );
    return 1;
  }

  free( buffer );

  if( fclose( f ) ) {
    print_error( "error closing '%s': %s", filename, strerror( errno ) );
    return 1;
  }

  return 0;
}  

static int
convert_snap( libspectrum_snap *snap, const settings_t *settings )
{
  libspectrum_tape *tape = libspectrum_tape_alloc();
  int error;

  print_verbose( "\nCreating TZX file:\n" );

  error = create_main_header( tape, settings->loader_name );
  if( error ) { libspectrum_tape_free( tape ); return error; }

  error = create_main_data( tape, snap, settings );
  if( error ) { libspectrum_tape_free( tape ); return error; }

  error = write_tape( tape, settings->output_filename );
  if( error ) { libspectrum_tape_free( tape ); return error; }

  libspectrum_tape_free( tape );

  return 0;
}

int
main( int argc, char **argv )
{
  settings_t settings;
  libspectrum_snap *snap;
  int error;

  progname = argv[0];

  error = init_libspectrum(); if( error ) return error;
  error = parse_args( &settings, argc, argv ); if( error ) return error;

  error = load_snap( &snap, settings.input_filename );
  if( error ) return error;

  error = convert_snap( snap, &settings );
  if( error ) { libspectrum_snap_free( snap ); return error; }

  libspectrum_snap_free( snap );

  print_verbose( "\nDone!\n" );

  return 0;
}
