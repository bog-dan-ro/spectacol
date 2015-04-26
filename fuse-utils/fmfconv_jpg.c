/* fmfconv_jpg.c: jpeg/mjpeg/avi output routine included into fmfconv.c
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
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#define XMD_H
#include <jpeglib.h>

#include <libspectrum.h>

#include "fmfconv.h"

extern int progressive;
extern int greyscale;
extern char *out_orig;

int jpg_dctfloat = 0;
int jpg_idctfast = 0;
int jpg_optimize = 0;
int jpg_smooth = 0;
int jpg_quality = 75;

int jpeg_header_ok = 0;

static struct jpeg_compress_struct cinfo;
static struct jpeg_error_mgr jerr;
static unsigned char *mem_dest = NULL;
static unsigned long mem_size = 0;
static libspectrum_byte *row_pointers[480];

/* we need a custom 'dest' manager for AVI */
void jpeg_avi_mem_dest( j_compress_ptr cinfo, unsigned char **outbuffer,
                        unsigned long *outsize );

int
out_write_jpegheader( void )
{
  size_t y;

  for( y = 0; y < frm_h; y++ )
    row_pointers[y] = &pix_rgb[ ( greyscale ? 1 : 3 ) * y * frm_w ];

  cinfo.err = jpeg_std_error( &jerr );
  jpeg_create_compress( &cinfo );
  cinfo.image_height = frm_h;
  cinfo.image_width  = frm_w;
  cinfo.input_components = greyscale ? 1 : 3;
  cinfo.in_color_space = greyscale ? JCS_GRAYSCALE : JCS_RGB;
  jpeg_set_defaults( &cinfo );
  cinfo.dct_method = jpg_dctfloat ? JDCT_FLOAT :
                   ( jpg_idctfast ? JDCT_IFAST : JDCT_ISLOW );
  cinfo.optimize_coding = jpg_optimize ? TRUE : FALSE;
  cinfo.smoothing_factor = jpg_smooth < 0 || jpg_smooth > 100 ?
                           0 : jpg_smooth;
  cinfo.write_JFIF_header = TRUE;

  if( out_t != TYPE_JPEG )
    jpeg_set_colorspace( &cinfo, JCS_YCbCr );
  if( greyscale ) /* override AVI YCbCr... */
    jpeg_set_colorspace( &cinfo, JCS_GRAYSCALE );
  if( progressive )
    jpeg_simple_progression( &cinfo );
  jpeg_set_quality( &cinfo,
                    ( jpg_quality < 0 || jpg_quality > 100 ? 75 : jpg_quality ),
                    0 );
  if( out_t == TYPE_AVI )
    jpeg_avi_mem_dest( &cinfo, &mem_dest, &mem_size );
  else
    jpeg_stdio_dest( &cinfo, out );

  if( jpeg_header_ok >= 0 )
    printi( 1, "out_write_jpegheader(): W=%d H=%d\n", frm_w, frm_h );
  jpeg_header_ok = 1;

  return 0;
}

static int
write_jpeg_img( int tables, void *comment )
{
  int err;

  if( jpeg_header_ok < 1 && ( err = out_write_jpegheader() ) )
    return err;

  if( out_t == TYPE_JPEG )
    jpeg_stdio_dest( &cinfo, out );

  jpeg_start_compress( &cinfo, tables );
  if( comment != NULL )
    jpeg_write_marker( &cinfo, JPEG_COM, (void *)comment, strlen( comment ) );
  jpeg_write_scanlines( &cinfo, row_pointers, frm_h );
  jpeg_finish_compress( &cinfo );

  return 0;
}

int
out_write_mjpeg( void )
{
  int err;
  const char str[] =
    "fmfconv created M-JPEG file (http://fuse-emulator.sourceforge.net)\n";

  if( ( err = write_jpeg_img( FALSE, out_header_ok ? NULL : (void *)str ) ) )
    return err;
  out_header_ok = jpeg_header_ok;

  printi( 2, "out_write_mjpeg()\n" );
  return 0;
}

void
out_finalize_mjpeg( void )
{
  jpeg_destroy_compress( &cinfo );
}

int
out_write_jpg( void )
{
  int err;
  const char str[] =
    "fmfconv created JPEG file (http://fuse-emulator.sourceforge.net)\n";

  if( ( err = write_jpeg_img( TRUE, (void *)str ) ) ) return err;
  jpeg_header_ok = 0;
  jpeg_destroy_compress( &cinfo );

  printi( 2, "out_write_jpg()\n" );

  return 0;
}

/*-------------AVI/MJPEG---------------------*/
/*
Byte order: Little-endian

*/

#define OUTPUT_BUF_SIZE  4096	/* choose an efficiently fwrite'able size */

/* We need our memory destination, to manage multiple image (AVI) */
/* Expanded data destination object for memory output */

typedef struct {
  struct jpeg_destination_mgr pub; /* public fields */

  unsigned char **outbuffer;	/* target buffer */
  unsigned long *outsize;
  unsigned char *newbuffer;	/* newly allocated buffer */
  JOCTET *buffer;		/* start of buffer */
  size_t bufsize;
} avi_mem_destination_mgr;

typedef avi_mem_destination_mgr *avi_mem_dest_ptr;

