/* fmfconv_ff.c: Routines for converting movie with ffmpeg libs
   Copyright (c) 2010-2013 Gergely Szasz

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

   szaszg@hu.inter.net

*/

/* Most of the ffmpeg part of the code deduced from FFMPEG's output-example.c.
   Here is the original copyright note:
*/

/*
 * Libavformat API example: Output a media file in any supported
 * libavformat format. The default codecs are used.
 *
 * Copyright (c) 2003 Fabrice Bellard
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <config.h>

#ifdef USE_FFMPEG
#include <stdio.h>
#include <stdlib.h>

#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

#include "libspectrum.h"
#include "fmfconv.h"

#define VRATE_MULT 2
static int sws_flags = SWS_BICUBIC | SWS_CPU_CAPS_3DNOW | SWS_CPU_CAPS_MMX2;

int ffmpeg_arate = 0;		/* audio bitrate */
int ffmpeg_vrate = 0;		/* video bitrate */
AVRational ffmpeg_aspect = { 1, 1 };
int ffmpeg_libx264 = 0;
const char *ffmpeg_frate = NULL;
const char *ffmpeg_format = NULL;
const char *ffmpeg_vcodec = NULL;
const char *ffmpeg_acodec = NULL;
type_t ffmpeg_rescale = TYPE_NONE;
int ffmpeg_list = -1;

static AVOutputFormat *fmt;
static AVFormatContext *oc;
static AVStream *audio_st, *video_st;

static uint8_t *audio_outbuf;
static uint8_t *audio_tmp_inpbuf;
static int16_t *audio_inpbuf;
static int audio_outbuf_size;
static int audio_tmp_inpbuf_size;		/* if resampling */
static int audio_iframe_size;			/* size of an input audio frame in bytes */
static int audio_oframe_size;			/* size of an output audio frame in bytes */
static int audio_input_frames;			/* number of required audio frames for a packet */
static int audio_inpbuf_len;			/* actual number of audio frames */

static AVFrame *ff_picture, *ff_tmp_picture, *ffmpeg_pict;
static struct SwsContext *video_resize_ctx;

static ReSampleContext *audio_resample_ctx;
static int16_t **ffmpeg_sound;

static uint8_t *video_outbuf;
static int video_outbuf_size;

static AVFrame *alloc_picture( enum PixelFormat pix_fmt, int width, int height, void *fmf_pict );

static enum PixelFormat out_pix_fmt = PIX_FMT_NONE;
/* FFMPEG utility functions */

static int res_rte = -1;

static void setup_x264_dict( AVDictionary **  pm );

int
ffmpeg_resample_audio( void )
{
  int len;

  if( audio_resample_ctx && res_rte != snd_rte ) {
    audio_resample_close( audio_resample_ctx );
    audio_resample_ctx = NULL;
    av_free( audio_tmp_inpbuf );
    ffmpeg_sound = &sound16;
    printi( 2, "ffmpeg_resample_audio(): reinit resample %dHz -> %dHz\n", snd_rte, out_rte );
    res_rte = -1;
  }
  if( res_rte == -1 ) {
    res_rte = snd_rte;
    out_fsz = 2 * out_chn;
  }
  if( !audio_resample_ctx ) {
    audio_resample_ctx = av_audio_resample_init( out_chn, snd_chn,
						 out_rte, snd_rte,
						 AV_SAMPLE_FMT_S16,
                                                 AV_SAMPLE_FMT_S16,
						 16, 8, 1, 1.0 );
    audio_tmp_inpbuf_size = (float)audio_outbuf_size * out_rte / snd_rte * (float)out_chn / snd_chn + 1.0;
    audio_tmp_inpbuf = av_malloc( audio_tmp_inpbuf_size );
    ffmpeg_sound = (void *)(&audio_tmp_inpbuf);
    printi( 2, "ffmpeg_resample_audio(): alloc resample context (%dHz -> %dHz) %d byte len\n", snd_rte, out_rte, audio_tmp_inpbuf_size );
  }
  if( !audio_resample_ctx ) {
    printe( "FFMPEG: Could not allocate audio resample context\n" );
    return 1;
  }

  len = audio_resample( audio_resample_ctx,
                  (short *)audio_tmp_inpbuf, (short *)sound16,
                  snd_len / snd_fsz );
  if( !len ) {
    printe( "FFMPEG: Error during audio resampling\n" );
    return 1;
  }
  out_len = len * out_fsz;
  printi( 3, "ffmpeg_resample_audio(): resample %d samples to %d samples\n", snd_len / snd_fsz, out_len / out_fsz );
  return 0;

}

