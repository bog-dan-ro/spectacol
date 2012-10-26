/* fmfconv_ppm.c: ppm output routine included into fmfconv.c
   Copyright (c) 2004-2005 Gergely Szasz

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


int
out_write_ppm( void )
{
  fprintf( out, "P6\n#ZX Spectrum screenshot created by fmfconv "
		"(http://fuse-emulator.sourceforge.net) "
		"\n%u %u 255\n", frm_w, frm_h );

  if( fwrite( pix_rgb, frm_w * frm_h * 3, 1, out ) != 1 ) return ERR_WRITE_OUT;

  printi( 2, "out_write_ppm()\n" );

  return 0;
}
