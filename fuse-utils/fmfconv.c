/* fmfconv.c: Convert .fmf movie files
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
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#ifdef HAVE_STRINGS_H
#include <strings.h>		/* Needed for strncasecmp() on QNX6 */
#endif /* #ifdef HAVE_STRINGS_H */
#include <fcntl.h>

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#else
#define PRIu64 "llu"
#endif /* #ifdef HAVE_INTTYPES_H */

#include "libspectrum.h"
#include "movie_tables.h"

#include <getopt.h>

#ifdef USE_FFMPEG
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#endif

#ifdef HAVE_ZLIB_H
#define USE_ZLIB 1
#endif

#ifdef USE_ZLIB
#include <zlib.h>
#endif

#include "fmfconv.h"

#define VERBOSE_MAX 3
#define VERBOSE_MIN -1

#ifdef USE_ZLIB
#define ZBUF_SIZE 1024
#define ZBUF_INP_SIZE 512
z_stream zstream;			/* zlib struct */
unsigned char zbuf_o[ZBUF_SIZE];	/* zlib output buffer */
unsigned char zbuf_i[ZBUF_INP_SIZE];	/* zlib input buffer */
int fmf_compr_feof = 0;
#endif	/* USE_ZLIB */

typedef enum {
  DO_FILE = 0,		/* open (next) file */
  DO_HEAD,		/* read and check file header (FMF) */
  DO_FRAME_HEAD,	/* a video frame ready */
  DO_SLICE,		/* read next slice or sound chunk */
  DO_SOUND,		/* a sound chunk ready */
  DO_SOUND_FLUSH,	/* a sound chunk ready but flush sound buffer */
  DO_FRAME,		/* a video frame ready */
  DO_LAST_FRAME,	/* last video frame ready */
  DO_EOP,		/* end of input(s) */
} do_t;

int help_exit = 0;	/* exit after print help */
int verbose = 0;
int do_info = 0;	/* no output and verbose 2 */
do_t do_now = DO_FILE;

FILE *inp_file = NULL, *out = NULL, *snd = NULL;

type_t inp_t = TYPE_UNSET, out_t = TYPE_UNSET, snd_t = TYPE_UNSET, scr_t = TYPE_UNSET, prg_t = TYPE_NONE;

const char *inp_name = NULL;
const char *out_name = NULL;
const char *snd_name = NULL;

char out_tmpl[16];		/* multiple out filename number template */
const char *out_orig = NULL;		/* multiple out original filename/template */
const char *out_pfix = NULL;		/* after number */
char *out_nmbr = NULL;		/* start of number */
char *out_next = NULL;		/* multiple out filename next name */

libspectrum_qword cut_frm = 0, cut__to = 0;
type_t cut_f_t, cut_t_t, cut_cmd = TYPE_NONE;
int cut_cut = 0;
char *out_cut = NULL;

int inp_ftell = 0;

int sound_exists = 0;				/* FMF frame without sound */

libspectrum_qword input_no = 0;         /* current file (input) no */
libspectrum_qword input_last_no = 0;    /* number of files (input) - 1 */


int inp_fps = 50000;
long int inp_drop_value = 0;

int machine_timing[] = {                /* timing constants for different machines */
  50080, 50021, 60115, 50000, 59651
};

int machine_ftime[] = {
  19968, 19992, 16635, 20000, 16764
};

const char *machine_name[] = {
 "ZX Spectrum 16K/48K, Timex TC2048/2068, Scorpion, Spectrum SE",
 "ZX Spectrum 128K/+2/+2A/+3/+3E",
 "Timex TS2068", "Pentagon 128K/256K/512K"
 "ZX Spectrum 48K (NTSC)"
};

#define SCR_PITCH 40
libspectrum_byte zxscr[9600 * 2];	/* 2x 40x240 bitmap1 bitmap2 */
libspectrum_byte attrs[9600];		/* 40x240 attrib */

#define SOUND_CHUNK_LEN 8192
libspectrum_signed_byte *sound8;	/* sound buffer for x-law */
libspectrum_signed_word *sound16;	/* sound buffer for pcm */
size_t sound_buf_len = 0;

libspectrum_byte yuv_pal[] = {
  0, 128, 128,  22, 224, 112,  57, 96, 224,   79, 191, 208,
  112, 65, 48,  134, 160, 32,  169, 32, 144,  191, 128, 128,
  0, 128, 128,  29, 255, 107,  76, 85, 255,   105, 212, 235,
  150, 44, 21,  179, 171, 0,   226, 0, 149,   255, 128, 128,
};

libspectrum_byte rgb_pal[] = {
  0, 0, 0,    0, 0, 192,    192, 0, 0,    192, 0, 192,
  0, 192, 0,  0, 192, 192,  192, 192, 0,  192, 192, 192,
  0, 0, 0,    0, 0, 255,    255, 0, 0,    255, 0, 255,
  0, 255, 0,  0, 255, 255,  255, 255, 0,  255, 255, 255,
};

type_t yuv_t = TYPE_420M;

#define OUT_PITCH 320
libspectrum_byte pix_rgb[3 * 40 * 8 * 240 * 4];	/* bpp = 3; w = 40*8 h = 240 timex = 2*2 */

libspectrum_byte *pix_yuv[3] = {		/* other view of data */
  pix_rgb, NULL, NULL
};

libspectrum_qword frame_no = 0;	/* current frame (input) no */
libspectrum_qword time_sec = 0, time_frm = 0;	/* current time (input) no */
libspectrum_qword drop_no = 0;	/* dropped frame no */
libspectrum_qword dupl_no = 0;	/* duplicated frame no */
libspectrum_qword output_no = 0;	/* output frame no */
int frm_scr = 0, frm_rte = 0, frm_mch = 0; /* screen type, frame rate, machine type (A/B/C/D) */
int frm_slice_x, frm_slice_y, frm_slice_w, frm_slice_h;
int frm_w = -1, frm_h = -1;
int frm_fps = 0;

int out_w, out_h;
int out_fps = 0;			/* desired output frame rate */
int out_header_ok = 0;			/* output header ok? */

int out_chn = 2, out_rte = -1, out_fsz, out_len;	/* by default convert sound to 2 channel (STEREO) */

/* fmf variables */
int fmf_compr = 0;			/* fmf compressed or not */
int fmf_little_endian = 0;		/* little endian PCM */
int snd_little_endian = -1;			/* little endian PCM */
libspectrum_qword fmf_slice_no = 0;
libspectrum_qword fmf_sound_no = 0;

/* sound variables */
#define SOUND_BUFF_MAX_SIZE 65536	/* soft max size of collected sound samples */
type_t snd_enc;				/* sound type (pcm/alaw) */
int snd_rte, snd_chn, snd_fsz, snd_len;	/* sound rate (Hz), sound channels (1/2), sound length in byte  */
int snd_header_ok = 0;			/* sound header ok? */

int sound_raw = 0;			/* do not do law->PCM and channels conversion */
int sound_pcm = 1;			/* by default always convert sound to PCM */
int sound_stereo = 1;			/* by default always convert sound to STEREO */
int sound_only = 0;			/* by default process video */

libspectrum_signed_word alaw_table[256] = { ALAW_TAB };

libspectrum_byte fhead[32];		/* fmf file/frame/slice header */

void close_out( void );

#define FBUFF_SIZE 32
libspectrum_byte fbuff[FBUFF_SIZE];		/* file buffer used for check file type... */
size_t fbuff_size = 0;

#define INTO_BUFF 0
#define FROM_BUFF 1

size_t
fread_buff( libspectrum_byte *ptr, size_t size, int how )	/* read from inp, how -> into buff or buff + file */
{
  size_t s;
  size_t n;

  if( how == INTO_BUFF ) {
    n = size > FBUFF_SIZE ? FBUFF_SIZE : size;

    s = fread( fbuff, n, 1, inp_file );
    if( s != 1 ) return s;
    fbuff_size = n;
    memcpy( ptr, fbuff, n );
    if( size > FBUFF_SIZE ) {
      ptr += n;
      n = size - FBUFF_SIZE;
      return fread( ptr, n, 1, inp_file );
    }
  } else {
    n = size <= fbuff_size ? size : fbuff_size;

    if( n > 0 ) {
      memcpy( ptr, fbuff, n );
      if( n < fbuff_size )
        memmove( fbuff, fbuff + n, fbuff_size - n );
      fbuff_size -= n;
      size -= n;
      ptr += n;
    }
    if( size > 0 ) {
      return fread( ptr, size, 1, inp_file );
    }
  }
  return 1;
}

#ifdef USE_ZLIB
int
feof_compr( FILE *f )
{
  if( fmf_compr ) {
    if( fmf_compr_feof )
      return 1;
    if( zstream.avail_in > 0 || zstream.avail_out != ZBUF_SIZE )
      return 0;
    fmf_compr_feof = feof( f );
    return fmf_compr_feof;
  } else {
    return feof( f );
  }
}

int fget_read = 0;

int
fgetc_compr( FILE *f )
{
  int s;

  if( fmf_compr ) {
    if( zstream.avail_out == ZBUF_SIZE ) {
      fget_read++;
      zstream.next_out = zbuf_o;
      do {
	if( zstream.avail_in == 0 && !fmf_compr_feof ) {
	  zstream.avail_in = fread( zbuf_i, 1, ZBUF_INP_SIZE, inp_file );
	  zstream.next_in = zbuf_i;
	  if( zstream.avail_in == 0 )
	    fmf_compr_feof = 1;
	}
	s = inflate( &zstream, fmf_compr_feof ? Z_FINISH : Z_SYNC_FLUSH );
      } while ( zstream.avail_out != 0 && s == Z_OK );
      zstream.next_out = zbuf_o;
    }
    if( zstream.avail_out == ZBUF_SIZE )	/* end of file */
      return -1;
    zstream.avail_out++;
    return *( zstream.next_out++ );    /* !!! we use it for own purpose */
  } else {
    return fgetc( f );
  }
}