static int res_w = -1, res_h;

int
ffmpeg_rescale_video( void )
{
  if( video_resize_ctx && ( frm_w != res_w || frm_h != res_h ) ) {
    sws_freeContext( video_resize_ctx );
    video_resize_ctx = NULL;
    res_w = -1;
    ffmpeg_pict = ff_picture;
  }

  if( res_w == -1 ) {
    ff_tmp_picture = alloc_picture( out_pix_fmt, out_w, out_h, NULL );
    if( !ff_tmp_picture ) {
      printe( "FFMPEG: Could not allocate temporary picture\n" );
      avcodec_close( video_st->codec );
      av_free( ff_picture );
      return 1;
    }
    res_w = frm_w;
    res_h = frm_h;
  }
  if( video_resize_ctx == NULL ) {
    video_resize_ctx = sws_getContext( res_w, res_h,
                                      PIX_FMT_YUV444P,
                                      out_w, out_h,
                                      out_pix_fmt,
                                      sws_flags, NULL, NULL, NULL );
    if( !video_resize_ctx ) {
      printe( "Cannot initialize the conversion context\n" );
      avcodec_close( video_st->codec );
      av_free( ff_picture );
      return 1;
    }
    ffmpeg_pict = ff_tmp_picture;
  }
  sws_scale( video_resize_ctx, 
		(const uint8_t * const*)ff_picture->data, ff_picture->linesize,
		0, res_h, ff_tmp_picture->data, ff_tmp_picture->linesize );
  printi( 3, "ffmpeg_rescale_frame(): resize %dx%d -> %dx%d pix format %d->%d\n", frm_w, frm_h, out_w, out_h, PIX_FMT_YUV444P, out_pix_fmt );
  return 0;
}

/**************************************************************/
/* audio output */
/*
 * add an audio output stream
 */

static int
add_audio_stream( enum CodecID codec_id, int freq, int stereo )
{
  AVCodecContext *c;

  audio_st = av_new_stream( oc, 1 );
  if( !audio_st ) {
    printe( "FFMPEG: Could not allocate audio stream stream\n" );
    return 1;
  }

  c = audio_st->codec;
  c->codec_id = codec_id;
  c->codec_type = AVMEDIA_TYPE_AUDIO;

    /* put sample parameters */
  c->sample_fmt = AV_SAMPLE_FMT_S16;
  if( ffmpeg_arate > 0 )
    c->bit_rate = ffmpeg_arate;
  audio_resample_ctx = NULL;

  c->sample_rate = out_rte;
  c->channels = out_chn;

    /* some formats want stream headers to be separate */
  if( oc->oformat->flags & AVFMT_GLOBALHEADER )
    c->flags |= CODEC_FLAG_GLOBAL_HEADER;

  return 0;
}

