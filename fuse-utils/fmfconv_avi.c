/* fmfconv_avi.c: avi output with raw video/audio included into fmfconv.c
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

#include <libspectrum.h>

#include "fmfconv.h"

extern int progressive;
extern int greyscale;
extern char *out_orig;
extern int avi_subtype;
#ifdef USE_LIBJPEG
extern int jpeg_header_ok;
#endif

static char *frame_buffer = NULL;
static unsigned long int frame_size = 0;

static libspectrum_byte buffer[64];
static int idx;

static char *idx_name = NULL;
static FILE *ifile = NULL;	/* index file */
static libspectrum_qword vid_frames_no = 0;
static libspectrum_qword snd_frames_no = 0;
static libspectrum_dword vid_stream_id;

/*-------------AVI/RAW---------------------*/
/*
Byte order: Little-endian

*/

#define WORD( x ) buffer[idx++] =   (x) & 0xff; \
                  buffer[idx++] = ( (x) & 0xff00 ) >> 8

#define DWORD( x ) buffer[idx++] =   (x) & 0xff; \
                   buffer[idx++] = ( (x) & 0xff00 ) >> 8; \
                   buffer[idx++] = ( (x) & 0xff0000 ) >> 16; \
                   buffer[idx++] = ( (x) & 0xff000000U ) >> 24

#define WSTRING( str ) fwrite( str, strlen( str ), 1, out )

#define WORD2( w1, w2 ) WORD( w1 ); DWORD( w2 )
#define WORD4( w1, w2, w3, w4 ) WORD2( w1, w2 ); DWORD2( w3, w4 )

#define WWORD( w ) idx = 0; WORD( w ); fwrite( buffer, 2, 1, out )
#define W2WORD( w1, w2 ) idx = 0; WORD2( w1, w2 ); fwrite( buffer, 4, 1, out )
#define W4WORD( w1, w2, w3, w4 ) \
  idx = 0; WORD4( w1, w2, w3, w4 ); fwrite( buffer, 8, 1, out )

#define DWORD2( dw1, dw2 ) DWORD( dw1 ); DWORD( dw2 )
#define DWORD4( dw1, dw2, dw3, dw4 ) DWORD2( dw1, dw2 ); DWORD2( dw3, dw4 )

#define WDWORD( dw ) idx = 0; DWORD( dw ); fwrite( buffer, 4, 1, out )
#define W2DWORD( dw1, dw2 ) \
  idx = 0; DWORD2( dw1, dw2 ); fwrite( buffer, 8, 1, out )
#define W4DWORD( dw1, dw2, dw3, dw4 ) \
  idx = 0; DWORD4( dw1, dw2, dw3, dw4 ); fwrite( buffer, 16, 1, out )