int
fread_compr( void *buff, size_t n, size_t m, FILE *f )
{
  size_t i;
  int d;
  char *b = buff;

  if( fmf_compr ) {
    for( i = n * m; i > 0; i-- ) {
      if( ( d = fgetc_compr( f ) ) == -1 )	/****FIXME may too slow */
        return ( n * m - i ) / n;
      *b++ = d;
    }
    return m;
  } else {
    return fread( b, n, m, f );
  }
}
#else	/* USE_ZLIB */
#define fgetc_compr fgetc
#define fread_compr fread
#define feof_compr feof
#endif	/* USE_ZLIB */

libspectrum_dword
swap_endian_dword( libspectrum_dword d )
{
  return ( ( d & 0x000000ff ) << 24 ) |
	    ( ( d & 0x0000ff00 ) << 8 ) |
	      ( ( d & 0x00ff0000 ) >> 8 ) |
	        ( d >> 24 );
}

int
alloc_sound_buff( size_t len )
{
  if( sound_buf_len < len ) {
    len = ( ( len * 3 / 2 ) / SOUND_CHUNK_LEN + 1 ) * SOUND_CHUNK_LEN;

    if( sound_buf_len == 0 ) {
      sound8 = malloc( len );
    } else {
      sound8 = realloc( sound8, len );
    }
    if( sound8 == NULL ) {
      printe( "\n\nMemory (re)allocation error.\n" );
      return ERR_OUTOFMEM;
    }
    sound16 = (void *)sound8;
    sound_buf_len = len;
    printi( 3, "fmf_alloc_sound(): sound buffer length: %lu\n", (unsigned long)sound_buf_len );
  }
  return 0;
}

int
law_2_pcm( void )
{
  int err;
  libspectrum_signed_byte *s;
  libspectrum_signed_word *t;

  if( snd_t == TYPE_NONE ) return 0;

  if( ( err = alloc_sound_buff( snd_len * 2 ) ) ) return err;

  for( s = sound8 + snd_len - 1, t = sound16 + snd_len - 1 ;
		s >= sound8; s--, t-- ) {
    *t = alaw_table[*(libspectrum_byte*)s];
  }
#ifdef WORDS_BIGENDIAN
  snd_little_endian = 0;
#else
  snd_little_endian = 1;
#endif
  snd_enc = TYPE_PCM;
  snd_fsz <<= 1;		/* 1byte -> 2byte */
  snd_len <<= 1;		/* 1byte -> 2byte */

  printi( 3, "law_2_pcm(): %d sample converted\n", snd_len / 2 );

  return 0;
}

void
pcm_swap_endian( void )	/* buff == sound */
{
  libspectrum_word *s;
  int len = snd_len / 2;

  for( s = (void *)sound16; len > 0; len-- ) {
    *s = ( ( *s & 0x00ff ) << 8 ) | ( *s >> 8 );
    s++;
  }
  printi( 3, "pcm_swap_endian(): %d sample converted\n", snd_len / 2 );
}

/*
  [1][2][3][4][5] -> [1][2][3][4][5][ ][ ][ ][5][5][6][6] ->  [1][1][2][2][3][3][4][4][5][5][6][6]
*/
int
mono_2_stereo( void )
{
  int err;

  if( snd_t == TYPE_NONE ) return 0;

  if( ( err = alloc_sound_buff( snd_len * 2 ) ) ) return err;

  if( snd_enc == TYPE_PCM ) {
    libspectrum_signed_word *s;
    libspectrum_signed_word *t;
    for( s = sound16 + snd_len / 2 - 1, t = sound16 + snd_len - 1 ;
		s >= sound16; s--, t-- ) {
      *t = *s; t--; *t = *s;
    }
  } else {
    libspectrum_signed_byte *s;
    libspectrum_signed_byte *t;
    for( s = sound8 + snd_len - 1, t = sound8 + 2 * snd_len - 1 ;
		s >= sound8; s--, t-- ) {
      *t = *s; t--; *t = *s;
    }
  }
  snd_chn = 2;
  snd_fsz <<= 1;		/* 1/2byte -> 2/4byte */
  snd_len <<= 1;		/* 1/2byte -> 2/4byte */

  printi( 3, "mono_2_stereo(): %d sample converted (%d)\n", snd_len / snd_fsz, snd_len );

  return 0;
}

/*
  [1][1][2][2][3][3][4][4][5][5][6][6] -> [1][2][3][4][5][ ][ ][ ][5][5][6][6] -> [1][2][3][4][5]
*/
int
stereo_2_mono( void )
{
  void *buff_end;

  if( snd_t == TYPE_NONE ) return 0;

  if( snd_enc == TYPE_PCM ) {
    libspectrum_signed_word *s;
    libspectrum_signed_word *t;
    buff_end = sound16 + snd_len;
    for( s = t = sound16; (void *)s < buff_end; s++, t++ ) {
      *t = *s; s++; *t = ( (libspectrum_signed_dword)*t + *s + 1 ) / 2;
    }
  } else {
    libspectrum_signed_byte *s;
    libspectrum_signed_byte *t;
    buff_end = sound16 + snd_len;
    for( s = t = sound8; (void *)s < buff_end; s++, t++ ) {
      *t = *s; s++; *t = ( (libspectrum_signed_dword)*t + *s + 1 ) / 2;
    }
  }
  snd_chn = 1;
  snd_fsz >>= 1;		/* 1/2byte -> 2/4byte */
  snd_len >>= 1;		/* 1/2byte -> 2/4byte */

  printi( 3, "stereo_2_mono(): %d sample converted (%d)\n", snd_len / snd_fsz, snd_len );

  return 0;
}

/*
   [-/^^^^\______/-] ->

int
resample_sound( void )
{
  return 0;
}

*/

char *
find_filename_ext( const char *filename )
{
  char *extension = NULL;

  /* Get the filename extension, if it exists */
  if( filename ) {
    extension = strrchr( filename, '.' ); if( extension ) extension++;
  }
  return extension;
}

int
inp_get_cut_value( char *s, libspectrum_qword *value, type_t *type )
{
  int n = 0;
  libspectrum_qword frm = 0;

  char *tmp = strchr( s, ':' );
  if( tmp ) {		/*OK, min:sec */
    unsigned int sec = 0;
    if( ( sscanf( s, "%"PRIu64":%n%u", &frm, &n, &sec) != 2 ) || sec > 59 ) {	/* OK, unknown value.. */
      printe( "Bad interval definition in a -C/--cut option\n" );
      return 6;
    }
    frm = frm * 60 + sec;
    *value = frm;
    *type = TYPE_TIME;
    return 0;
  }
  if( ( sscanf( s, "%"PRIu64, &frm) != 1 ) ) {	/* OK, unknown value.. */
      printe( "Bad interval definition in a -C/--cut option\n" );
      return 6;
  }
  *value = frm;
  *type = TYPE_FRAME;
  return 0;
}

int
inp_get_next_cut( void )
{
  char *mns, *tmp;
  int err;

  cut_cut = 0;
  if( !out_cut || *out_cut == '\0' ) {	/* End of cut!!! */
    cut_cmd = TYPE_NONE;
    return 0;
  }

  tmp = strchr( out_cut, ',' );
  if( tmp )
    *(tmp++) = '\0';	/*Ok, we start the first intervall */

  mns = strchr( out_cut, '-' );	/* ff[:ss] */
  if( !mns ) {	/* there is no '-' one frame/sec cut out*/
    if( ( err = inp_get_cut_value( out_cut, &cut_frm, &cut_f_t ) ) ) return err;
    cut__to = cut_frm; cut_t_t = cut_f_t;
    cut_cmd = TYPE_CUT;
  } else if( out_cut == mns )	{ /* -ff[:ss]*/
    cut_frm = 0; cut_f_t = TYPE_FRAME;
    if( ( err = inp_get_cut_value( out_cut + 1, &cut__to, &cut_t_t ) ) ) return err;
    cut_cmd = TYPE_CUT;
  } else {	/* xx-xx or xx- */
    mns++;
    if( ( err = inp_get_cut_value( out_cut, &cut_frm, &cut_f_t ) ) ) return err;
    if( *mns == '\0' ) {	/* xxx- */
/*      if( ( err = inp_get_cut_value( out_cut + 1, &cut_frm, &cut_f_t ) ) ) return err; */
      cut__to = 0;
      cut_cmd = TYPE_CUTFROM;
    } else {
      if( ( err = inp_get_cut_value( mns, &cut__to, &cut_t_t ) ) ) return err;		/* xxx-yyy */
      cut_cmd = TYPE_CUT;
    }
  }
  out_cut = tmp;
  return 0;
}


/*
%[flags] [width] [.precision] [{h | l | ll | I | I32 | I64}]type
%[0 ][0-9]*<d|x|o|u>
%08d -> %08llu
*/