static int
open_audio( void )
{
  AVCodecContext *c;
  AVCodec *codec;

  c = audio_st->codec;

    /* find the audio encoder */
  codec = avcodec_find_encoder( c->codec_id );
  if( !codec ) {
    printe( "FFMPEG: audio codec not found\n" );
    return 1;
  }

    /* open it */
  if( avcodec_open2( c, codec, NULL ) < 0 ) {
    printe( "FFMPEG: could not open audio codec\n" );
    return 1;
  }

  if( codec->sample_fmts != NULL )
    c->sample_fmt = AV_SAMPLE_FMT_S16;

#ifdef HAVE_FFMPEG_BYTES_SAMPLE
  audio_oframe_size = ( av_get_bytes_per_sample( c->sample_fmt ) + 7 ) / 8 * out_chn;
#elif defined(HAVE_FFMPEG_SAMPLE_FMT)
  audio_oframe_size = ( av_get_bits_per_sample_fmt( c->sample_fmt ) + 7 ) / 8 * out_chn;
#else
  audio_oframe_size = ( av_get_bits_per_sample_format( c->sample_fmt ) + 7 ) / 8 * out_chn;
#endif
  audio_input_frames = c->frame_size;

  if( audio_input_frames <= 1 ) {
    audio_outbuf_size = out_rte * 1250 * audio_oframe_size / out_fps;
  } else {
    audio_outbuf_size = audio_input_frames * audio_oframe_size * 125 / 100;
    if( audio_outbuf_size < FF_MIN_BUFFER_SIZE ) audio_outbuf_size = FF_MIN_BUFFER_SIZE;
    audio_inpbuf = av_malloc( audio_input_frames * 4 );  /* alloc buffer as stereo PCM */
    audio_inpbuf_len = 0;
  }
  audio_outbuf = av_malloc( audio_outbuf_size );
  ffmpeg_sound = &sound16;

  return 0;
}

/* add an audio frame to the stream */
void
ffmpeg_add_sound_ffmpeg( int len )
{
  AVCodecContext *c;
  int coded_bps;
  int16_t *buf;

  if( !audio_st ) return;

  buf = *ffmpeg_sound;
  c = audio_st->codec;
  coded_bps = av_get_bits_per_sample( c->codec_id );
  audio_iframe_size = snd_chn > 1 ? 4 : 2;

  if( audio_input_frames > 1 && audio_inpbuf_len + len < audio_input_frames ) {
  /* store the full sound buffer and wait for more...*/
    memcpy( (char *)audio_inpbuf + ( audio_inpbuf_len * audio_iframe_size ), buf, len * audio_iframe_size );
    audio_inpbuf_len += len;
    printi( 3, "ffmpeg_add_sound_ffmpeg(): store %d samples (%d/%d)\n", len, audio_input_frames, audio_iframe_size );
    return;
  }

  printi( 3, "ffmpeg_add_sound_ffmpeg(): %d -> %d\n", audio_iframe_size, audio_oframe_size );
  if( audio_input_frames > 1 ) {
     while( audio_inpbuf_len + len >= audio_input_frames ) {
      int copy_len = ( audio_input_frames - audio_inpbuf_len ) * audio_iframe_size;
      AVPacket pkt;
      av_init_packet( &pkt );

      memcpy( (char *)audio_inpbuf + ( audio_inpbuf_len * audio_iframe_size ), buf, copy_len );
      len -= audio_input_frames - audio_inpbuf_len;
      buf = (void *)( (char *)buf + copy_len );

      pkt.size = avcodec_encode_audio( c, audio_outbuf, audio_outbuf_size, audio_inpbuf );

      if( c->coded_frame && c->coded_frame->pts != AV_NOPTS_VALUE )
	pkt.pts= av_rescale_q( c->coded_frame->pts, c->time_base, audio_st->time_base );
      pkt.flags |= AV_PKT_FLAG_KEY;
      pkt.stream_index = audio_st->index;
      pkt.data = audio_outbuf;

    /* write the compressed frame in the media file */
      if( av_interleaved_write_frame( oc, &pkt ) != 0 ) {
        fprintf( stderr, "Error while writing audio frame\n" );
    /*    exit( 1 ); */
      }
      audio_inpbuf_len = 0;
      printi( 3, "ffmpeg_add_sound_ffmpeg(): write sound packet %d samples (remain: %d) pkt.size = %dbyte\n", audio_input_frames, len, pkt.size );
    }
    /* store the remaind bytes */
    memcpy( audio_inpbuf, buf, len * audio_iframe_size );
    audio_inpbuf_len = len;


  } else {			/* with PCM output */
    AVPacket pkt;
    av_init_packet( &pkt );

    pkt.size = avcodec_encode_audio( c, audio_outbuf, coded_bps ? len * out_chn * coded_bps / 8 : len * out_chn, buf );

    if( c->coded_frame && c->coded_frame->pts != AV_NOPTS_VALUE )
      pkt.pts= av_rescale_q( c->coded_frame->pts, c->time_base, audio_st->time_base );
    pkt.flags |= AV_PKT_FLAG_KEY;
    pkt.stream_index = audio_st->index;
    pkt.data = audio_outbuf;

    /* write */
    if( av_interleaved_write_frame( oc, &pkt ) != 0 ) {
      fprintf( stderr, "Error while writing audio frame\n" );
/*      exit( 1 ); */
    }
    printi( 3, "ffmpeg_add_sound_ffmpeg(): write sound packet %d samples pkt.size = %dbyte\n", len, pkt.size );
  }
}