static int
out_write_aviheader( void )
{
  int diBitCount;

#ifdef USE_LIBJPEG
  if( avi_subtype != TYPE_AVI_DIB )
    diBitCount = 16, vid_stream_id = 0x63643030;
  else
#endif
    diBitCount = 8 * ( greyscale ? 1 : 3 ), vid_stream_id = 0x62643030;

  if( !out_to_stdout ) {
    if( !( idx_name = malloc( strlen( out_name ) + 5 ) ) ) {
      printe( "out_write_aviheader(): out of memory error.\n" );
      return ERR_WRITE_OUT;
    }
    sprintf( idx_name, "%s.idx", out_name );
    if( !( ifile = fopen_overwr( idx_name, "w+b", 1 ) ) ) {
      printe( "out_write_aviheader(): cannot create temporary idx file: %s.\n",
              strerror( errno ) );
      return ERR_WRITE_OUT;
    }

#ifndef WIN32
    /* Note: On Windows, the file remains in existence after the file
       descriptor referring to it is closed */
    unlink( idx_name );
    free( idx_name );
    idx_name = NULL;
#endif
  }

#define AVI_POS_FILELEN 0x04L
#define AVI_POS_HDRLEN 0x10L
  WSTRING( "RIFF\177\377\377\377AVI LIST"); WDWORD( 0x170 ); WSTRING("hdrlavih" );

/* Length of the avih, dwMicroSecPerFrame, dwMaxBytesPerSec, dwPaddingGranularity */
  W4DWORD( 0x38, ( 1000000000L / out_fps ),
           ( frm_w * frm_h * 3 * out_fps / 1000 + out_rte * snd_fsz ), 0 );
#define AVI_POS_TOTALFRAMES 0x30L
/* dwFlags, ++dwTotalFrames++, dwInitialFrames, dwStreams */
  W4DWORD( 0x910, 0x8fffffffU, 0, 2 );

/* dwSuggestedBufferSize, dwWidth, dwHeight, dwReserved */
  WDWORD( 1000000 ); W2DWORD( frm_w, frm_h ); W4DWORD( 0, 0, 0, 0 );

  WSTRING( "LIST" ); WDWORD( 0x0c0 ); WSTRING( "strl" );
  WSTRING( "strh" ); WDWORD( 0x038 );
#ifdef USE_LIBJPEG
  if( avi_subtype != TYPE_AVI_DIB )
    WSTRING( "vidsMJPG" );
  else
#endif
  if( greyscale )
    WSTRING( "vidsYUV " );
  else
    WSTRING( "vidsRGB " );

/* dwFlags, wPriority, wLanguage, dwInitialFrames */
  WDWORD( 0 ); W2WORD( 0, 0 ); WDWORD( 0 );

#define AVI_POS_VIDEOFRAMES 0x8cL
/* dwScale, dwRate, dwStart, ++dwLength++ */
  W4DWORD( 1000, out_fps, 0, 0x8fffffffU );

/* dwSuggestedBufferSize, dwQuality, dwSampleSize, rcFrame */
  W2DWORD( frm_w * frm_h * 3, -1 ); WDWORD( 0 ); W4WORD( 0, 0, frm_w, frm_h );

/* stream format (dw)biSize */
  WSTRING( "strf" ); WDWORD( 0x28 ); WDWORD( 0x028 );

  /* (dw)biWidth, (dw)biHeight, (w)biPlanes, (w)biBitCount */
  W2DWORD( frm_w, frm_h * ( avi_subtype == TYPE_AVI_DIB ? -1 : 1 ) );
  W2WORD( 1, diBitCount );

/* (dw)biCompression, (dw)biSizeImage (dw)biXPelsPerMeter, (dw)biYPelsPerMeter */
#ifdef USE_LIBJPEG
  if( avi_subtype != TYPE_AVI_DIB )
    WSTRING( "MJPG" );
  else
#endif
  if( greyscale ) {
    WSTRING( "Y8  " );
  } else {
    WDWORD( 0x0 ); /* BI_RGB */
  }

  WDWORD( frm_w * frm_h * ( greyscale ? 1 : 3 ) );  W2DWORD( 0, 0 );

/* (dw)biClrUsed, (dw)biClrImportant, (dw)biExtDataOffset???? */
  W2DWORD( 0, 0 );

/* video prop, (dw)biSize, (dw)VideoFormatToken, (dw)VideoStandard, dwVerticalRefreshRate  */
  WSTRING( "JUNK" ); WDWORD( 0x44 ); W4DWORD( 0, 0, 50, frm_w );
/* dwVerticalRefreshRate, dwHTotalInT, dwVTotalInLines, dwFrameAspectRatio */
  WDWORD( 15 ); /*WDWORD( frm_h );*/ W2WORD( 0x03, 0x04 ); W2DWORD( frm_w, 15 );
/* dwFrameWidthInPixels, dwFrameHeightInLines, nbFieldPerFrame */
  W4DWORD( 1, 15, frm_w, 15 );
/* (dw)CompressedBMHeight, (dw)CompressedBMWidth, (dw)ValidBMHeight, (dw)ValidBMWidth */
  W4DWORD( frm_w, 0, 0, 0 );
  WDWORD( 15 );

  WSTRING( "LIST" ); WDWORD( 0x05c ); WSTRING( "strl" );
  WSTRING( "strh" ); WDWORD( 0x038 ); WSTRING( "auds" );

/* (dw)fccHandler */
  WDWORD( snd_enc == TYPE_PCM ? 1 : ( snd_enc == TYPE_ALW ? 0x06 : 0x07 ) );
/* dwFlags, wPriority, wLanguage, dwInitialFrames */
  WDWORD( 0 ); W2WORD( 0, 0 ); WDWORD( 0 );

#define AVI_POS_AUDIOFRAMES 0x154L
/* dwScale, dwRate, dwStart, ++dwLength++ */
  W4DWORD( 1, out_rte, 0, 0 );

/* dwSuggestedBufferSize, dwQuality, dwSampleSize, rcFrame */
  W2DWORD( 1000, 0 ); WDWORD( snd_fsz );
  W4WORD( 0, 0, 0, 0 );

/* stream format (dw)biSize */
  WSTRING( "strf" ); WDWORD( 0x010 );
/*   wFormatTag, (w)nChannels, (dw)nSamplesPerSec, (dw)nAvgBytesPerSec */
  W2WORD( snd_enc == TYPE_PCM ? 1 : ( snd_enc == TYPE_ALW ? 0x06 : 0x07 ), out_chn );
  W2DWORD( out_rte, out_rte * snd_fsz / snd_chn );

/* (w)nBlockAlign, wBitsPerSample */
  W2WORD( snd_fsz, 8 * snd_fsz / snd_chn );

  WSTRING( "LIST" ); WDWORD( 0x08c ); WSTRING( "INFOISFT" ); WDWORD( 0x02e );
  WSTRING( "fmfconv -- Fuse Movie File converting utility\n" );
  WSTRING( "ICMT" ); WDWORD( 0x04a );
  WSTRING( "ZX Spectrum movie created by fmfconv\n" );
  WSTRING( "http://fuse-emulator.sourceforge.net\n" );
#define AVI_POS_MOVILEN 0x21cL
#define AVI_POS_MOVISTART ( AVI_POS_MOVILEN + 4L )
  WSTRING( "LIST" ); WDWORD( 0 ); WSTRING( "movi" ); /* 00dc .... ; 01wb .... */

  if( avi_subtype == TYPE_AVI_DIB ) {
    frame_buffer = (void *)pix_rgb;
    frame_size = frm_w * frm_h * ( greyscale ? 1 : 3 );
  }

  out_header_ok = 1;
  printi( 1, "out_write_aviheader(): W=%d H=%d\n", frm_w, frm_h );

  return 0;
}