int
parse_outname( void )
{
  const char *f = out_name, *perc;
  char c1[2], c2[2];
  int n, len;

  perc = f;
  out_orig = out_name;
  while( f && *f ) {
    while( ( perc = strchr( f, '%' ) ) != NULL && 
           *( perc + 1 ) == '%' ) {
      f = perc + 1;
    }
    f = perc;
    if( !f ) break;
    if( sscanf( perc, "%%%1[0 ]%2u%1[dxXou]%n", c1, &n, c2, &len ) == 3 ) {
      break;
    } else if( sscanf( perc, "%%%2u%1[dxXou]%n", &n, c2, &len ) == 2 ) {
      c1[0] = '\0';
      break;
    } else if( sscanf( perc, "%%%1[dxXou]%n", c2, &len ) == 1 ) {
      n = 0; c1[0] = '\0';
      break;
    } else {
      f++;
    }
  }

  out_next = malloc( strlen( out_name ) + 24 );

  if( f == NULL ) {	/* there is no number template */
    char *ext;

    ext = find_filename_ext( out_name );
    if( ext == NULL ) {
      strcpy( out_tmpl, "-%08u" );
      out_nmbr = out_next + strlen( out_name );
      out_pfix = NULL;
    } else {
      strcpy( out_tmpl, "-%08u" );
      out_nmbr = out_next + ( ext - out_name - 1 );
      out_pfix = ext - 1;
    }
  } else {
    if( n > 20 ) n = 20;
    if( c2[0] == 'd' ) c2[0] = 'u';

    if( n )
      sprintf( out_tmpl, "%%%s%ull%c", c1, n, c2[0] );
    else
      sprintf( out_tmpl, "%%%sll%c", c1, c2[0] );

    out_nmbr = out_next + ( perc - out_name );
    out_pfix = perc + len;
  }
  printi( 2, "parse_outname(): Multiple output file No. template: %s.\n",
          out_tmpl );

  return 0;
}

int
next_outname( void )
{
  int err;

  if( !out_orig && ( err = parse_outname() ) ) return err;
  strcpy( out_next, out_orig );
  sprintf( out_nmbr, out_tmpl, output_no );
  if( out_pfix )
    strcat( out_next, out_pfix );
  out_name = out_next;
  printi( 3, "next_outname(): Next generated output filename: %s.", out_name );

  return 0;
}

int
open_out( void )
{
  char *ext;
  typedef struct {
    type_t type;
    const char *extension;
  } ext_t;

  int i, err;
  ext_t out_ext_tab[] = {
    { TYPE_YUV,  "yuv" },
    { TYPE_YUV,  "y4m" },
    { TYPE_SCR,  "scr"  },
    { TYPE_PPM,  "ppm"  },
    { TYPE_PPM,  "pnm"  },
    { TYPE_PNG,  "png" },
    { TYPE_JPEG, "jpg" },
    { TYPE_JPEG, "jpeg" },
    { TYPE_FFMPEG,"mpeg" },
    { TYPE_FFMPEG,"mpg" },
    { TYPE_FFMPEG,"avi" },
    { TYPE_FFMPEG,"mkv" },
    { 0, NULL }
  };
  const char *out_tstr[] = {
    "SCR - ZX Spectrum screenshot file",
    "PPM - Portable PixMap", "PNG - Portable Network Graphics",
    "JPEG - Joint Photographic Experts Group file format",
    "YUV4MPEG2 - uncompressed video stream format",
    "FFMPEG - ffmpeg file format"
  };

  if( do_info ) return 0;
  if( out_t == TYPE_NONE ) return 0;

  if( out_name && out_t == TYPE_UNSET ) {	/* try to identify the file type */
#ifdef USE_FFMPEG
    out_t = TYPE_FFMPEG;	/* default to FFMPEG */
#else
    out_t = TYPE_YUV;		/* default to YUV */
#endif
    ext = find_filename_ext( out_name );
    for( i = 0; ext != NULL && out_ext_tab[i].extension != NULL; i++ ) {
      if( !strcasecmp( ext, out_ext_tab[i].extension ) ) {
        out_t = out_ext_tab[i].type;
        break;
      }
    }
  }
  if( out_name && ( out_t >= TYPE_SCR && out_t <= TYPE_JPEG ) && 
      ( err = next_outname() ) > 0 ) return err;

  if( out_name
#ifdef USE_FFMPEG
		&& out_t != TYPE_FFMPEG
#endif
  ) {
    if( ( out = fopen( out_name, "wb" ) ) == NULL ) {
      printe( "Cannot open output file '%s'...\n", out_name );
      return ERR_OPEN_SND;
    }
  } else
#ifdef USE_FFMPEG
	 if( out_t != TYPE_FFMPEG )
#endif
  {

    if( isatty( fileno( stdout ) ) ) {
      out_t = TYPE_NONE;
      out = NULL;
      out_name = "(-=null=-)";
    } else {
      out_t = TYPE_YUV;		/* default to YUV */
      out = stdout;
      out_name = "(-=stdout=-)";
#ifdef WIN32
      setmode( fileno( stdout ), O_BINARY );
#endif				/* #ifdef WIN32 */
    }
  }

  if( out_t == TYPE_NONE ) {
    printi( 0, "open_out(): Output is a terminal, cannot dump binary data.\n" );
  } else if( !( out_t >= TYPE_SCR && out_t <= TYPE_JPEG ) ) {
    printi( 0, "open_out(): Output file (%s) opened as %s file.\n", out_name,
		out_tstr[out_t - TYPE_SCR] );
  } else {
    if( !output_no )
      printi( 0, "open_out(): Multiple output file (%s) as %s.\n", out_next,
		out_tstr[out_t - TYPE_SCR] );
    printi( 2, "open_out(): Output file (%s) opened as %s file.\n", out_name,
		out_tstr[out_t - TYPE_SCR] );
  }
  return 0;
}

int
open_snd( void )
{
  char *ext;
  typedef struct {
    type_t type;
    const char *extension;
  } ext_t;

  int i;
  ext_t snd_ext_tab[] = {
    { TYPE_WAV,  "wav" },
    { TYPE_AU,   "au"  },
    { TYPE_AU,   "snd" },
    { TYPE_AIFF, "aif" },
    { TYPE_AIFF, "aiff" },
    { TYPE_FFMPEG, "mp2" },
    { TYPE_FFMPEG, "mp3" },
    { TYPE_FFMPEG, "ogg" },
    { TYPE_FFMPEG, "aac" },
    { TYPE_FFMPEG, "ac3" },
    { TYPE_FFMPEG, "m4a" },
    { 0, NULL }
  };

  const char *snd_tstr[] = {
    "FFMPEG - ffmpeg file format",
    "WAV - MS Wave format",
    "AU - SUN OS audio file format",
    "AIF - MAC/OS audio file"
  };

  if( do_info ) return 0;

  if( snd_name ) {
    if( ( snd = fopen( snd_name, "wb" ) ) == NULL ) {
      printe( "Cannot open sound file '%s'...\n", snd_name );
      return ERR_OPEN_SND;
    }
  } else if( out_t == TYPE_FFMPEG ) {	/* FFMPEG video and audio if nothing else specified */
    snd_t = TYPE_FFMPEG;
    return 0;
  } else {
    snd_t = TYPE_NONE;
    return 0;
  }
  if( snd_t == TYPE_UNSET ) {	/* try to identify the file type */
    snd_t = TYPE_WAV;		/* default to WAV */
    ext = find_filename_ext( snd_name );
    for( i = 0; ext != NULL && snd_ext_tab[i].extension != NULL; i++ ) {
      if( !strcasecmp( ext, snd_ext_tab[i].extension ) ) {
        snd_t = snd_ext_tab[i].type;
        break;
      }
    }
  }
  printi( 0, "open_snd(): Sound file (%s) opened as %s file.\n", snd_name,
          snd_tstr[snd_t - TYPE_FFMPEG] );
  return 0;
}

int
open_inp( void )
{
  if( inp_file ) {				/* we have to close before last */
    if( inp_file != stdin ) {
      fclose( inp_file );
    } else {			/* reopen stdin???? */
      do_now = DO_EOP;
      return 0;
    }
  }
  if( inp_name ) {
    if( ( inp_file = fopen( inp_name, "rb" ) ) == NULL ) {
      printe( "Cannot open input file '%s'...\n", inp_name );
      return ERR_OPEN_INP;
    }
  } else if( out_t == TYPE_FFMPEG ) {	/* FFMPEG video and audio if nothing else specified */
    snd_t = TYPE_FFMPEG;
    return 0;
  } else {
    inp_file = stdin;
    inp_name = "(-=stdin=-)";
  }
  if( fseek( inp_file, 0, SEEK_END ) == -1 ) {
    printi( 4, "open_inp(): cannot seek to the end of the input file (not seekable).\n" );
    inp_ftell = 0;
  } else if( ( inp_ftell = ftell( inp_file ) ) == -1 ) {
    printi( 4, "open_inp(): cannot ftell input file??.\n" );
    fseek( inp_file, 0, SEEK_SET );
    inp_ftell = 0;
  } else {
    fseek( inp_file, 0, SEEK_SET );
  }

  if( inp_t == TYPE_UNSET ) {	/* try to identify the file type */
    fread_buff( fhead, 4, INTO_BUFF );		/* check file type */
    if( memcmp( fhead, "FMF_", 4 ) == 0 )
      inp_t = TYPE_FMF;
    else
      inp_t = TYPE_SCR;
  }
  do_now = inp_t == TYPE_FMF ? DO_HEAD : DO_SLICE;
  printi( 0, "open_inp(): Input file (%s) opened as %s file.\n", inp_name,
          inp_t == TYPE_FMF ? "FMF" : "SCR" );
  return 0;
}

void
setup_frame_wh( void )
{
  if( ( scr_t = fhead[5] ) == TYPE_HRE ) {	/* screen type => $ R C X */
    if( frm_w != 640 ) {
      frm_w = 640; frm_h = 480;
      pix_yuv[1] = &pix_rgb[ 640 * 480 ];
      pix_yuv[2] = &pix_rgb[ 640 * 480 * 2 ];
    }
  } else {
    if( frm_w != 320 ) {
      frm_w = 320; frm_h = 240;
      pix_yuv[1] = &pix_rgb[ 320 * 240 ];
      pix_yuv[2] = &pix_rgb[ 320 * 240 * 2 ];
    }
  }
  yuv_uvlen = yuv_ylen = frm_w * frm_h;
}