int
snd_write_ffmpeg( void )
{

/*  if( !snd_header_ok && ( err = snd_write_ffmpegheader() ) ) return err; */
  if( !audio_st ) return 0;

  if( snd_enc == TYPE_PCM && 
#ifndef WORDS_BIGENDIAN
	!snd_little_endian
#else
	snd_little_endian
#endif
  ) {	/* we have to swap all sample */
    pcm_swap_endian();
  }
  if( snd_len > 0 && ( out_rte != snd_rte || out_chn != snd_chn ) ) {
    printi( 3, "snd_write_ffmpeg(): got %d samples sound\n", snd_len / snd_fsz );
    ffmpeg_resample_audio();
    ffmpeg_add_sound_ffmpeg( out_len / out_fsz );
    printi( 2, "snd_write_ffmpeg(): %d samples sound\n", out_len / out_fsz );
  } else if( snd_len > 0 ) {
    ffmpeg_add_sound_ffmpeg( snd_len / snd_fsz );
    printi( 2, "snd_write_ffmpeg(): %d samples sound\n", snd_len / snd_fsz );
  }

  return 0;
}

static void
close_audio( void )
{
  if( audio_st ) avcodec_close( audio_st->codec );
  if( audio_outbuf ) av_free( audio_outbuf );
}

/**************************************************************/
/*   pix_yuv --> [ tmp_picture ] --> [encode] */
/* video output */

static int
check_framerate( const AVRational *frates, int timing )
{
  if( frates == NULL ) return -1;
  while( frates->den != 0 || frates->num != 0 ) {
    if( ( frates->num * 1000 / frates->den ) == timing ) {
      return 1;
    }
    frates++;
  }
  return 0;
}

/* add a video output stream */
static int
add_video_stream( enum CodecID codec_id, int w, int h, int timing )
{
  AVCodecContext *c;

  video_st = av_new_stream( oc, 0 );
  if( !video_st ) {
    printe( "FFMPEG: Could not allocate video stream\n" );
    return 1;
  }

  c = video_st->codec;
  c->codec_id = codec_id;
  c->codec_type = AVMEDIA_TYPE_VIDEO;

    /* put sample parameters */
  if( ffmpeg_vrate > 0 )
    c->bit_rate = ffmpeg_vrate;
  else if( ffmpeg_vrate == 0 )
    c->bit_rate = c->bit_rate * VRATE_MULT;

    /* resolution must be a multiple of two */
  c->width  = w;
  c->height = h;
    /* time base: this is the fundamental unit of time (in seconds) in terms
       of which frame timestamps are represented. for fixed-fps content,
       timebase should be 1/framerate and timestamp increments should be
       identically 1. */
  c->gop_size = 25; /* emit one intra frame every twelve frames at most */
  video_st->sample_aspect_ratio = c->sample_aspect_ratio = ffmpeg_aspect;
  if( ffmpeg_libx264 ) {
    c->profile = FF_PROFILE_H264_HIGH;
    c->me_range = 16;
    c->max_qdiff = 4;
    c->qmin = 10;
    c->qmax = 51;
    c->qcompress = 0.0;
    c->me_method = 8;
    c->trellis = 1;
/*
    c->partitions = X264_PART_I4X4 | X264_PART_I8X8 | X264_PART_P8X8 | X264_PART_P4X4 | X264_PART_B8X8;
*/
    c->flags2 |= 0;
    
    setup_x264_dict( c->priv_data );

  }
    /* some formats want stream headers to be separate */
  if( oc->oformat->flags & AVFMT_GLOBALHEADER )
    c->flags |= CODEC_FLAG_GLOBAL_HEADER;
  return 0;
}