int
out_write_avi( void )
{
  int err;
  FILE *tmp;
  long pos1, pos2;
  static int avi_seq_no = 1;

  if( !out_header_ok && ( err = out_write_aviheader() ) )
    return err;

  if( !out_to_stdout )
    pos1 = ftell( out );
  else
    pos1 = 0;
#ifdef USE_LIBJPEG
  if( avi_subtype != TYPE_AVI_DIB )
    err = out_build_avi_mjpeg_frame( &frame_buffer, &frame_size );
  if( err ) return err;
#endif

  W2DWORD( vid_stream_id, frame_size );
  if( fwrite( frame_buffer, frame_size, 1, out ) != 1 ) return ERR_WRITE_OUT;

  if( frame_size & 0x01 ) WSTRING( "P" ); /* pad to even byte boundary */

  printi( 4, "out_write_avi(): %ld byte image\n", frame_size );

  vid_frames_no++;

  if( !out_to_stdout ) {
    tmp = out;
    out = ifile;

    W4DWORD( vid_stream_id, 0x00010, pos1 - AVI_POS_MOVISTART, frame_size );
    pos2 = ftell( out );
    out = tmp;
  }

  /* rename old file with serial and open a new */
  if( !out_to_stdout && pos1 + pos2 + frame_size >= 1024 * 1024 * 1024 ) {

    if( out_orig == NULL ) {
      char *old_name;
      old_name = strdup( out_name );
      next_outname( 0 );
      if( ( err = rename( old_name, out_name ) ) ) {
        free( old_name );
        printe( "out_write_avi(): cannot rename avi file: `%s`\n",
                strerror( errno ) );
        return ERR_WRITE_OUT;
      }
      printi( 0, "out_write_avi(): AVI file size limit reached, old file renamed to: %s.\n",
              out_name );
      free( old_name );
    }
    close_out();
    next_outname( avi_seq_no );
    avi_seq_no++;
    printi( 0, "out_write_avi(): AVI file size limit reached, new file opened: %s.\n",
            out_name );
    if( ( err = open_out() ) ) {
      printe( "out_write_avi(): cannot open new file: `%s`\n",
              strerror( errno ) );
      return err;
    }
#ifdef USE_LIBJPEG
    jpeg_header_ok = 0;
#endif
    out_header_ok = 0;
  }
  printi( 2, "out_write_avi() frame no.: %"PRIu64"\n", vid_frames_no );
  return 0;
}