int
check_fmf_head( void )
{
  if( memcmp( fhead, "FMF_", 4 ) ) {
    printe( "Not a Fuse Movie File '%s'\n", inp_name );
    return ERR_CORRUPT_INP;
  }
  if( fread_buff( fhead, 12, FROM_BUFF ) != 1 ||
	fhead[0] != 'V' ||
	fhead[1] != '1' ||	/* V1 */
	( fhead[2] != 'e' && fhead[2] != 'E' ) ||		/* endianness */
	( fhead[3] != 'U' && fhead[3] != 'Z' ) )		/* compressed? */
  {
    printe("This version of Fuse Movie File not supported, sorry...\n");
    return ERR_VERSION_INP;
  }
  fmf_little_endian = fhead[2] == 'e' ? 1 : 0;
  fmf_compr = fhead[3] == 'Z' ? 1 : 0;

#ifndef USE_ZLIB
  if( fmf_compr ) {				/* error, if no zlib */
    printe( "Sorry, no zlib compressed FMF file support in this binary!\n" );
    return ERR_NO_ZLIB;
  }
#endif	/* USE_ZLIB */
/* Check initial frame parameters */
  if( fhead[4] < 1 ) {
    printe( "Wrong Frame rate '%d', sorry...\n", fhead[4] );
    return ERR_CORRUPT_INP;
  }
  if( fhead[5] != TYPE_ZXS && fhead[5] != TYPE_TXS && fhead[5] != TYPE_HRE &&
      fhead[5] != TYPE_HCO ) {
    printe( "Unknown Screen$ type '%d', sorry...\n", fhead[5] );
    return ERR_CORRUPT_INP;
  }
  if( fhead[6] < 'A' || fhead[6] > 'E' ) {
    printe( "Unknown Machine type '%d', sorry...\n", fhead[6] );
    return ERR_CORRUPT_INP;
  }
  frm_rte = fhead[4];				/* frame rate (1:#) */
  setup_frame_wh();
  frm_mch = fhead[6];				/* machine type */

  inp_fps = machine_timing[frm_mch - 'A'] / frm_rte;	/* real input fps * 1000 frame / 1000s */
  if( out_fps == 0 ) out_fps = inp_fps;	/* later may change */

/* Check initial sound parameters */
  if( fhead[7] != 'P' && fhead[7] != 'A' ) {
    printe( "Unknown FMF sound encoding type '%d', sorry...\n", fhead[7] );
    return ERR_CORRUPT_INP;
  }
  if( fhead[8] + ( fhead[9] << 8 ) < 1 ) {
    printe( "Wrong FMF sound rate '%d', sorry...\n", fhead[8] + ( fhead[9] << 8 ) );
    return ERR_CORRUPT_INP;
  }
  if( fhead[10] != 'M' && fhead[10] != 'S' ) {
    printe( "Unknown FMF sound channels type '%d', sorry...\n", fhead[10] );
    return ERR_CORRUPT_INP;
  }

  snd_enc = fhead[7];		/* P U A */
  snd_rte = fhead[8] + ( fhead[9] << 8 );
  snd_chn = fhead[10] == TYPE_STEREO ? 2 : 1;
  snd_fsz = ( snd_enc == TYPE_PCM ? 2 : 1 ) * snd_chn;
  if( snd_enc == TYPE_PCM ) snd_little_endian = fmf_little_endian;

  if( out_rte == -1 ) out_rte = snd_rte;
  if( out_chn == -1 ) out_chn = snd_chn;
  do_now = DO_SLICE;
  printi( 1, "check_fmf_head(): file:  FMF V1 %s endian %scompressed.\n",
          fmf_little_endian ? "little" : "big", fmf_compr ? "" : "un" );
  printi( 1, "check_fmf_head(): video: frame rate = 1:%d frame time: %dus %s machine timing.\n", frm_rte, machine_ftime[frm_mch - 'A'],
          machine_name[frm_mch - 'A'] );
  printi( 1, "check_fmf_head(): audio: sampling rate %dHz %c encoded %s sound.\n",
          snd_rte, snd_enc, snd_chn == 2 ? "stereo" : "mono" );
  return 0;
}

int
fmf_read_head( void )
{
  fread_buff( fhead, 4, FROM_BUFF );
  return check_fmf_head();
}

int
fmf_read_frame_head( void )
{
  if( fread_compr( fhead, 3, 1, inp_file ) != 1 ) {
    printe( "\n\nfmf_read_frame_head(): Corrupt input file (N) @0x%08lx.\n",
            (unsigned long)ftell( inp_file ) );
    return ERR_CORRUPT_INP;
  }
  if( fhead[0] < 1 ) {
    printe( "Wrong Frame rate '%d', sorry...\n", fhead[0] );
    return ERR_CORRUPT_INP;
  }
  if( fhead[1] != TYPE_ZXS && fhead[1] != TYPE_TXS && fhead[1] != TYPE_HRE &&
      fhead[1] != TYPE_HCO ) {
    printe( "Unknown Screen$ type '%d', sorry...\n", fhead[1] );
    return ERR_CORRUPT_INP;
  }
  if( fhead[2] < 'A' || fhead[2] > 'E' ) {
    printe( "Unknown Machine type '%d', sorry...\n", fhead[2] );
    return ERR_CORRUPT_INP;
  }

  setup_frame_wh();

  if( frm_rte != fhead[0] || frm_mch != fhead[2] ) {	/* recalculate timings */
    frm_rte = fhead[0];				/* frame rate (1:#) */
    frm_mch = fhead[2];				/* machine type */
    inp_fps = machine_timing[frm_mch - 'A'] / frm_rte;	/* real input fps * 1000 frame / 1000s */
    if( out_fps == 0 ) out_fps = inp_fps;	/* later may change */
  }
  do_now = DO_SLICE;
  frame_no++;
  time_frm += machine_ftime[frm_mch - 'A'] * frm_rte;
  while( time_frm >= 1000000 ) {
    time_frm -= 1000000;
    time_sec++;
  }
  printi( 2, "fmf_read_frame_head(): rate = 1:%d frame time: %dus %s machine.\n",
          frm_rte, machine_ftime[frm_mch - 'A'], machine_name[frm_mch - 'A'] );
  return 0;
}

int
fmf_read_chunk_head( void )
{
  if( fread_compr( fhead, 1, 1, inp_file ) != 1 ) {	/* */
    printe( "Unexpected end of input file @0x%08lx.\n",
            (unsigned long)ftell( inp_file )  );
    return ERR_ENDOFFILE;
  }
  if( fhead[0] != 'N' && fhead[0] != '$' && fhead[0] != 'S' && fhead[0] != 'X' ) {
    printe( "Unknown FMF chunk type '%d', sorry...\n", fhead[0] );
    return ERR_CORRUPT_INP;
  }
  printi( 2, "fmf_read_chunk_head(): Chunk: %c - %s\n", fhead[0], fhead[0] == 'N' ? "(N)ew frame" :
				( fhead[0] == '$' ? "($)lice": ( fhead[0] == 'S' ? "(S)ound" : 
				  ( fhead[0] == 'X' ? "End of recording" : "\???" ) ) ) );
  return 0;
}

int
fmf_read_init( void )		/* read first N */
{
  int err;
  if( ( err = fmf_read_chunk_head() ) ) return err;

  if( fhead[0] != 'N' ) {				/* end of record... */
    printe( "\n\nfmf_read_init(): Corrupt input file (N) @0x%08lx.\n", (unsigned long)ftell( inp_file ) );
    return ERR_CORRUPT_INP;
  }
  return fmf_read_frame_head();
}

int
fmf_read_sound( void )
{
  int err;
  int len;
  static int fmf_snd_head_read = 0;

  if( !fmf_snd_head_read && fread_compr( fhead, 6, 1, inp_file ) != 1 ) {
    printe( "\n\nCorrupt input file (S) @0x%08lx.\n", (unsigned long)ftell( inp_file ) );
    return ERR_CORRUPT_INP;
  }

  fmf_snd_head_read = 1;
  if( fhead[0] != 'P' && fhead[0] != 'A' ) {
    printe( "Unknown FMF sound encoding type '%d', sorry...\n", fhead[0] );
    return ERR_CORRUPT_INP;
  }
  if( fhead[1] + ( fhead[2] << 8 ) < 1 ) {
    printe( "Wrong FMF sound rate '%d', sorry...\n", fhead[1] + ( fhead[2] << 8 ) );
    return ERR_CORRUPT_INP;
  }
  if( fhead[3] != 'M' && fhead[3] != 'S' ) {
    printe( "Unknown FMF sound channels type '%d', sorry...\n", fhead[3] );
    return ERR_CORRUPT_INP;
  }

  if( snd_len && ( snd_enc != fhead[0] ||		/* encoding change */
		snd_rte != fhead[1] + ( fhead[2] << 8 ) || /* sampling rate change */
		snd_chn != ( fhead[3] == TYPE_STEREO ? 2 : 1 ) ) ) { /* channels change */
    printi( 3, "fmf_read_sound(): sound parameters changed flush sound buffer\n" );
    do_now = DO_SOUND_FLUSH;
    return 0;
  }

  fmf_snd_head_read = 0;
  snd_enc = fhead[0];		/* P U A */
  snd_rte = fhead[1] + ( fhead[2] << 8 );
  snd_chn = fhead[3] == TYPE_STEREO ? 2 : 1;
  snd_fsz = ( snd_enc == TYPE_PCM ? 2 : 1 ) * snd_chn;
  len = ( fhead[4] + ( fhead[5] << 8 ) + 1 ) * snd_fsz;

  if( sound_stereo == -1 ) out_chn = snd_chn;
  if( ( err = alloc_sound_buff( snd_len + len ) ) ) return err;
  if( fread_compr( (void *)( sound8 + snd_len ), len, 1, inp_file ) != 1 ) {
    printe( "\n\nCorrupt input file (S) @0x%08lx.\n", (unsigned long)ftell( inp_file ) );
    return ERR_CORRUPT_INP;
  }
  fmf_sound_no++;
  if( !cut_cut ) snd_len += len;
  if( snd_len >= SOUND_BUFF_MAX_SIZE ) {
    printi( 3, "fmf_read_sound(): flush sound buffer because full\n" );
    do_now = DO_SOUND;
    return 0;
  }
  printi( 3, "fmf_read_sound(): %s %dHz %c encoded %s %d samples (%d bytes) sound\n", cut_cut ? "skip" : "store", snd_rte, snd_enc,
		 snd_chn == 2 ? "stereo" : "mono", len/snd_fsz, len );
  return 0;
}