static AVFrame *
alloc_picture( enum PixelFormat pix_fmt, int width, int height, void *fmf_pict )
{
  AVFrame *picture;
  uint8_t *picture_buf;
  int size;

  picture = avcodec_alloc_frame();
  if( !picture ) return NULL;
  if( !fmf_pict ) {
    size = avpicture_get_size( pix_fmt, width, height );
    picture_buf = av_malloc( size );
    if( !picture_buf ) {
      av_free( picture );
      return NULL;
    }
    avpicture_fill( (AVPicture *)picture, picture_buf,
                    pix_fmt, width, height );
  } else {
    avpicture_fill( (AVPicture *)picture, fmf_pict,
                    pix_fmt, width, height );
  }
  return picture;
}

static int
open_video( void )
{
  AVCodec *codec;
  AVCodecContext *c;

  c = video_st->codec;

    /* find the video encoder */
  codec = avcodec_find_encoder( c->codec_id );
  if( !codec ) {
    printe( "FFMPEG: video codec not found\n" );
    return 1;
  }

  if( codec->pix_fmts == NULL )
     c->pix_fmt = PIX_FMT_YUV420P;
  else
    c->pix_fmt = codec->pix_fmts[0];

  out_pix_fmt = c->pix_fmt;
  if( out_fps == 0 ) {		/* we use 24 1/s default framerate */
    out_fps = 24000;
    c->time_base.den = 24000;
    c->time_base.num = 1000;
  } else {
    if( codec->supported_framerates == NULL ||
        check_framerate( codec->supported_framerates, out_fps ) ) {	/* codec support the selected frate */
      c->time_base.den = out_fps;
      c->time_base.num = 1000;
    } else {
      printe( "FFMPEG: The selected video codec cannot use the required frame rate!" );
      return 1;
    }
  }

  if( ffmpeg_libx264 ) {
    setup_x264_dict( c->priv_data );
  }

    /* open the codec */
  if( avcodec_open2( c, codec, NULL ) < 0 ) {
    printe( "FFMPEG: could not open video codec\n" );
    return 1;
  }

  if( !( oc->oformat->flags & AVFMT_RAWPICTURE ) ) {
        /* allocate output buffer */
        /* XXX: API change will be done */
        /* buffers passed into lav* can be allocated any way you prefer,
           as long as they're aligned enough for the architecture, and
           they're freed appropriately (such as using av_free for buffers
           allocated with av_malloc) */
    video_outbuf_size = 200000;
    video_outbuf = av_malloc( video_outbuf_size );
  }

  /* allocate the encoded raw picture */
  ff_picture = alloc_picture( PIX_FMT_YUV444P, frm_w, frm_h, pix_yuv[0] );
  ff_tmp_picture = NULL;

  if( !ff_picture ) {
    printe( "FFMPEG: Could not allocate picture\n" );
    avcodec_close(video_st->codec);
    return 1;
  }
  ffmpeg_pict = ff_picture;

  return 0;
}

