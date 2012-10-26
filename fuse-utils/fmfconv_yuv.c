/* fmfconv_yuv.c: yuv4mpeg output routine included into fmfconv.c
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

int yuv_ylen = 0, yuv_uvlen = 0;
static libspectrum_byte pix_uv[2][40 * 8 * 240 * 2];	/* bpp = 2; w = 40*8 h = 240 timex = 2 subsampled yuv*/

/*
   444 			->  nothing
   422 cosited 		->  @*@* @*@*
                	    @*@* @*@*
   420j centered	->  @*@* @*@*
                	    **** ****
   420m h cosited 	->  * * * *  * * * *
                    	    @   @    @   @
                    	    * * * *  * * * *
   420 cosited 		->  @*@* @*@*
                	    **** ****

!!!410 cosited 		->  @*** @***		TODO
                	    **** ****
*/

static void
uv_subsample( void )
{
  libspectrum_byte *u, *v, *u_out, *v_out;
  int i, j;
  /*
	X X
	X X
  */
  u_out = pix_uv[0];
  v_out = pix_uv[1];

/*	--- YUV444 ---		*/
  if( yuv_t == TYPE_444 ) {	/*  */
    yuv_uvlen = yuv_ylen;

/*	--- YUV420J ---		*/
  } else if( yuv_t == TYPE_420J ) {	/* interstitial */
    for( j = 0; j < frm_h; j += 2 ) {	/* lines */
      u = &pix_yuv[1][j * frm_w];
      for( i = 0; i < frm_w; i += 2 ) {	/* columns */
	*u_out++ = ( (libspectrum_word)(*u) + *(u + 1) +
		    *( u + frm_w ) + *( u + frm_w + 1 ) ) >> 2;
	u += 2;
      }
    }
    for( j = 0; j < frm_h; j += 2 ) {	/* lines */
      v = &pix_yuv[2][j * frm_w];
      for( i = 0; i < frm_w; i += 2 ) {	/* columns */
        *v_out++ = ( (libspectrum_word)(*v) + *(v + 1) +
		    *( v + frm_w ) + *( v + frm_w + 1 ) ) >> 2;
	v += 2;
      }
    }
    yuv_uvlen = yuv_ylen >> 2;

/*	--- YUV420M ---		*/
  } else if( yuv_t == TYPE_420M ) {	/* horiz cosited */
    for( j = 0; j < frm_h; j += 2 ) {	/* lines */
      u = &pix_yuv[1][j * frm_w];
      *u_out++ = ( (libspectrum_word)(*u) * 3 + *(u + 1) +
		    *( u + frm_w ) * 3 + *( u + frm_w + 1 ) ) >> 3;
      u += 2;
      for( i = 2; i < frm_w; i += 2 ) {	/* columns */
        *u_out++ = ( (libspectrum_word)(*(u - 1)) + *u * 2 + *(u + 1) +
		    *( u + frm_w - 1 ) + *( u + frm_w ) * 2 +
		    *( u + frm_w + 1 ) ) >> 3;
	u += 2;
      }
    }
    for( j = 0; j < frm_h; j += 2 ) {	/* lines */
      v = &pix_yuv[2][j * frm_w];
      *v_out++ = ( (libspectrum_word)(*v) * 3 + *(v + 1) +
		    *( v + frm_w ) * 3 + *( v + frm_w + 1 ) ) >> 3;
      v += 2;
      for( i = 2; i < frm_w; i += 2 ) {	/* columns */
        *v_out++ = ( (libspectrum_word)(*(v - 1)) + *v * 2 + *(v + 1) +
		    *( v + frm_w - 1 ) + *( v + frm_w ) * 2 +
		    *( v + frm_w + 1 ) ) >> 3;
	v += 2;
      }
    }
    yuv_uvlen = yuv_ylen >> 2;

/*	--- YUV420 ---		*/
  } else if( yuv_t == TYPE_420 ) {	/* cosited */
 /*
     abcdef      hino =>  1/4a+1/2b+1/4c+1/2g+h+1/2i+1/4m+1/2n+1/4o / 4
     ghijkl      abgh =>  1/4a+1/2a+1/4b+1/2a+a+1/2b+1/4g+1/2g+1/4h / 4
     mnopqr      ghmn =>  1/4a+1/2a+1/4b+1/2g+g+1/2h+1/4m+1/2m+1/4n / 4
  */
    for( j = 0; j < frm_h; j += 2 ) {	/* lines */
      u = &pix_yuv[1][j * frm_w];
      if( j == 0 )
        *u_out++ = ( (libspectrum_word)(*u) * 9 + *(u + 1) * 3 +
		    *(u + frm_w) * 3 + *(u + frm_w + 1) ) >> 4;
      else
        *u_out++ = ( (libspectrum_word)(*(u - frm_w)) * 3 +
						    *(u - frm_w + 1) +
		    (*u) * 5 + *(u + 1) * 2 +
		    *(u + frm_w) * 3 + *(u + frm_w + 1) ) >> 4;
      u += 2;
      for( i = 2; i < frm_w; i += 2 ) {	/* columns */
        *u_out++ = ( (libspectrum_word)(*(u - frm_w - 1)) +
			*(u - frm_w) * 2 + *(u - frm_w + 1) +
		    *(u - 1) * 2 + *u * 4 + *(u + 1) * 2 +
		    *(u + frm_w - 1) +
			*(u + frm_w) * 2 + *(u + frm_w + 1) ) >> 4;
	u += 2;
      }
    }
    for( j = 0; j < frm_h; j += 2 ) {	/* lines */
      v = &pix_yuv[2][j * frm_w];
      if( j == 0 )
        *v_out++ = ( (libspectrum_word)(*v) * 9 + *(v + 1) * 3 +
		    *(v + frm_w) * 3 + *(v + frm_w + 1) ) >> 4;
      else
        *v_out++ = ( (libspectrum_word)(*(v - frm_w)) * 3 +
						    *(v - frm_w + 1) +
		    (*v) * 5 + *(v + 1) * 2 +
		    *(v + frm_w) * 3 + *(v + frm_w + 1) ) >> 4;
      v += 2;
      for( i = 2; i < frm_w; i += 2 ) {	/* columns */
        *v_out++ = ( (libspectrum_word)(*(v - frm_w - 1)) +
			*(v - frm_w) * 2 + *(v - frm_w + 1) +
		    *(v - 1) * 2 + *v * 4 + *(v + 1) * 2 +
		    *(v + frm_w - 1) +
			*(v + frm_w) * 2 + *(v + frm_w + 1) ) >> 4;
	v += 2;
      }
    }
    yuv_uvlen = yuv_ylen >> 2;

/*	--- YUV422 ---		*/
  } else if( yuv_t == TYPE_422 ) {		/* cosited (PAL/NTSC TV)*/
    for( j = 0; j < frm_h; j++ ) {	/* lines */
      u = &pix_yuv[1][j * frm_w];
      *u_out++ = ( (libspectrum_word)(*u) * 3 + *(u + 1) +
		    *( u + frm_w ) * 3 + *( u + frm_w + 1 ) ) >> 3;
      u += 2;
      for( i = 2; i < frm_w - 2; i += 2 ) {	/* columns */
        *u_out++ = ( (libspectrum_word)(*(u - 1)) + *u * 2 + *(u + 1) +
		    *( u + frm_w - 1 ) + *( u + frm_w ) * 2 +
		    *( u + frm_w + 1 ) ) >> 3;
	u += 2;
      }
      *u_out++ = ( (libspectrum_word)(*u) + *(u + 1) * 3 +
		    *( u + frm_w ) + *( u + frm_w + 1 ) * 3 ) >> 3;
      u += 2;
    }
    for( j = 0; j < frm_h; j++ ) {	/* lines */
      v = &pix_yuv[2][j * frm_w];
      *v_out++ = ( (libspectrum_word)(*v) * 3 + *(v + 1) +
		    *( v + frm_w ) * 3 + *( v + frm_w + 1 ) ) >> 3;
      v += 2;
      for( i = 2; i < frm_w - 2; i += 2 ) {	/* columns */
        *v_out++ = ( (libspectrum_word)(*(v - 1)) + *v * 2 + *(v + 1) +
		    *( v + frm_w - 1 ) + *( v + frm_w ) * 2 +
		    *( v + frm_w + 1 ) ) >> 3;
	v += 2;
      }
      *v_out++ = ( (libspectrum_word)(*v) + *(v + 1) * 3 +
		    *( v + frm_w ) + *( v + frm_w + 1 ) * 3 ) >> 3;
      v += 2;
    }
    yuv_uvlen = yuv_ylen >> 1;
  }

/*	--- YUV410 ---		*/
}