int
snd_write_sound( void )
{
  int err;

  if( snd_len <= 0 ) return 0;

  if( snd_enc != TYPE_PCM && sound_pcm && ( err = law_2_pcm() ) ) return err;
  if( snd_chn == 1 && sound_stereo == 1 && ( err = mono_2_stereo() ) ) return err;
  if( snd_chn == 2 && sound_stereo == 0 && ( err = stereo_2_mono() ) ) return err;

  if( snd_t == TYPE_WAV )
    err = snd_write_wav();
  else if( snd_t == TYPE_AU )
    err = snd_write_au();
  else if( snd_t == TYPE_AIFF )
    err = snd_write_aiff();
#ifdef USE_FFMPEG
  else if( snd_t == TYPE_FFMPEG )
    err = snd_write_ffmpeg();
#endif
  snd_len = 0;
  if( err ) return err;

  return 0;
}

int
fmf_gen_sound( void )
{
  int err;
  static libspectrum_word len_frag = 0;
  libspectrum_qword len;

  if( snd_t == TYPE_NONE ) return 0;

/* spectrum frame length * frame rate * freq * framesize 20000 * 9 * 64000 * 4 ~ 5*10^10 so we need 64bit! */
  len = (libspectrum_qword)len_frag + (libspectrum_qword)machine_ftime[frm_mch - 'A'] * frm_rte * snd_rte * snd_fsz;
  len_frag = len - (len / 1000000) * 1000000;
  len = len / 1000000;

  if( ( err = alloc_sound_buff( snd_len + len ) ) ) return err;
  memset((void *)( sound8 + snd_len ), snd_enc == 'P' ? 0 : 0xd5, len);	/* only A-Law!!!*/
  snd_len += len;
  printi( 3, "fmf_gen_sound(): store %dHz %c encoded %s %d samples (%d bytes) silence\n", snd_rte, snd_enc,
		 snd_chn == 2 ? "stereo" : "mono", (int)len/snd_fsz, (int)len );
  if( snd_len >= SOUND_BUFF_MAX_SIZE ) {
    if( ( err = snd_write_sound() ) ) return err;
    if( do_info ) snd_len = 0;			/* anyhow, we have to empty sound buffer */
    printi( 3, "fmf_gen_sound(): flush sound buffer because full\n" );
  }
  return 0;
}

void
close_snd( void )
{
  snd_write_sound(); /* write pending sound data */
  if( snd_t == TYPE_WAV )
    snd_finalize_wav();
  else if( snd_t == TYPE_AU )
    snd_finalize_au();
  else if( snd_t == TYPE_AIFF )
    snd_finalize_aiff();

  if( !snd ) return;
  fclose( snd );
  snd = NULL;
}

int
fmf_read_slice_blokk( libspectrum_byte *scrl, int w, int h )
{
  int last = -1, d = -1, width = w, len = 0, i;
  libspectrum_byte *scrp;

  for( ; h > 0; h--, scrl += SCR_PITCH ) {	/* every line  */
    scrp = scrl;
    w = width;			/* restore width */
    while( w > 0 ) {
      if( len == 0 ) {
        if( ( d = fgetc_compr( inp_file ) ) == -1 ) {
          printe( "\n\nCorrupt input file ($) @0x%08lx.\n", (unsigned long)ftell( inp_file ) );
          return ERR_CORRUPT_INP;	/* read a byte */
        }
        if( d == last ) {	/* compressed chunk... */
	  if( ( len = fgetc_compr( inp_file ) ) == -1 ) {
	    printe( "\n\nCorrupt input file ($) @0x%08lx.\n", (unsigned long)ftell( inp_file ) );
	    return ERR_CORRUPT_INP;	/* read a byte */
	  }
	  len++;
	  last = -1;	/* 0 - 255 */
        } else {
	  len = 1, last = d;
        }
      }
      if( len > w ) i = w; else i = len;
      len -= i;
      w -= i;
      for(; i > 0; i-- )
        *scrp++ = d;		/* save data */
    }
  }
  return 0;
}

int
fmf_read_screen( void )
{
  int err;

  if( fread_compr( fhead, 6, 1, inp_file ) != 1 ) {
    printe( "\n\nCorrupt input file ($) @0x%08lx.\n", (unsigned long)ftell( inp_file ) );
    return ERR_CORRUPT_INP;
  }
  if( fhead[0] > 39 ) {
    printe( "Wrong FMF slice X position '%d', sorry...\n", fhead[0] );
    return ERR_CORRUPT_INP;
  }
  if( fhead[1] + ( fhead[2] << 8 ) > 239 ) {
    printe( "Wrong FMF slice Y position '%d', sorry...\n", fhead[1] + ( fhead[2] << 8 ) );
    return ERR_CORRUPT_INP;
  }
  if( fhead[3] - 1 > 39 ) {
    printe( "Wrong FMF slice Width '%d', sorry...\n", fhead[3] );
    return ERR_CORRUPT_INP;
  }
  if( fhead[4] + ( fhead[5] << 8 ) - 1 > 239 ) {
    printe( "Wrong FMF slice Height '%d', sorry...\n", fhead[4] + ( fhead[5] << 8 ) );
    return ERR_CORRUPT_INP;
  }

  frm_slice_x = fhead[0];
  frm_slice_y = fhead[1] + ( fhead[2] << 8 );
  frm_slice_w = fhead[3];
  frm_slice_h = fhead[4] + ( fhead[5] << 8 );

/* read bitmap1 */
/*	fprintf( stderr, "$0 0x%08x: %d,%d %dx%d\n", (unsigned int) ftell( scr ) - 7, fmf_x, fmf_y ,fmf_w, fmf_h ); */
  if( ( err = fmf_read_slice_blokk( zxscr + frm_slice_y * SCR_PITCH + frm_slice_x,
			       frm_slice_w, frm_slice_h ) ) ) return err;	/* we store it with cont. */
/* read bitmap2 if HighResolution Timex */
  if( scr_t == TYPE_HRE && ( err = fmf_read_slice_blokk( zxscr + 9600 +
			     frm_slice_y * SCR_PITCH + frm_slice_x,
			     frm_slice_w, frm_slice_h ) ) ) return err;	/* we store it with cont. */
/*read attrib */
/*	fprintf( stderr, "Attr 0x%08x: %d,%d %dx%d\n", (unsigned int) ftell( scr ) - 7, fmf_x, fmf_y ,fmf_w, fmf_h ); */
  if( ( err = fmf_read_slice_blokk( attrs + frm_slice_y * SCR_PITCH + frm_slice_x,
			       frm_slice_w, frm_slice_h ) ) ) return err;
/*	fprintf(stderr, "next slice@0x%08x\n", (unsigned int)ftell(scr)); */
  fmf_slice_no++;
  printi( 3, "fmf_read_screen(): x=%d, y=%d, w=%d, h=%d\n", frm_slice_x, frm_slice_y, frm_slice_w, frm_slice_h );
  return 0;
}

int
fmf_read_slice( void )
{
  int err = 0;
  if( ( err = fmf_read_chunk_head() ) ) return err;

  if( fhead[0] == 'N' ) {		/* end of frame, start new frame! */
    do_now = DO_FRAME;

/*
we have to handle cut here
*/
  if( cut_cmd != TYPE_NONE ) {
    if( cut_cmd == TYPE_CUT && ( cut_t_t == TYPE_FRAME ? frame_no > cut__to : time_sec > cut__to ) )
      inp_get_next_cut();
    if( ( cut_f_t == TYPE_FRAME ? frame_no >= cut_frm : time_sec >= cut_frm ) &&
	( cut_cmd == TYPE_CUTFROM || ( cut_t_t == TYPE_FRAME ? frame_no <= cut__to : time_sec <= cut__to ) ) ) {
      drop_no++;
      cut_cut = 1;
    }
  }

  if( sound_exists == 0 && !cut_cut ) err = fmf_gen_sound();	/* generate silence */
    sound_exists = 0;
  } else if( fhead[0] == 'X' ) {
    do_now = DO_LAST_FRAME;
  } else if( fhead[0] == 'S' ) {
    sound_exists = 1;			/* found a sound chunk */
    return fmf_read_sound();
  } else {				/* read screen */
    return fmf_read_screen();
  }
  return err;
}

int
scr_read_scr( void )
{
  return 0;
}