void
ffmpeg_add_frame_ffmpeg( void )
{
  int out_size, ret;
  AVCodecContext *c;

  if( !video_st ) return;

  c = video_st->codec;

  if( oc->oformat->flags & AVFMT_RAWPICTURE ) {
        /* raw video case. The API will change slightly in the near
           future for that */
    AVPacket pkt;
    av_init_packet( &pkt );

    pkt.flags |= AV_PKT_FLAG_KEY;
    pkt.stream_index= video_st->index;
    pkt.data= (uint8_t *)ffmpeg_pict;
    pkt.size= sizeof( AVPicture );

    ret = av_interleaved_write_frame( oc, &pkt );
  } else {
        /* encode the image */
    out_size = avcodec_encode_video( c, video_outbuf, video_outbuf_size, ffmpeg_pict );
        /* if zero size, it means the image was buffered */
    if( out_size > 0 ) {
      AVPacket pkt;
      av_init_packet( &pkt );

      if( c->coded_frame->pts != AV_NOPTS_VALUE )
        pkt.pts = av_rescale_q( c->coded_frame->pts, c->time_base, video_st->time_base );
      if( c->coded_frame->key_frame )
        pkt.flags |= AV_PKT_FLAG_KEY;
      pkt.stream_index= video_st->index;
      pkt.data = video_outbuf;
      pkt.size = out_size;

            /* write the compressed frame in the media file */
      ret = av_interleaved_write_frame( oc, &pkt );
    } else {
      ret = 0;
    }
  }
  if( ret != 0 ) {
    fprintf(stderr, "Error while writing video frame\n");
/*      exit( 1 ); */
  }
}

static void
close_video( void )
{
  if( video_st )
    avcodec_close( video_st->codec );
  if( ff_picture ) {
    /* av_free( ff_picture->data[0] ); */
    av_free( ff_picture );
  }
  if( ff_tmp_picture ) {
    av_free( ff_tmp_picture->data[0] );
    av_free( ff_tmp_picture );
  }
  if( video_outbuf ) av_free( video_outbuf );
}

/**************************************************************/
/* media file output */

/* init lavc and open output file... */

int
out_write_ffmpegheader( void )
{

  AVCodec *c;
  enum CodecID acodec, vcodec;

  ff_picture = NULL;
  ff_tmp_picture = NULL;
  video_outbuf = NULL;
  audio_outbuf = NULL;

    /* initialize libavcodec, and register all codecs and formats */
  av_register_all();

    /* auto detect the output format from the name. default is
       mpeg. */
  if( ffmpeg_format != NULL && *ffmpeg_format != 0 ) {
    fmt = av_guess_format( NULL, NULL, ffmpeg_format );		/* MIME */
    if( !fmt )
      fmt = av_guess_format( ffmpeg_format, NULL, NULL );	/* Name */
    if( !fmt ) {
      printe( "FFMPEG: Unknown output format '%s'.\n", ffmpeg_format );
      return 1;
    }
  }

  if( !fmt )
    fmt = av_guess_format( NULL, out_name, NULL );
  if( !fmt ) {
/*    printf( "Could not deduce output format from file extension: using AVI.\n" ); */
    fmt = av_guess_format( "avi", NULL, NULL );
  }
  if( !fmt ) {
    printe( "FFMPEG: Could not find suitable output format\n" );
    return 1;
  }

    /* allocate the output media context */
  oc = avformat_alloc_context();
  if( !oc ) {
    printe( "FFMPEG: Memory error\n" );
    return 1;
  }
  oc->oformat = fmt;
  snprintf( oc->filename, sizeof( oc->filename ), "%s", out_name );

    /* add the audio and video streams using the default format codecs
       and initialize the codecs */
  video_st = NULL;
  audio_st = NULL;
  vcodec = fmt->video_codec;
  acodec = fmt->audio_codec;
  if( out_t == TYPE_FFMPEG && vcodec != CODEC_ID_NONE ) {
    AVCodec *c;
    if( ffmpeg_vcodec != NULL && *ffmpeg_vcodec != 0 ) {
      c = avcodec_find_encoder_by_name( ffmpeg_vcodec );
      if( c && c->type == AVMEDIA_TYPE_VIDEO ) {
        vcodec = c->id;
      } else {
        printe( "FFMPEG: Unknown video encoder '%s'.\n", ffmpeg_vcodec );
        return 1;
      }
    }
    if( ffmpeg_rescale == TYPE_NONE ) {
      out_w = frm_w; out_h = frm_h;
    } else if( ffmpeg_rescale == TYPE_RESCALE_X ) {
      int tmp = frm_h * out_w / out_h;
      out_w = frm_w * out_w / out_h;
      out_h = tmp;
    }

    if( add_video_stream( vcodec, out_w, out_h, machine_timing[frm_mch - 'A'] ) )
      return 1;
  }
  if( snd_t == TYPE_FFMPEG && acodec != CODEC_ID_NONE ) {
    if( ffmpeg_acodec != NULL && *ffmpeg_acodec != 0 ) {
      c = avcodec_find_encoder_by_name( ffmpeg_acodec );
      if( c && c->type == AVMEDIA_TYPE_AUDIO ) {
        acodec = c->id;
      } else {
        printe( "FFMPEG: Unknown audio encoder '%s'.\n", ffmpeg_acodec );
      }
    }
    if( add_audio_stream( acodec, snd_rte, snd_chn > 1 ? 1 : 0 ) ) {
      close_video();
      return 1;
    }
  }

    /* set the output parameters (must be done even if no
       parameters). */
#ifndef HAVE_FFMPEG_AVFORMAT_WRITE_HEADER
  if( av_set_parameters( oc, NULL ) < 0 ) {
    printe( "FFMPEG: Invalid output format parameters\n" );
    close_video();
    close_audio();
    return 1;
  }
#endif
/* dump_format( oc, 0, out_name, 1 ); */

    /* now that all the parameters are set, we can open the audio and
       video codecs and allocate the necessary encode buffers */
  if( video_st && open_video() ) {
    close_video();
    close_audio();
    return 1;
  }
  if( audio_st && open_audio() ) {
    close_video();
    close_audio();
    return 1;
  }

    /* open the output file, if needed */
  if( !( fmt->flags & AVFMT_NOFILE ) ) {
#ifdef HAVE_FFMPEG_AVIO_OPEN
    if( avio_open( &oc->pb, out_name, AVIO_FLAG_WRITE ) < 0 ) {
      printe( "FFMPEG: Could not open '%s'\n", out_name );
      return 1;
    }
#else
    if( url_fopen( &oc->pb, out_name, URL_WRONLY ) < 0 ) {
      printe( "FFMPEG: Could not open '%s'\n", out_name );
      return 1;
    }
#endif
  }
    /* write the stream header, if any */
#ifdef HAVE_FFMPEG_AVFORMAT_WRITE_HEADER
  avformat_write_header( oc, NULL );
#else
  av_write_header( oc );
#endif

  out_header_ok = 1;

  return 0;
}

