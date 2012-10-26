/* fmfconv_aiff.c: aiff output routine included into fmfconv.c
   Copyright (c) 2010 Gergely Szasz

   $Id: $

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

   Author contact information:

   E-mail: szaszg@hu.inter.net

*/
#include <config.h>

#include <stdio.h>
#include <stdlib.h>

#include "libspectrum.h"
#include "fmfconv.h"


/*
Byte order: Big-endian

Offset   Length   Contents
  0      4 bytes  "FORM"
  4      4 bytes  <File length - 8>
  8      4 bytes  "AIFF"

 12      4 bytes  "COMM"
 16      4 bytes  <Length of the COMM data> // (=18)
 20      2 bytes  <Channels>     // Channels: 1 = mono, 2 = stereo
 22      4 bytes  <No of frames>
 26      2 bytes  <bits/sample>  // 8 or 16
 36     10 bytes  <Frame rate>  // Frameses per second: e.g., 44100
.................................................... PCM
 38      4 bytes  "SSND"
 42      4 bytes  <No of bytes - 46>
 46      4 bytes  <Offset>	// (=0)
 50      4 bytes  <Block>	// (=0)
 54     (n)bytes  <Sample data>
..................................................... Law!!!


AIFF 80bit FLOAT

if exp = 16383 --> *2^0
[sign1][exp15][1][mant63]
0 exp-16383
*/
#define AIFF_POS_FILELEN  4L
#define AIFF_POS_NOFRAME 22L
#define AIFF_POS_SSNDLEN 42L

#define AIFF_POS_SSNCLEN 62L

int force_aifc = 0;			/* record aifc file even PCM sound */
static int aifc = 0;

/* save float to head[0..9] */
static void
aiff_int_2_80bitfloat( libspectrum_byte head[], libspectrum_word num )
{
  uint64_t frac = num;
  uint64_t max = (uint64_t)1 << 63;
  uint16_t exp = 16382;

  if( num < 1 ) return;

  while( frac < max ) {
    frac <<= 1;
  }
  while( num ) {
    exp++;
    num >>= 1;
  }
  head[0] = exp >> 8;              head[1] = exp & 0xff;
  head[2] = frac >> 56;            head[3] = ( frac >> 48 ) & 0xff;
  head[4] = ( frac >> 40 ) & 0xff; head[5] = ( frac >> 32 ) & 0xff;
  head[6] = ( frac >> 24 ) & 0xff; head[7] = ( frac >> 16 ) & 0xff;
  head[8] = ( frac >>  8 ) & 0xff; head[9] = frac & 0xff;
}

int
snd_write_aiffheader( void )
{
  libspectrum_byte buff[10];

  if( force_aifc || snd_enc != TYPE_PCM ) aifc = 1;		/* AIFF or AIFC */

  if( aifc )
    fwrite( "FORM\377\377\377\377AIFCCOMM\000\000\000\046", 20, 1, snd );
  else
    fwrite( "FORM\377\377\377\377AIFFCOMM\000\000\000\022", 20, 1, snd );

  if( snd_chn == 2 )
    fwrite( "\000\002", 2, 1, snd );			/* Stereo */
  else
    fwrite( "\000\001", 2, 1, snd );			/* Mono */

  fwrite( "\377\377\377\377", 4, 1, snd );			/* Stereo */
  if( snd_enc == TYPE_PCM )
    fwrite( "\000\020", 2, 1, snd );/* 16 bit/ sample */
  else
    fwrite( "\000\010", 2, 1, snd );/* 8 bit/ sample */

  aiff_int_2_80bitfloat( buff, out_rte );
  fwrite( buff, 10, 1, snd );			/* sampling rate in Hz */

/* NONE not compressed.
   ulaw uLaw 2:1.......
   alaw aLaw 2:1....... */
  if( aifc ) {
    if( snd_enc == TYPE_PCM )
      fwrite( "NONE\020not compressed ", 20, 1, snd );	/* PCM */
    else if( snd_enc == TYPE_ALW )
      fwrite( "alaw\020aLaw 2:1       ", 20, 1, snd );	/* A-Law */
    else
      fwrite( "ulaw\020uLaw 2:1       ", 20, 1, snd );	/* u-Law */
  }

  fwrite( "SSND\377\377\377\377\000\000\000\000\000\000\000\000", 16, 1, snd );/* data Chunk header */
  snd_header_ok = 1;
  printi( 1, "snd_write_aif%cheader(): %dHz %c encoded %s\n", aifc ? 'c' : 'f', snd_rte, snd_enc,
		 snd_chn == 2 ? "stereo" : "mono" );
  return 0;
}

int
snd_write_aiff( void )
{
  int err;

  if( !snd_header_ok && ( err = snd_write_aiffheader() ) ) return err;
  if( snd_enc == TYPE_PCM && snd_little_endian ) {	/* we have to swap all sample */
    pcm_swap_endian();
  }

  if( fwrite( sound8, snd_len, 1, snd ) != 1 ) return ERR_WRITE_SND;
  printi( 2, "snd_write_aif%c(): %d samples (%d bytes) sound\n", aifc ? 'c' : 'f', snd_len/snd_fsz, snd_len );

  return 0;
}

void
snd_finalize_aiff( void )
{
  libspectrum_byte buff[4];
  long pos = ftell( snd );
  long dsz = pos;

  if( fseek( snd, AIFF_POS_FILELEN, SEEK_SET ) == -1 ) {
    printw( "snd_finalize_aif%c(): cannot finalize sound output file (not seekable).\n", aifc ? 'c' : 'f' );
  } else {
    pos -= 8;
    buff[3] = pos & 0xff;           buff[2] = ( pos >> 8 ) & 0xff;
    buff[1] = ( pos >> 16 ) & 0xff; buff[0] = pos >> 24;
    fwrite( buff, 4, 1, snd );			/* set file length - 8 */

    fseek( snd, aifc ? AIFF_POS_SSNCLEN : AIFF_POS_SSNDLEN, SEEK_SET );
    pos -= aifc ? 52 : 32;
    buff[3] = pos & 0xff;           buff[2] = ( pos >> 8 ) & 0xff;
    buff[1] = ( pos >> 16 ) & 0xff; buff[0] = pos >> 24;
    fwrite( buff, 4, 1, snd );			/* set file length - 8 */

    fseek( snd, AIFF_POS_NOFRAME, SEEK_SET );
    dsz = pos - 8; dsz /= snd_fsz;
    buff[3] = dsz & 0xff;           buff[2] = ( dsz >> 8 ) & 0xff;
    buff[1] = ( dsz >> 16 ) & 0xff; buff[0] = dsz >> 24;
    fwrite( buff, 4, 1, snd );			/* set file length - 8 */

    printi( 1, "snd_finalize_aif%c(): FORM size: %ldbyte COMM frames: %ldbyte SSND size: %ld\n", aifc ? 'c' : 'f', pos + 32, dsz, pos );
  }
}
