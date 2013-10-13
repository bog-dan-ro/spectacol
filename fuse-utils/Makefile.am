## Process this file with automake to produce Makefile.in
## Copyright (c) 1999-2002 Philip Kendall

## $Id$

## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License along
## with this program; if not, write to the Free Software Foundation, Inc.,
## 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
##
## Author contact information:
##
## E-mail: philip-fuse@shadowmagic.org.uk

AUTOMAKE_OPTIONS = foreign

ACLOCAL_AMFLAGS = -I m4

bin_PROGRAMS = createhdf \
	       fmfconv \
	       listbasic \
	       profile2map \
	       raw2hdf \
	       rzxdump \
	       rzxtool \
	       scl2trd \
	       snap2tzx \
	       snapconv \
	       tapeconv \
	       tzxlist

if BUILD_AUDIOTOOLS
bin_PROGRAMS += audio2tape tape2wav
endif

if BUILD_RZXCHECK
bin_PROGRAMS += rzxcheck
endif

EXTRA_PROGRAMS = rzxcheck audio2tape tape2wav

AM_CPPFLAGS = @LIBSPEC_CFLAGS@ @AUDIOFILE_CFLAGS@ @GLIB_CFLAGS@

createhdf_SOURCES = ide.c createhdf.c

fmfconv_SOURCES = fmfconv.c \
		  fmfconv_ff.c \
		  fmfconv_yuv.c \
		  fmfconv_scr.c \
		  fmfconv_ppm.c \
		  fmfconv_wav.c \
		  fmfconv_au.c \
		  fmfconv_aiff.c

if COMPAT_GETOPT
fmfconv_SOURCES += compat/getopt.c compat/getopt1.c
endif

fmfconv_LDADD = @FFMPEG_LIBS@ compat/libcompatos.a

listbasic_SOURCES = listbasic.c utils.c
listbasic_LDADD = @LIBSPEC_LIBS@ compat/libcompatos.a

profile2map_SOURCES = profile2map.c utils.c
profile2map_LDADD = @LIBSPEC_LIBS@ compat/libcompatos.a

raw2hdf_SOURCES = ide.c raw2hdf.c

rzxcheck_SOURCES = rzxcheck.c keys.c utils.c
rzxcheck_LDADD = @LIBSPEC_LIBS@ compat/libcompatos.a

rzxdump_SOURCES = rzxdump.c utils.c
rzxdump_LDADD = @LIBSPEC_LIBS@ compat/libcompatos.a

rzxtool_SOURCES = rzxtool.c utils.c
rzxtool_LDADD = @LIBSPEC_LIBS@ compat/libcompatos.a

scl2trd_SOURCES = scl2trd.c

snap2tzx_SOURCES = snap2tzx.c utils.c
snap2tzx_LDADD = @LIBSPEC_LIBS@ compat/libcompatos.a

snapconv_SOURCES = snapconv.c utils.c
snapconv_LDADD = @LIBSPEC_LIBS@ compat/libcompatos.a

tapeconv_SOURCES = tapeconv.c utils.c
tapeconv_LDADD = @LIBSPEC_LIBS@ compat/libcompatos.a

tzxlist_SOURCES = tzxlist.c utils.c
tzxlist_LDADD = @LIBSPEC_LIBS@ @LTLIBICONV@ compat/libcompatos.a

audio2tape_SOURCES = audio2tape.cc importer/soundfile.cc importer/trigger.cc \
                     converter/findpilot.cc converter/findsync1.cc \
                     converter/getpulse1.cc converter/getpulse2.cc \
                     converter/getsync2.cc converter/romloader.cc \
                     converter/romloaderstate.cc utils.c
audio2tape_LDADD = @LIBSPEC_LIBS@ @AUDIOFILE_LIBS@ \
                   compat/libcompatos.a

tape2wav_SOURCES = tape2wav.c utils.c
tape2wav_LDADD = @LIBSPEC_LIBS@ @AUDIOFILE_LIBS@ \
                 compat/libcompatos.a

noinst_HEADERS = compat.h ide.h utils.h audio2tape.h importer/interpolator.h \
                 importer/schmitt.h importer/simple.h importer/soundfile.h \
                 importer/trigger.h converter/findpilot.h \
                 converter/findsync1.h converter/getpulse1.h \
                 converter/getpulse2.h converter/getsync2.h \
                 converter/romloader.h converter/romloaderstate.h \
                 compat/getopt.h fmfconv.h movie_tables.h

noinst_LIBRARIES =

EXTRA_DIST =

include compat/Makefile.am
include hacking/Makefile.am
include m4/Makefile.am
include man/Makefile.am
