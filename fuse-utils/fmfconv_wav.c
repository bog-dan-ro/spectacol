/* fmfconv_wav.c: wav output routine included into fmfconv.c
   Copyright (c) 2004-2005 Gergely Szasz

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

   E-mail: szaszg@hu.inter.net

*/
#include <config.h>

#include <stdio.h>
#include <stdlib.h>

#include "libspectrum.h"
#include "fmfconv.h"

/*
Byte order: Little-endian

Offset   Length   Contents
  0      4 bytes  "RIFF"
  4      4 bytes  <File length - 8>
  8      4 bytes  "WAVE"

 12      4 bytes  "fmt "
 16      4 bytes  <Length of the fmt data> // (=16)
 20      2 bytes  <WAVE File Encoding Tag>
 22      2 bytes  <Channels>     // Channels: 1 = mono, 2 = stereo
 24      4 bytes  <Sample rate>  // Samples per second: e.g., 44100
 28      4 bytes  <bytes/second> // sample rate * block align
 32      2 bytes  <block align>  // channels * bits/sample / 8
 34      2 bytes  <bits/sample>  // 8 or 16
.................................................... PCM
 36      4 bytes  "data"
 40      4 bytes  <Sample data size(n)>
 44     (n)bytes  <Sample data>
..................................................... Law!!!
 36      2 bytes  <cbLength>    // 0

 38      4 bytes  "fact"
 42      4 bytes  <Chunk size>  // 4
 46      4 bytes  <Number of samples >

 50      4 bytes  "data"
 54      4 bytes  <Sample data size(n)>
 58     (n)bytes  <Sample data>
......................................................

*/
#define WAV_POS_FILELEN 4L
#define WAV_POS_PCMLEN 40L
#define WAV_POS_LAWLEN 54L
#define WAV_POS_SAMPLES 46L

int
snd_write_wavheader( void )
{
  libspectrum_byte buff[4];
  int tmp;

  fwrite( "RIFF\377\377\377\377WAVEfmt ", 16, 1, snd );

  if( snd_enc == TYPE_PCM )
    fwrite( "\020\000\000\000\001\000", 6, 1, snd );	/* PCM */
  else if( snd_enc == TYPE_ALW )
    fwrite( "\022\000\000\000\006\000", 6, 1, snd );	/* A-Law */
  else
    fwrite( "\022\000\000\000\007\000", 6, 1, snd );	/* u-Law */

  if( snd_chn == 2 )
    fwrite( "\002\000", 2, 1, snd );			/* Stereo */
  else
    fwrite( "\001\000", 2, 1, snd );			/* Mono */

  buff[0] = snd_rte & 0xff; buff[1] = snd_rte >> 8; buff[2] = buff[3] = 0;
  fwrite( buff, 4, 1, snd );			/* sampling rate in Hz */

  tmp = snd_rte * snd_fsz;
  buff[0] = tmp & 0xff; buff[1] = ( tmp >> 8 )  & 0xff ; buff[2] = tmp >> 16; buff[3] = 0;
  fwrite( buff, 4, 1, snd );			/* avarege byte rate */

  buff[0] = snd_fsz & 0xff; buff[1] = snd_fsz >> 8; buff[2] = snd_fsz / snd_chn * 8; buff[3] = 0;
  fwrite( buff, 4, 1, snd );			/* frame size + bits/sample */

  if( snd_enc != TYPE_PCM )
    fwrite( "\000\000fact\004\000\000\000\377\377\377\377", 14, 1, snd );/*  */

  fwrite( "data\377\377\377\377", 8, 1, snd );/* data Chunk header */
  snd_header_ok = 1;
  printi( 1, "snd_write_wavheader(): %dHz %c encoded %s\n", snd_rte, snd_enc,
		 snd_chn == 2 ? "stereo" : "mono" );
  return 0;
}

int
snd_write_wav( void )
{
  int err;

  if( !snd_header_ok && ( err = snd_write_wavheader() ) ) return err;
  if( snd_enc == TYPE_PCM && !snd_little_endian ) {	/* we have to swap all sample */
    pcm_swap_endian();
  }

  if( fwrite( sound8, snd_len, 1, snd ) != 1 ) return ERR_WRITE_SND;
  printi( 2, "snd_write_wav(): %d samples (%d bytes) sound\n", snd_len/snd_fsz, snd_len );

  return 0;
}

void
snd_finalize_wav( void )
{
  libspectrum_byte buff[4];
  long pos = ftell( snd );
  long dsz = pos;

  if( fseek( snd, WAV_POS_FILELEN, SEEK_SET ) == -1 ) {
    printw( "snd_finalize_wav(): cannot finalize sound output file (not seekable).\n" );
  } else {
    if( pos & 1 ) {
      fwrite( buff, 1, 1, snd );		/* padding byte */
      pos++;
    }
    pos -= 8;
    buff[0] = pos & 0xff; buff[1] = ( pos >> 8 ) & 0xff;
    buff[2] = ( pos >> 16 ) & 0xff; buff[3] = pos >> 24;
    fwrite( buff, 4, 1, snd );			/* set file length - 8 */

    if( snd_enc == TYPE_PCM ) {
      dsz -= 44;
      fseek( snd, WAV_POS_PCMLEN, SEEK_SET );
    } else {
      dsz -= 58;
      fseek( snd, WAV_POS_LAWLEN, SEEK_SET );
    }
    buff[0] = dsz & 0xff; buff[1] = ( dsz >> 8 ) & 0xff;
    buff[2] = ( dsz >> 16 ) & 0xff; buff[3] = dsz >> 24;
    fwrite( buff, 4, 1, snd );			/* set data size */
    if( snd_enc != TYPE_PCM ) {
      fseek( snd, WAV_POS_SAMPLES, SEEK_SET );
      fwrite( buff, 4, 1, snd );			/* set sample num (1 byte/sample) */
    }

    printi( 1, "snd_finalize_wav(): RIFF size: %ldbyte DATA size: %ldbyte\n", pos, dsz );
  }
}