void
out_2_yuv( void )
{

  libspectrum_byte *bitmap, *attr;
  int i, w, idx;
  int Y, U, V;
  int x = frm_slice_x, y = frm_slice_y, h = frm_slice_h;
  int inv = ( frame_no * frm_rte % 32 ) > 15 ? 1 : 0;

  w = frm_slice_w;
  for( h = frm_slice_h; h > 0; h--, y++, w = frm_slice_w ) {
    if( scr_t != TYPE_HRE )
      idx = y * OUT_PITCH + x * 8 ;		/* 8pixel/data */
    else
      idx = y * OUT_PITCH * 2 + x * 16 ;	/* 16 pixel/data*/
    bitmap = &zxscr[SCR_PITCH * y + x];
    attr   = &attrs[SCR_PITCH * y + x];
    for( ; w > 0; w--, bitmap++, attr++ ) {
      int px, fx, ix = *attr;
      int bits = scr_t == TYPE_HRE ? ( *bitmap << 8 ) + *(bitmap + 9600) : *bitmap;

      fx = ( ix & 0x80 ) * inv;
      px = ( ( ix & 0x38 ) >> 3 ) + ( ( ix & 0x40 ) ? 8 : 0 );
      ix = ( ix & 0x07 ) + ( ( ix & 0x40 ) ? 8 : 0 );
      for( i = ( scr_t == TYPE_HRE ? 16 : 8 ); i > 0; i-- ) {
        if( ( bits ^ fx ) & 128 ) {	/* ink */
          Y = yuv_pal[ ix * 3 + 0 ];
          U = yuv_pal[ ix * 3 + 1 ];
          V = yuv_pal[ ix * 3 + 2 ];
        } else {			/* paper */
          Y = yuv_pal[ px * 3 + 0 ];
          U = yuv_pal[ px * 3 + 1 ];
          V = yuv_pal[ px * 3 + 2 ];
        }
        pix_yuv[0][idx]   = Y;
        pix_yuv[1][idx]   = U;
        pix_yuv[2][idx++] = V;
        bits <<= 1;
      }
    }
  }
}

void
out_2_rgb( void )
{
  libspectrum_byte *bitmap, *attr;
  int i, w, idx;
  int R, G, B;
  int x = frm_slice_x, y = frm_slice_y, h = frm_slice_h;
  int inv = ( frame_no * frm_rte % 32 ) > 15 ? 1 : 0;

  w = frm_slice_w;
  for( h = frm_slice_h; h > 0; h--, y++, w = frm_slice_w ) {
    if( scr_t != TYPE_HRE )
      idx = y * OUT_PITCH + x * 8 ;		/* 8pixel/data */
    else
      idx = y * OUT_PITCH * 2 + x * 16 ;	/* 16 pixel/data*/

    idx *= 3;

    bitmap = &zxscr[SCR_PITCH * y + x];
    attr   = &attrs[SCR_PITCH * y + x];
    for( ; w > 0; w--, bitmap++, attr++ ) {
      int px, fx, ix = *attr;
      int bits = scr_t == TYPE_HRE ? ( *bitmap << 8 ) + *(bitmap + 9600) : *bitmap;

      fx = ( ix & 0x80 ) * inv;
      px = ( ( ix & 0x38 ) >> 3 ) + ( ( ix & 0x40 ) ? 8 : 0 );
      ix = ( ix & 0x07 ) + ( ( ix & 0x40 ) ? 8 : 0 );
      for( i = scr_t == TYPE_HRE ? 16 : 8; i > 0; i-- ) {
        if( ( bits ^ fx ) & 128 ) {	/* ink */
          R = rgb_pal[ ix * 3 + 0 ];
          G = rgb_pal[ ix * 3 + 1 ];
          B = rgb_pal[ ix * 3 + 2 ];
        } else {			/* paper */
          R = rgb_pal[ px * 3 + 0 ];
          G = rgb_pal[ px * 3 + 1 ];
          B = rgb_pal[ px * 3 + 2 ];
        }
        pix_rgb[idx++] = R;
        pix_rgb[idx++] = G;
        pix_rgb[idx++] = B;
        bits <<= 1;
      }
    }
  }
}

int
out_write_frame( void )
{
  int err;
  int add_frame = 0;
  int n = -frm_fps / inp_fps;

  if( frm_fps > 0 ) {
    drop_no++;
    printi( 2, "out_write_frame(): drop this frame.\n" );
  }

  if( cut_cut ) {
    drop_no++;
    printi( 2, "out_write_frame(): cut this frame.\n" );
    return 0;
  }

  while( frm_fps <= 0 ) {
    if( n > 1 ) {		/* we have to add sound in more part */
    } else {
      snd_write_sound();
    }
    if( add_frame ) {
      printi( 2, "out_write_frame(): add this frame again.\n" );
      dupl_no++;
    } else {
      printi( 2, "out_write_frame(): add frame.\n" );
      add_frame = 1;
    }
    if( out_t == TYPE_YUV ) {
      if( ( err = out_write_yuv() ) ) return err;
#ifdef USE_FFMPEG
    } else if( out_t == TYPE_FFMPEG ) {
      if( ( err = out_write_ffmpeg() ) ) return err;
#endif
    } else if( out_t == TYPE_SCR ) {
      if( ( err = out_write_scr() ) ) return err;
    } else if( out_t == TYPE_PPM ) {
      if( ( err = out_write_ppm() ) ) return err;
    }
    frm_fps += inp_fps;
    output_no++;
    if( ( out_t >= TYPE_SCR && out_t <= TYPE_JPEG ) ) {
      close_out();
      open_out();
    }
  }
  frm_fps -= out_fps;

  return 0;
}

void
close_out( void )
{
#ifdef USE_FFMPEG
  if( out_t == TYPE_FFMPEG )
    out_finalize_ffmpeg();
#endif

  if( !out ) return;
  fclose( out );
  out = NULL;
}


int
prepare_next_file( void )		/* multiple input file */
{
  if( input_no > input_last_no )
    do_now = DO_EOP;		/* no more file */
  else
    input_no++;
  return 0;
}

void
print_help( void )
{
  printf ("\n"
	  "Usage: fmfconv [options] [infile [outfile [soundfile]]]\n"
	  "\n"
	  "Options:\n"
	  "\n"
	  "  -i --input <filename>        Input file\n"
	  "  -o --output <filename>       Output file\n"
	  "  -s --sound <filename>        Output sound file\n"
	  "     --sound-only              Process only the sound from an 'fmf' file\n"
	  "     --mono                    Convert sound to mono (by default sound\n"
	  "                                 is converted to stereo)\n"
	  "     --raw-sound               Do not convert sound to 16 bit signed PCM and\n"
	  "                                 STEREO or MONO\n"
	  "                                 This is an advanced option. If stereo/mono or\n"
	  "                                 audio encoding change through 'fmf' file, your\n"
	  "                                 sound will be crappy.\n"
#ifdef USE_FFMPEG
	  "                                 note: FFMPEG needs PCM audio without change\n"
	  "                                 the channel number too.\n"
#endif /* USE_FFMPEG */
	  "  -w --wav                     Save sound to Microsoft audio (wav) file\n"
	  "  -u --au                      Save sound to Sun Audio (au) file\n"
	  "  -m --aiff                    Save sound to Apple Computer audio (aiff/aiff-c)\n"
	  "                                 file\n"
	  "     --aifc                    Force AIFF-C output if sound format is AIFF\n"
	  "  -Y --yuv                     Save video as yuv4mpeg2.\n"
	  "     --yuv-format <frm>        Set yuv4mpeg2 file frame format to 'frm',\n"
	  "                                 where 'frm' is one of '444', '422',\n"
	  "                                 '420j', '420m', '420' or '410'.\n"
	  "  -S --scr                     Save video as SCR screenshots\n"
	  "  -P --ppm                     Save video as PPM screenshots\n"
	  "  -f --frate <timing>          Set output frame rate. 'timing' is 'pal',\n"
	  "                                 'ntsc', 'movie' or a number with a maximum\n"
	  "                                 of 3 digits after the decimal point, or a #/#\n"
	  "                                 (e.g.: -f 29.97 or -f 30000/1001)\n"
	  "  -g --progress <form>         Show progress, where <form> is one of '%%', 'bar'\n"
	  "                                 'frame' or 'time'. frame and time are similar\n"
	  "                                 to bar and also show movie seconds or frame\n"
	  "                                 number.\n"
	  "  -C --out-cut <cut>           Leave out the comma delimited 'cut' ranges\n"
	  "                                 e.g.: 100-200,300,500,1:11-2:22 cut the frames\n"
	  "                                 100-200, 300, 500 and frames from 1min 11sec\n"
	  "                                 to 2min 22sec (in the given timing see:\n"
	  "                                 -f/--frate)\n"
#ifdef USE_FFMPEG
	  "  -X --ffmpeg                  Save video and audio as FFMPEG file (default)\n"
	  "  -p --profile <profile>       Select the profile for FFMPEG output where <profile>\n"
	  "                                 is 'youtube', 'dvd' or 'ipod'\n"
	  "  -F --format <format>         Select file format for FFMPEG output (by default\n"
	  "                                 the filename extension determines the format)\n"
	  "  -c --vcodec <codec>          Select video codec for FFMPEG output (by default\n"
	  "                                 file format determine)\n"
	  "  -a --acodec <codec>          Select audio codec for FFMPEG output (by default\n"
	  "                                 file format determine)\n"
	  "  -A --arate <rate>            Select audio bitrate for FFMPEG output (by\n"
	  "                                 default audio codec determine) where <rate> is\n"
	  "                                 'default' or a number\n"
	  "  -r --vrate <rate>            Select video bitrate for FFMPEG output (by\n"
	  "                                 default video codec determine) where <rate> is\n"
	  "                                 'default' or 'ffdefault' or a number\n"
	  "  -R --resize <res>            Resize video frame where <res> is 'vga' for\n"
	  "                                 640x480, 'hvga' for 480x360, 'qvga' for\n"
	  "                                 320x240, 'pal' for 768x576 (this set frame\n"
	  "                                 rate to 25 also) or WxH, Sx, N/Mx or W\n"
	  "  -E --srate <rate>            Resample audio to <rate> sampling rate where\n"
	  "                                 <rate> is 'cd' for 44100 or 'dat' for 48000 or\n"
	  "                                 a number. ('cd' and 'dat' set 'stereo' also)\n"
	  "     --formats                 List available FFMPEG formats (see -F/--format)\n"
	  "     --vcodecs                 List available FFMPEG video codecs (see\n"
	  "                                 -c/--vcodec)\n"
	  "     --acodecs                 List available FFMPEG audio codecs (see\n"
	  "                                 -a/--acodec)\n"
#endif
	  "     --info                    Scan input file(s) and print information.\n"
	  "  -v --verbose                 Increase the verbosity level by one.\n"
	  "  -q --quiet                   Decrease the verbosity level by one.\n"
	  "  -V --version                 Print the version number and exit.\n"
	  "  -h --help                    Print this help\n\n"
  );
}