int
out_write_ffmpeg( void )
{
  int err;

  if( !video_st ) return 0;

  if( ( out_w != frm_w || out_h != frm_h || out_pix_fmt != PIX_FMT_YUV444P ) && 
	( err = ffmpeg_rescale_video() ) ) return err;
  ffmpeg_add_frame_ffmpeg();
  printi( 2, "out_write_ffmpeg(): add frame\n" );
  return 0;
}


void
out_finalize_ffmpeg( void )
{
  unsigned int i;
    /* write the trailer, if any.  the trailer must be written
     * before you close the CodecContexts open when you wrote the
     * header; otherwise write_trailer may try to use memory that
     * was freed on av_codec_close() */
  av_write_trailer( oc );

    /* close each codec */
  if( video_st )
    close_video();
  if( audio_st )
    close_audio();

    /* free the streams */
  for( i = 0; i < oc->nb_streams; i++ ) {
    av_freep( &oc->streams[i]->codec );
    av_freep( &oc->streams[i] );
  }

  if( !( fmt->flags & AVFMT_NOFILE ) ) {
        /* close the output file */
#ifdef HAVE_FFMPEG_AVIO_OPEN
    avio_close( oc->pb );
#else
    url_fclose( oc->pb );
#endif
  }

    /* free the stream */
  av_free( oc );
  printi( 2, "out_finalize_ffmpeg(): FIN\n" );
}

