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


int
snd_write_auheader( void )
{
  const char str[] =
    "fmfconv created AU file (http://fuse-emulator.sourceforge.net)\n";
  size_t len = sizeof(str); /* Includes the null terminator */
  int padding = len % 8;
  libspectrum_dword buff[6];

#ifdef WORDS_BIGENDIAN
  buff[0] = 0x2e736e64UL;		/* magic id */
  buff[1] = 24 + 64;
  buff[2] = ~0U;
  buff[3] = snd_enc == TYPE_PCM ? 3 : ( snd_enc == TYPE_ULW ? 1 : 27 );
  buff[4] = snd_rte;
  buff[5] = snd_chn;
#else
  buff[0] = swap_endian_dword( 0x2e736e64UL );		/* magic id */
  buff[1] = swap_endian_dword( 24 + 64 );
  buff[2] = swap_endian_dword( ~0U );
  buff[3] = swap_endian_dword( snd_enc == TYPE_PCM ? 3 : ( snd_enc == TYPE_ULW ? 1 : 27 ) );
  buff[4] = swap_endian_dword( snd_rte );
  buff[5] = swap_endian_dword( snd_chn );
#endif
  fwrite( buff, 24, 1, snd );

  fwrite( str, len, 1, snd );

  /* Pad with zeroes until the next multiple of 8 */
  if( padding ) {
    const char zeros[7] = { 0 };
    fwrite( zeros, 8 - padding, 1, snd );
  }

  snd_header_ok = 1;
  printi( 1, "snd_write_auheader(): %dHz %c encoded %s\n", snd_rte, snd_enc,
		 snd_chn == 2 ? "stereo" : "mono" );
  return 0;
}

int
snd_write_au( void )
{
  int err;

  if( !snd_header_ok && ( err = snd_write_auheader() ) ) return err;

  if( snd_enc == TYPE_PCM && snd_little_endian ) {	/* we have to swap all sample */
    pcm_swap_endian();
  }
  if( fwrite( sound8, snd_len, 1, snd ) != 1 ) return ERR_WRITE_SND;
  printi( 2, "snd_write_au(): %d samples (%d bytes) sound\n", snd_len/snd_fsz, snd_len );

  return 0;
}

void
snd_finalize_au( void )
{
  libspectrum_dword len;
  long pos = ftell( snd );

  if( fseek( snd, 8, SEEK_SET ) == -1 ) {
    printi( 2, "snd_finalize_au(): cannot finalize sound output file (not seekable).\n" );
  } else {
#ifdef WORDS_BIGENDIAN
    len = pos - 24 - 64;
#else
    len = swap_endian_dword( pos - 24 - 64 );
#endif
    fwrite( &len, 4, 1, snd );			/* data length */
    printi( 1, "snd_finalize_au(): DATA size: %ldbyte\n", pos - 24 - 64 );
  }
}