void
print_progress( int force )
{
  static long int fpos = 0;
  long int npos;
  int perc;
  static int last_perc = -1;
  const char *bar = "##############################################################################";
  const char *spc = "                                                                              ";

  if( !inp_ftell ) return;

  npos = ftell( inp_file );
  if( npos <= fpos ) return;

  perc = (libspectrum_signed_qword)npos * 1009 / inp_ftell / 10;

  if( !force && last_perc == perc ) return;
  if( perc > 100 ) perc = 100;

  if( prg_t == TYPE_PERC )
    fprintf( stderr, "Percent: %4d%%\r", perc );
  else if( prg_t == TYPE_BAR )
    fprintf( stderr, "[%s%s]\r", bar + 78 - 78 * perc / 100, spc + 78 * perc / 100 );
  else if( prg_t == TYPE_FRAME )
    fprintf( stderr, "[%s%s]%8"PRIu64"\r", bar + 78 - 70 * perc / 100, spc + 8 + 70 * perc / 100, frame_no );
  else if( prg_t == TYPE_TIME )
    fprintf( stderr, "[%s%s]%8"PRIu64"\r", bar + 78 - 70 * perc / 100, spc + 8 + 70 * perc / 100, time_sec );
  last_perc = perc;
}

int
parse_args( int argc, char *argv[] )
{
  struct option long_options[] = {
    {"input", 1, NULL, 'i'},
    {"output", 1, NULL, 'o'},
    {"sound", 1, NULL, 's'},
    {"sound-only", 0, NULL, 0x103},
    {"mono",   0, &sound_stereo, 0},		/* set mono */
    {"raw-sound",0, &sound_raw, 1},		/* do not convert fmf sound to PCM_S16LE */
/*
    {"no-sound", 0, &video_only, 1},
*/
    {"wav",    0, NULL, 'w'},		/* save .wav sound */
    {"au",     0, NULL, 'u'},		/* save .au sound */
    {"aiff",   0, NULL, 'm'},		/* save .aiff sound */
    {"aifc",   0, NULL, 0x102},		/* force aifc */
/*
    {"simple-height", 0, &simple_height, 1},
    {"no-aspect", 0, &simple_height, -1},
*/
    {"scr",    0, NULL, 'S'},		/* extract as SCR */
    {"ppm",    0, NULL, 'P'},		/* extract as PPM */
    {"yuv",    0, NULL, 'Y'},		/* YUV output */
    {"yuv-format", 1, NULL, 0x101},
    {"frate",  1, NULL, 'f'},		/* video frame rate */
    {"progress",1,NULL, 'g'},		/* progress 'bar' */
    {"cut",    1, NULL, 'C'},		/*TODO cut */
#ifdef USE_FFMPEG
    {"ffmpeg", 0, NULL, 'X'},		/* select FFMPEG output */
    {"profile",1 ,NULL, 'p'},		/* output profile */
    {"format", 1, NULL, 'F'},		/* file format */
    {"acodec", 1, NULL, 'a'},		/* audio codec */
    {"vcodec", 1, NULL, 'c'},		/* video codec */
    {"arate",  1, NULL, 'A'},		/* audio bitrate */
    {"vrate",  1, NULL, 'r'},		/* video bitrate */
    {"resize", 1, NULL, 'R'},		/* resize video */
    {"srate",  1, NULL, 'E'},		/* audio new samplerate cd/dat set 'stereo' also */
    {"formats",0, &ffmpeg_list, 0},	/* list formats */
    {"acodecs",0, &ffmpeg_list, 1},	/* list formats */
    {"vcodecs",0, &ffmpeg_list, 2},	/* list formats */
#endif
    {"info", 0, &do_info, 1},

    {"help", 0, NULL, 'h'},
    {"version", 0, NULL, 'V'},
    {"verbose", 0, NULL, 'v'},
    {"quiet", 0, NULL, 'q'},
    {0, 0, 0, 0}		/* End marker: DO NOT REMOVE */
  };

/*
  atexit( &cleanup );
*/

  while( 1 ) {
    int  c;
    char t;
    int  i;

    c = getopt_long (argc, argv, "i:o:s:f:g:C:wumSPY"
#ifdef USE_FFMPEG
				"p:F:a:c:A:r:R:E:X"
#endif
				"hVvq", long_options, NULL);

    if( c == -1 )
      break;			/* End of option list */

    switch( c ) {
    case 0:
      break;		/* Used for long option returns */
    case 'i':
      inp_name = optarg;	/* strdup( optarg ); ?? */
      break;
    case 'o':
      out_name = optarg;	/* strdup( optarg ); ?? */
      break;
    case 's':
      snd_name = optarg;	/* strdup( optarg ); ?? */
      break;
    case 'w':
      snd_t = TYPE_WAV;	/* audio container WAV */
      break;
    case 'u':
      snd_t = TYPE_AU;		/* audio container AU */
      break;
    case 'm':
      snd_t = TYPE_AIFF;	/* audio container AIFF */
      break;
    case 0x102:
      force_aifc = 1;
      break;
    case 0x103:
      sound_only = 1;
      out_t = TYPE_NONE;
      break;
    case 'S':		/* SCR output */
      out_t = TYPE_SCR;
      break;
    case 'P':		/* PPM  output */
      out_t = TYPE_PPM;
      break;
    case 'Y':		/* YUV output */
      out_t = TYPE_YUV;
      break;
    case 0x101:		/* YUV FORMAT */
      if( !strcmp( optarg, "444" ) )
        yuv_t = TYPE_444;
      else if( !strcmp( optarg, "422" ) )
        yuv_t = TYPE_422;
      else if( !strcmp( optarg, "420j" ) ||
		!strcmp( optarg, "420jpeg" ) || !strcmp( optarg, "420mpeg1" ) )
        yuv_t = TYPE_420J;
      else if( !strcmp( optarg, "420m" ) || !strcmp( optarg, "420mpeg2" ) )
        yuv_t = TYPE_420M;
      else if( !strcmp( optarg, "420" ) )
        yuv_t = TYPE_420;
/*
      else if( !strcmp( optarg, "410" ) )
        yuv_t = TYPE_410;
*/
      else {
	printe( "Bad value for --yuv-format ...\n");
	return ERR_BAD_PARAM;
      }

	break;
    case 'f':				/* output frame rate */
      if( !strcmp( optarg, "pal" ) )
	out_fps = 25000;	/* 25.000 1/s */
      else if( !strcmp( optarg, "ntsc" ) )
	out_fps = 29970;	/* 29.970 1/s */
      else if( !strcmp( optarg, "movie" ) )
	out_fps = 24000;	/* 24.000 1/s */
      else if( sscanf( optarg, "%d.%1d%c", &out_fps, &i, &t ) == 2 )
	out_fps = out_fps * 1000 + i * 100;
      else if( sscanf( optarg, "%d.%2d%c", &out_fps, &i, &t ) == 2 )
	out_fps = out_fps * 1000 + i * 10;
      else if( sscanf( optarg, "%d.%3d%c", &out_fps, &i, &t ) == 2 )
	out_fps = out_fps * 1000 + i;
      else if( sscanf( optarg, "%d/%d%c", &out_fps, &i, &t ) == 2 )
	out_fps = out_fps * 1000 / i;
      else if( sscanf( optarg, "%d%c", &out_fps, &t ) == 1 )
	out_fps *= 1000;
      else {
	printe( "Unknow value for '-f/--frate' ...\n");
	return ERR_BAD_PARAM;
      }
      if( out_fps < 1 ) {
	printe( "Bad value for '-f/--frate' ...\n");
	return ERR_BAD_PARAM;
      }
      break;
    case 'C':
	out_cut = optarg;
	inp_get_next_cut();
	break;
#ifdef USE_FFMPEG
    case 'X':		/* FFMPEG output */
      out_t = TYPE_FFMPEG;
      snd_t = TYPE_FFMPEG;
      break;
    case 'p':		/* profile */
      if( !strcmp( optarg, "youtube" ) ) {
	ffmpeg_rescale = TYPE_RESCALE_WH;
	out_w = 480; out_h = 360; out_fps = 25000;
	ffmpeg_format = "mov"; ffmpeg_arate = 128000; ffmpeg_vrate = 600000;
      } else if( !strcmp( optarg, "dvd" ) ) {
	ffmpeg_rescale = TYPE_RESCALE_WH;
	out_w = 720; out_h = 576; out_fps = 25000;
	ffmpeg_format = "vob"; ffmpeg_arate = 192000; ffmpeg_vrate = 5000000;
	ffmpeg_aspect.num = 48; ffmpeg_aspect.den = 45;
	out_rte = 48000; sound_stereo = 1;
      } else if( !strcmp( optarg, "svcd" ) ) {
	ffmpeg_rescale = TYPE_RESCALE_WH;
	out_w = 480; out_h = 576; out_fps = 25000;
	ffmpeg_format = "svcd"; ffmpeg_arate = 192000; ffmpeg_vrate = 2600000;
	ffmpeg_aspect.num = 48; ffmpeg_aspect.den = 45;
	out_rte = 48000; sound_stereo = 1;
      } else if( !strcmp( optarg, "ipod" ) ) {
	ffmpeg_rescale = TYPE_RESCALE_WH;
	out_w = 320; out_h = 240; out_fps = 30000;
	ffmpeg_acodec = "aac"; ffmpeg_vcodec = "libx264";
	ffmpeg_format = "ipod"; ffmpeg_arate = 128000;ffmpeg_vrate = 256000;
	out_rte = 44100; sound_stereo = 1; ffmpeg_libx264 = 1;
      } else {
	printe( "Unknow value for '-p/--profile' ...\n");
	return ERR_BAD_PARAM;
      }
	break;
    case 'F':
      ffmpeg_format = optarg;
      break;
    case 'a':
      ffmpeg_acodec = optarg;
      break;
    case 'c':
      ffmpeg_vcodec = optarg;
      break;
    case 'A':
      if( !strcmp( optarg, "default" ) )
        ffmpeg_arate = 0;
      else
        ffmpeg_arate = atoi( optarg );
      break;
    case 'r':
      if( !strcmp( optarg, "default" ) )
        ffmpeg_vrate = 0;
      else if( !strcmp( optarg, "ffdefault" ) )
        ffmpeg_vrate = -1;
      else
        ffmpeg_vrate = atoi( optarg );
      break;
    case 'R':
      if( !strcmp( optarg, "vga" ) ) {
	ffmpeg_rescale = TYPE_RESCALE_WH;
	out_w = 640; out_h = 320;
      } else if( !strcmp( optarg, "hvga" ) ) {
	ffmpeg_rescale = TYPE_RESCALE_WH;
	out_w = 480; out_h = 360;
      } else if( !strcmp( optarg, "qvga" ) ) {
	ffmpeg_rescale = TYPE_RESCALE_WH;
	out_w = 320; out_h = 240;
      } else if( !strcmp( optarg, "pal" ) ) {
	ffmpeg_rescale = TYPE_RESCALE_WH;
	out_w = 768; out_h = 576; out_fps = 25000;
      } else if( sscanf( optarg, "%dx%d%c", &out_w, &out_h, &t ) == 2 ) {
	ffmpeg_rescale = TYPE_RESCALE_WH;
      } else if( sscanf( optarg, "%d/%d%c%c", &out_w, &out_h, &t, &t ) == 3 && t == 'x' ) {
	ffmpeg_rescale = TYPE_RESCALE_X;
      } else if( sscanf( optarg, "%d%c%c", &out_w, &t, &t ) == 2 && t == 'x' ) {
	ffmpeg_rescale = TYPE_RESCALE_X, out_h = 1;
      } else if( sscanf( optarg, "%d%c", &out_w, &t ) == 1 ) {
	ffmpeg_rescale = TYPE_RESCALE_WH;
	out_h = out_w * 3 / 4;
      } else {
	printe( "Unknow value for '-R/--resize' ...\n");
	return ERR_BAD_PARAM;
      }
      break;
    case 'E':
      if( !strcmp( optarg, "cd" ) )
        out_rte = 44100, sound_stereo = 1;
      else if( !strcmp( optarg, "dat" ) )
        out_rte = 48000, sound_stereo = 1;
      else
        out_rte = atoi( optarg );

      if( out_rte < 1000 ) {
	printe( "Wrong value for '-E/--srate' ...\n");
	return ERR_BAD_PARAM;
      }
      break;
#endif
    case 'h':
	print_help();
	help_exit = 1;
	break;
    case 'V':
	break;
    case 'v':
	if( verbose < VERBOSE_MAX )
	  verbose++;
	break;
    case 'q':
      if( verbose > VERBOSE_MIN )
	verbose--;
      break;
    case 'g':		/* progress 'bar' */
      if( !strcmp( optarg, "%" ) )
        prg_t = TYPE_PERC;
      else if( !strcmp( optarg, "bar" ) )
        prg_t = TYPE_BAR;
      else if( !strcmp( optarg, "frame" ) )
        prg_t = TYPE_FRAME;
      else if( !strcmp( optarg, "time" ) )
        prg_t = TYPE_TIME;
      break;
    case ':':
    case '?':
      break;

    default:
      printe ("%s: getopt_long returned `%c'\n",
		  "fmfconv", (char) c);
      break;

    }
  }

  if( optind < argc )
    inp_name = argv[optind++];
  if( !sound_only && optind < argc )		/* if sound only, we use second nonopt arg as sound filename*/
    out_name = argv[optind++];
  if( optind < argc )
    snd_name = argv[optind++];
  if( optind < argc ) {
    printe ("Too many filenames...\n");
    return ERR_BAD_PARAM;
  }

  if( !help_exit && !inp_name
#ifdef USE_FFMPEG
      && ffmpeg_list < 0
#endif
  ) {
    int fd = fileno( stdin );
    if( isatty( fd ) ) {
      printe( "%s: no input file specified\n", "fmfconv" );
      fprintf( stderr, "Try `fmfconv --help' for more information.\n" );
      help_exit = 1;
    }
#ifdef WIN32
    else {
      setmode( fd, O_BINARY );
    }
#endif				/* #ifdef WIN32 */
  }
  return 0;
}