/* util functions */
static void
show_formats( void )
{
  AVInputFormat *ifmt = NULL;
  AVOutputFormat *ofmt = NULL;
  const char *last_name, *mime_type = NULL;

  printf( "List of available FFMPEG file formats:\n"
          "  name       mime type        long name\n"
          "-------------------------------------------------------------------------------\n" );
  last_name= "000";
  for(;;){
    int encode=0;
    const char *name=NULL;
    const char *long_name=NULL;

    while( ( ofmt = av_oformat_next( ofmt ) ) ) {
      if( ( name == NULL || strcmp( ofmt->name, name ) < 0 ) &&
            strcmp( ofmt->name, last_name ) > 0 ) {
        name = ofmt->name;
        long_name = ofmt->long_name;
        mime_type = ofmt->mime_type;
        encode = 1;
      }
    }

    while( ( ifmt = av_iformat_next( ifmt ) ) ) {
      if( ( name == NULL || strcmp( ifmt->name, name ) < 0 ) &&
            strcmp( ifmt->name, last_name ) > 0 ) {
        name = ifmt->name;
        long_name = ifmt->long_name;
        encode = 0;
      }
    }

    if( name == NULL ) break;
    last_name = name;

    if( encode )
      printf(
            "  %-10s %-16s %s\n", name,
            mime_type ? mime_type:"---",
            long_name ? long_name:"---" );
    }
    printf( "-------------------------------------------------------------------------------\n" );
}

void
show_codecs( int what )
{
  AVCodec *p = NULL, *p2;
  const char *last_name;

  printf( "List of available FFMPEG %s codecs:\n"
          "  name             long name\n"
          "-------------------------------------------------------------------------------\n", what ? "video" : "audio" );
  last_name= "000";
  for(;;){
    int encode = 0;
    int ok;

    p2 = NULL;
    while( ( p = av_codec_next( p ) ) ) {
      if( ( p2 == NULL || strcmp( p->name, p2->name ) < 0 ) &&
            strcmp( p->name, last_name ) > 0 ) {
        p2 = p;
        encode = 0;
      }
      if( p2 && strcmp( p->name, p2->name ) == 0 ) {
        if( p->encode2 ) encode=1;
      }
    }
    if( p2 == NULL ) break;
    last_name = p2->name;

    ok = 0;
    switch( p2->type ) {
    case AVMEDIA_TYPE_VIDEO:
      if( encode && what ) ok = 1;
      break;
    case AVMEDIA_TYPE_AUDIO:
      if( encode && !what ) ok = 1;
      break;
    default:
      break;
    }
    if( ok )
      printf( "  %-16s %s\n", p2->name,
/*              p2->mime_type ? p2->mime_type : "---", */
              p2->long_name ? p2->long_name : "---" );
    }
    printf( "-------------------------------------------------------------------------------\n" );
}

void
ffmpeg_print_idents( void )
{
  if( verbose > 0 ) {
    printf( "FFmpeg libraries:\n" );
    printf( "  libavcodec:  %s\n  libavformat: %s\n  libavutil:   %s\n  libswscale:  %s\n",
#ifdef LIBAVCODEC_IDENT
    LIBAVCODEC_IDENT,
#else
    "<unknown>",
#endif
#ifdef LIBAVFORMAT_IDENT
    LIBAVFORMAT_IDENT,
#else
    "<unknown>",
#endif
#ifdef LIBAVUTIL_IDENT
    LIBAVUTIL_IDENT,
#else
    "<unknown>",
#endif
#ifdef LIBSWSCALE_IDENT
    LIBSWSCALE_IDENT
#else
    "<unknown>"
#endif
    );
  }
}

void
ffmpeg_list_ffmpeg( int what )
{
  av_register_all();
  if( what == 0 ) show_formats();
  if( what == 1 ) show_codecs( 0 );
  if( what == 2 ) show_codecs( 1 );
  ffmpeg_print_idents();
  putc('\n', stdout);
}

static void
setup_x264_dict( AVDictionary ** dict )
{
  /* initialise private options if we will be using libx264 codecs */
  av_dict_set( dict, "mixed-refs", "1", 0 );
  av_dict_set( dict, "weightb", "1", 0 );
  av_dict_set( dict, "8x8dct", "1", 0 );
  av_dict_set( dict, "fast-pskip", "1", 0 );
  av_dict_set( dict, "qp", "-1", 0 );
  av_dict_set( dict, "crf", "0", 0 );
  av_dict_set( dict, "preset", "medium", 0 );
}

#endif	/* ifdef USE_FFMPEG */