int
snd_write_avi( void )
{
  int err;
  FILE *tmp;
  long pos1;

  if( !out_header_ok && ( err = out_write_aviheader() ) )
    return err;

  /* we have to swap all samples */
  if( snd_enc == TYPE_PCM && !snd_little_endian ) {
    pcm_swap_endian();
  }

  if( !out_to_stdout )
    pos1 = ftell( out );
  else
    pos1 = 0;
  W2DWORD( 0x62773130, snd_len );
  if( fwrite( sound8, snd_len, 1, out ) != 1 ) return ERR_WRITE_SND;

  if( snd_len & 0x01 ) WSTRING( "P" );
  printi( 2, "snd_write_avi(): %d samples sound\n", snd_len / snd_fsz );
  snd_frames_no++;

  if( !out_to_stdout ) {
    tmp = out;
    out = ifile;

    W4DWORD( 0x62773130, 0x00010, pos1 - AVI_POS_MOVISTART, snd_len );
    out = tmp;
  }
  printi( 2, "snd_write_avi() frame no.: %"PRIu64"\n", snd_frames_no );
  return 0;
}

void
out_finalize_avi( void )
{
  char buff[4096];
  long pos1, pos2;

  if( out_to_stdout ) {
    printi( 1, "out_finalize_avi(): cannot finalize output on stdout.\n" );
    return;
  }
  fflush( ifile );
  pos1 = ftell( out );
  pos2 = ftell( ifile );
  WSTRING( "idx1" ); WDWORD( pos2 );

  rewind( ifile );
  while( !feof( ifile ) ) {
    size_t len;
    len = fread( buff, 1, 4096, ifile );
    if( len > 0 ) {
      if( len != fwrite( buff, 1, len, out ) ) {
        return;
      }
    }
  }
  printi( 2, "out_finalize_avi(): Write idx1: %libyte\n", pos2 );
  fclose( ifile );

  if( idx_name ) {
    unlink( idx_name );
    free( idx_name );
    idx_name = NULL;
  }

  fflush( out );

  pos2 = ftell( out );
  if( fseek( out, AVI_POS_TOTALFRAMES, SEEK_SET ) == -1 ) {
    printw( "out_finalize_avi(): cannot finalize output file (not seekable).\n" );
    return;
  }
  WDWORD( output_no );
  fseek( out, AVI_POS_VIDEOFRAMES, SEEK_SET );
  WDWORD( output_no );
  fseek( out, AVI_POS_AUDIOFRAMES, SEEK_SET );
  WDWORD( output_no );
  fseek( out, AVI_POS_MOVILEN, SEEK_SET );
  WDWORD( pos1 - AVI_POS_MOVISTART );

  fseek( out, AVI_POS_FILELEN, SEEK_SET );
  WDWORD( pos2 - 8 );

  printi( 2, "out_finalize_avi(): image chunks: %"PRIu64", sound chunks: %"PRIu64"\n",
          vid_frames_no, snd_frames_no );
  printi( 1, "out_finalize_avi(): TotalFrames: %"PRIu64"\n", output_no );
}