int
main( int argc, char *argv[] )
{
  int err, eop = 0;

  if( ( err = parse_args( argc, argv ) ) ) return err;
  if( help_exit ) return 0;
#ifdef USE_FFMPEG
  if( ffmpeg_list >= 0 ) {
    ffmpeg_list_ffmpeg( ffmpeg_list );
    return 0;
  }
#endif
  if( do_info ) {
    snd_t = TYPE_NONE;
    out_t = TYPE_NONE;
    if( verbose < 1 ) verbose = 1;
  }
  if( sound_only ) {
    out_t = TYPE_NONE;
  }
  if( !sound_only && ( err = open_out() ) ) return err;
  if( ( err = open_snd() ) ) return err;

  if( snd_t == TYPE_UNSET && out_t == TYPE_FFMPEG ) {
    snd_t = TYPE_FFMPEG;
  }
  if( sound_raw ) {
    sound_pcm = 0;
    sound_stereo = -1;
  }
  if( sound_stereo == 1 ) out_chn = 2;
  else if( sound_stereo == 0 ) out_chn = 1;

#ifdef USE_ZLIB
  zstream.zalloc = Z_NULL;
  zstream.zfree = Z_NULL;
  zstream.opaque = Z_NULL;
  zstream.avail_in = 0;
  zstream.next_in = Z_NULL;
  inflateInit( &zstream );
  zstream.avail_out = ZBUF_SIZE;
#endif	/* USE_ZLIB */

  while( !eop ) {
    switch ( do_now ) {
    case DO_FILE:
      if( ( err = prepare_next_file() ) ) eop = 1;	/* if we read from multiple file... */
      if( do_now != DO_EOP && ( err = open_inp() ) ) eop = 1;		/* open (next) input file */
      break;
    case DO_HEAD:					/* read (next) file header */
      if( ( err = fmf_read_head() ) ) eop = 1;
#ifdef USE_FFMPEG
      if( out_t == TYPE_FFMPEG && !out_header_ok && ( err = out_write_ffmpegheader() ) ) return err;
#endif
      break;
    case DO_FRAME_HEAD:
      if( ( err = fmf_read_frame_head() ) ) eop = 1;	/* after finish a frame read next settings */
      break;
    case DO_SLICE:					/* read next fmf slice or 'N' or 'S' or 'X' */
      if( inp_t == TYPE_FMF ) {
        if( ( err = fmf_read_slice() ) ) eop = 1;
      } else {						/* read SCR file */
        if( ( err = scr_read_scr() ) ) eop = 1;
      }
      if( out_t != TYPE_NONE ) {		/* convert slice to RGB or YUV if needed */
        if( out_t >= TYPE_YUV )
          out_2_yuv();
        else if( out_t >= TYPE_PPM )
          out_2_rgb();
      }
      break;
    case DO_SOUND:
    case DO_SOUND_FLUSH:
      if( snd_t != TYPE_NONE && ( err = snd_write_sound() ) ) eop = 1;
      if( do_info ) snd_len = 0;			/* anyhow, we have to empty sound buffer */
      if( do_now == DO_SOUND_FLUSH ) fmf_read_sound();
      do_now = DO_SLICE;
      break;
    case DO_FRAME:
      if( out_t != TYPE_NONE ) {
        out_write_frame();
      }
    case DO_LAST_FRAME:
      if( inp_t == TYPE_FMF ) {
        if( do_now == DO_LAST_FRAME ) {
          fread_buff( fhead, 1, INTO_BUFF );	/* check concatenation */
          if( feof_compr( inp_file ) )
            do_now = DO_FILE;
          else
            do_now = DO_HEAD;
        } else {
          do_now = DO_FRAME_HEAD;
        }
      } else {
        do_now = DO_FILE;
      }
      break;
    case DO_EOP:
    default:
      eop = 1;
      break;
    }
    if( prg_t != TYPE_NONE && frame_no % 11 == 0 ) print_progress( 0 );
  }
  if( prg_t != TYPE_NONE ) print_progress( 1 ); /* update progress */
  if( ( out_t >= TYPE_SCR && out_t <= TYPE_JPEG ) && out_name ) unlink( out_name );
  close_snd();				/* close snd file */
  close_out();				/* close out file */
  if( prg_t != TYPE_NONE ) fprintf( stderr, "\n" );
  printi( 0, "main(): read %"PRIu64" frame (%"PRIu64" sec) and %"PRIu64" fmf slice and %"PRIu64" fmf sound chunk\n",
		frame_no, time_sec, fmf_slice_no, fmf_sound_no );
  if( !do_info )
    printi( 0, "main(): wrote %"PRIu64" frame dropped %"PRIu64" frame and inserted %"PRIu64" frame\n",
		output_no, drop_no, dupl_no );

#ifdef USE_ZLIB
  inflateEnd( &zstream );
#endif	/* USE_ZLIB */

  return 0;
}