/*
 * Initialize destination --- called by jpeg_start_compress
 * before any data is actually written.
 */

static void
init_avi_mem_destination( j_compress_ptr cinfo )
{
  /* no work necessary here */
}


/*
 * Empty the output buffer --- called whenever buffer fills up.
 *
 * In typical applications, this should write the entire output buffer
 * (ignoring the current state of next_output_byte & free_in_buffer),
 * reset the pointer & count to the start of the buffer, and return TRUE
 * indicating that the buffer has been dumped.
 *
 */

static boolean
empty_avi_mem_output_buffer( j_compress_ptr cinfo )
{
  size_t nextsize;
  JOCTET *nextbuffer;
  avi_mem_dest_ptr dest = (avi_mem_dest_ptr) cinfo->dest;

  /* Try to allocate new buffer with double size */
  nextsize = dest->bufsize * 2;
  nextbuffer = (JOCTET *)malloc( nextsize );

  if( nextbuffer == NULL ) {
    printe( "\n\nMemory allocation error.\n" );
    exit( ERR_OUTOFMEM );
  }

  memcpy( nextbuffer, dest->buffer, dest->bufsize );

  if( dest->newbuffer != NULL )
    free( dest->newbuffer );

  dest->newbuffer = nextbuffer;

  dest->pub.next_output_byte = nextbuffer + dest->bufsize;
  dest->pub.free_in_buffer = dest->bufsize;

  dest->buffer = nextbuffer;
  dest->bufsize = nextsize;

  return TRUE;
}


/*
 * Terminate destination --- called by jpeg_finish_compress
 * after all data has been written.  Usually needs to flush buffer.
 *
 * NB: *not* called by jpeg_abort or jpeg_destroy; surrounding
 * application must deal with any cleanup that should happen even
 * for error exit.
 */

static void
term_avi_mem_destination( j_compress_ptr cinfo )
{
  avi_mem_dest_ptr dest = (avi_mem_dest_ptr) cinfo->dest;

  *dest->outbuffer = dest->buffer;
  *dest->outsize = dest->bufsize - dest->pub.free_in_buffer;
}

static void
clear_avi_mem_destination( j_compress_ptr cinfo )
{
  avi_mem_dest_ptr dest = (avi_mem_dest_ptr) cinfo->dest;

  dest->pub.next_output_byte = dest->buffer;
  dest->pub.free_in_buffer = dest->bufsize;
}


/*
 * Prepare for output to a memory buffer.
 * The caller may supply an own initial buffer with appropriate size.
 * Otherwise, or when the actual data output exceeds the given size,
 * the library adapts the buffer size as necessary.
 * The standard library functions malloc/free are used for allocating
 * larger memory, so the buffer is available to the application after
 * finishing compression, and then the application is responsible for
 * freeing the requested memory.
 * Note:  An initial buffer supplied by the caller is expected to be
 * managed by the application.  The library does not free such buffer
 * when allocating a larger buffer.
 */

void
jpeg_avi_mem_dest( j_compress_ptr cinfo, unsigned char ** outbuffer,
                   unsigned long * outsize )
{
  avi_mem_dest_ptr dest;

  /* The destination object is made permanent so that multiple JPEG images
   * can be written to the same buffer without re-executing jpeg_mem_dest.
   */
  if( cinfo->dest == NULL ) {	/* first time for this JPEG object? */
    cinfo->dest = (struct jpeg_destination_mgr *)
      (*cinfo->mem->alloc_small)( (j_common_ptr)cinfo, JPOOL_PERMANENT,
                                   sizeof( avi_mem_destination_mgr ) );
  }

  dest = (avi_mem_dest_ptr)cinfo->dest;
  dest->pub.init_destination = init_avi_mem_destination;
  dest->pub.empty_output_buffer = empty_avi_mem_output_buffer;
  dest->pub.term_destination = term_avi_mem_destination;
  dest->outbuffer = outbuffer;
  dest->outsize = outsize;
  dest->newbuffer = NULL;

  if( *outbuffer == NULL || *outsize == 0 ) {
    /* Allocate initial buffer */
    dest->newbuffer = *outbuffer = (unsigned char *)malloc( OUTPUT_BUF_SIZE );
    if( dest->newbuffer == NULL ) {
      printe( "\n\nMemory allocation error.\n" );
      exit( ERR_OUTOFMEM );
    }
    *outsize = OUTPUT_BUF_SIZE;
  }

  dest->pub.next_output_byte = dest->buffer = *outbuffer;
  dest->pub.free_in_buffer = dest->bufsize = *outsize;
}


int
out_build_avi_mjpeg_frame( char **buffer, unsigned long *size )
{
  int err;

  if( ( err = out_write_mjpeg() ) ) return err;
  clear_avi_mem_destination( &cinfo );

  *buffer = (void *)mem_dest;
  *size = mem_size;
  return 0;
}

void
print_jpeg_version( void )
{
  printf( "  * libjpeg");

#ifdef JPEG_LIB_VERSION_MAJOR
  printf( " (version: %d.%d)", JPEG_LIB_VERSION_MAJOR, JPEG_LIB_VERSION_MINOR );
#else
  printf( " (version: %d)", JPEG_LIB_VERSION );
#endif

  printf( " support (JPEG, M-JPEG, AVI/M-JPEG output)\n" );
}
