/* fmfconv_png.c: png output routine included into fmfconv.c
   Copyright (c) 2015 Gergely Szasz

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

#include <png.h>

#ifdef HAVE_ZLIB_H
#include <zlib.h>
#else
#define Z_DEFAULT_COMPRESSION 6
#endif

#include <libspectrum.h>

#include "fmfconv.h"

extern libspectrum_byte yuv_pal[];
extern libspectrum_byte rgb_pal[];
extern int progressive;
extern int greyscale;
extern int png_palette;

int png_compress = Z_DEFAULT_COMPRESSION;

int
out_write_png( void )
{
  int y;
  libspectrum_byte *row_pointers[480];
  png_structp png_ptr;
  png_infop info_ptr;
  png_text text[3];

  for( y = 0; y < frm_h; y++ )
    if( png_palette )
      row_pointers[y] = &pix_rgb[ y * frm_w / 2 ];
    else
      row_pointers[y] = &pix_rgb[ 3 * y * frm_w ];

  png_ptr = png_create_write_struct( PNG_LIBPNG_VER_STRING, NULL, NULL, NULL );

  if( !png_ptr ) {
    printe( "Couldn't allocate `png_ptr'.\n" );
    return ERR_OUTOFMEM;
  }

  info_ptr = png_create_info_struct( png_ptr );
  if( !info_ptr ) {
    png_destroy_write_struct( &png_ptr, NULL );
    printe( "Couldn't allocate `info_ptr'.\n" );
    return ERR_OUTOFMEM;
  }

  /* Set up the error handling; libpng will return to here if it
     encounters an error */
  if( setjmp( png_jmpbuf( png_ptr ) ) ) {
    png_destroy_write_struct( &png_ptr, &info_ptr );
    printe( "Error from libpng.\n" );
    return ERR_WRITE_OUT;
  }

  png_init_io( png_ptr, out );

  /* Make files as small as possible */
  png_set_compression_level( png_ptr, png_compress );

  png_set_IHDR( png_ptr, info_ptr,
                frm_w, frm_h, png_palette ? 4 : 8,
                png_palette ? PNG_COLOR_TYPE_PALETTE : PNG_COLOR_TYPE_RGB,
                progressive ? PNG_INTERLACE_ADAM7 : PNG_INTERLACE_NONE,
                PNG_COMPRESSION_TYPE_DEFAULT,
                PNG_FILTER_TYPE_DEFAULT );
  if( png_palette ) {
    int i;
    png_colorp palette;

    palette = (png_colorp)png_malloc( png_ptr, 16 * sizeof( png_color ) );

    /* Set palette colors. */
    for( i = 0; i < 16; ++i ) {
      if( greyscale ) {
        palette[i].red = palette[i].green = palette[i].blue = yuv_pal[ i * 3 ];
      } else {
        palette[i].red   = rgb_pal[ i * 3 + 0 ];
        palette[i].green = rgb_pal[ i * 3 + 1 ];
        palette[i].blue  = rgb_pal[ i * 3 + 2 ];
      }
    }
    png_set_PLTE( png_ptr, info_ptr, palette, 16 );
  }

  text[0].key = "Software";
  text[0].text = "fmfconv -- Fuse Movie File converting utility\n";
  text[0].compression = PNG_TEXT_COMPRESSION_NONE;

  text[1].key = "Description";
  text[1].text = "ZX Spectrum PNG screenshot created by fmfconv\n";
  text[1].compression = PNG_TEXT_COMPRESSION_NONE;

  text[2].key = "URL";
  text[2].text = "http://fuse-emulator.sourceforge.net";
  text[2].compression = PNG_TEXT_COMPRESSION_NONE;

  png_set_text( png_ptr, info_ptr, text, 3 );

  png_set_rows( png_ptr, info_ptr, row_pointers );

  png_write_png( png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL );

  png_destroy_write_struct( &png_ptr, &info_ptr );

  printi( 2, "out_write_png()\n" );

  return 0;
}

void
print_png_version( void )
{
  printf( "  * libpng (version: " PNG_LIBPNG_VER_STRING ") support (PNG output)\n" );
}