int
out_write_yuvheader( void )
{
  const char *yuv4mpeg2[] = {
    "444", "422", "420jpeg", "420mpeg2", "420", "410"
  };

  fprintf( out, 
	    "YUV4MPEG2"
	    " W%d H%d F%d:1000 Ip A%c:1 C%s\n", 
	frm_w, frm_h, out_fps,
	 ( scr_t == TYPE_HRE && 0 ? '2' : '1' ),
	 yuv4mpeg2[yuv_t - TYPE_444] );
  out_header_ok = 1;
  printi( 1, "out_write_yuvheader(): W=%d H=%d F=%d:1000 A=%d\n", frm_w, frm_h,
		 out_fps, 1 );
  return 0;
}

int
out_write_yuv( void )
{
  int err;

  if( !out_header_ok && ( err = out_write_yuvheader() ) ) return err;

  fprintf( out, "FRAME\n" );
  if( fwrite( pix_yuv[0], yuv_ylen, 1, out ) != 1 ) return ERR_WRITE_OUT;
  if( yuv_t != TYPE_444 ) {
    uv_subsample();
    if( fwrite( pix_uv[0], yuv_uvlen, 1, out ) != 1 ) return ERR_WRITE_OUT;
    if( fwrite( pix_uv[1], yuv_uvlen, 1, out ) != 1 ) return ERR_WRITE_OUT;
  } else {
    if( fwrite( pix_yuv[1], yuv_ylen, 1, out ) != 1 ) return ERR_WRITE_OUT;
    if( fwrite( pix_yuv[2], yuv_ylen, 1, out ) != 1 ) return ERR_WRITE_OUT;
  }

  printi( 2, "out_write_yuv()\n" );

  return 0;
}
