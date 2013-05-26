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

SUBDIRS = @COMPAT_SUBDIR@ \
          hacking \
          man

DIST_SUBDIRS = compat/amiga \
               compat/unix \
               compat/win32 \
               hacking \
               man

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

fmfconv_LDADD = @FFMPEG_LIBS@ compat/@COMPAT_OSNAME@/libcompatos.a

listbasic_SOURCES = listbasic.c utils.c
listbasic_LDADD = @LIBSPEC_LIBS@ compat/@COMPAT_OSNAME@/libcompatos.a

profile2map_SOURCES = profile2map.c utils.c
profile2map_LDADD = @LIBSPEC_LIBS@ compat/@COMPAT_OSNAME@/libcompatos.a

raw2hdf_SOURCES = ide.c raw2hdf.c

rzxcheck_SOURCES = rzxcheck.c keys.c utils.c
rzxcheck_LDADD = @LIBSPEC_LIBS@ compat/@COMPAT_OSNAME@/libcompatos.a

rzxdump_SOURCES = rzxdump.c utils.c
rzxdump_LDADD = @LIBSPEC_LIBS@ compat/@COMPAT_OSNAME@/libcompatos.a

rzxtool_SOURCES = rzxtool.c utils.c
rzxtool_LDADD = @LIBSPEC_LIBS@ compat/@COMPAT_OSNAME@/libcompatos.a

scl2trd_SOURCES = scl2trd.c

snap2tzx_SOURCES = snap2tzx.c utils.c
snap2tzx_LDADD = @LIBSPEC_LIBS@ compat/@COMPAT_OSNAME@/libcompatos.a

snapconv_SOURCES = snapconv.c utils.c
snapconv_LDADD = @LIBSPEC_LIBS@ compat/@COMPAT_OSNAME@/libcompatos.a

tapeconv_SOURCES = tapeconv.c utils.c
tapeconv_LDADD = @LIBSPEC_LIBS@ compat/@COMPAT_OSNAME@/libcompatos.a

tzxlist_SOURCES = tzxlist.c utils.c
tzxlist_LDADD = @LIBSPEC_LIBS@ @LTLIBICONV@ compat/@COMPAT_OSNAME@/libcompatos.a

audio2tape_SOURCES = audio2tape.cc importer/soundfile.cc importer/trigger.cc \
                     converter/findpilot.cc converter/findsync1.cc \
                     converter/getpulse1.cc converter/getpulse2.cc \
                     converter/getsync2.cc converter/romloader.cc \
                     converter/romloaderstate.cc utils.c
audio2tape_LDADD = @LIBSPEC_LIBS@ @AUDIOFILE_LIBS@ \
                   compat/@COMPAT_OSNAME@/libcompatos.a

tape2wav_SOURCES = tape2wav.c utils.c
tape2wav_LDADD = @LIBSPEC_LIBS@ @AUDIOFILE_LIBS@ \
                 compat/@COMPAT_OSNAME@/libcompatos.a

noinst_HEADERS = compat.h ide.h utils.h audio2tape.h importer/interpolator.h \
                 importer/schmitt.h importer/simple.h importer/soundfile.h \
                 importer/trigger.h converter/findpilot.h \
                 converter/findsync1.h converter/getpulse1.h \
                 converter/getpulse2.h converter/getsync2.h \
                 converter/romloader.h converter/romloaderstate.h \
                 compat/getopt.h fmfconv.h movie_tables.h

EXTRA_DIST = m4/audiofile.m4 \
             m4/glib-1.0.m4 \
             m4/glib-2.0.m4 \
             m4/iconv.m4

if COMPAT_WIN32
package_win32=$(PACKAGE)-$(PACKAGE_VERSION)-win32
top_win32dir=$(top_builddir)/$(package_win32)

install-win32: all
	test -n "$(DESTDIR)" || { echo "ERROR: set DESTDIR path"; exit 1; }
	$(MKDIR_P) $(DESTDIR)/ || exit 1
#	Copy executables (we should manually copy the required libraries)
	list='$(bin_PROGRAMS)'; \
	for file in $$list; do \
	  if test -f "$(top_builddir)/.libs/$$file"; then \
	    cp "$(top_builddir)/.libs/$$file" $(DESTDIR); \
	  else \
	    cp "$(top_builddir)/$$file" $(DESTDIR); \
	  fi; \
	done
#	Get text files
	for file in AUTHORS ChangeLog COPYING README; \
	  do cp "$(top_srcdir)/$$file" "$(DESTDIR)/$$file.txt"; \
	done
#	Get manuals
	topsrcdirstrip=`echo "$(top_srcdir)/man" | sed 's/[].[^$$\\*]/\\\\&/g'`; \
	man_files=`for file in $(top_srcdir)/man/*.1; do echo "$$file"; done | \
	  sed -e "s|^$$topsrcdirstrip/||" -e "s|.1$$||"`; \
	for file in $$man_files; do \
	  if test -n "$(MAN2HTML)"; then \
	    $(MAN2HTML) -r "$(top_srcdir)/man/$$file.1" | sed '1d' > "$(DESTDIR)/$$file.html"; \
	  else \
	    test -z "$(GROFF)" || $(GROFF) -Thtml -man "$(top_srcdir)/man/$$file.1" > "$(DESTDIR)/$$file.html"; \
	  fi; \
	done
#	Convert to DOS line endings
	test -z "$(UNIX2DOS)" || find $(DESTDIR) -type f \( -name "*txt" -or -name "*html" \) -exec $(UNIX2DOS) -q {} \;

install-win32-strip: install-win32
	test -z "$(STRIP)" || { list='$(bin_PROGRAMS)'; \
	for file in $$list; do \
	  test -z "$(STRIP)" || $(STRIP) "$(DESTDIR)/$$file"; \
	done }

dist-win32-dir:
	$(MAKE) DESTDIR="$(top_win32dir)" install-win32-strip

dist-win32-zip: dist-win32-dir
	rm -f -- $(top_builddir)/$(package_win32).zip
	rm -f -- $(top_builddir)/$(package_win32).zip.sha1
	test -n "$(top_win32dir)" || exit 1
	@test `find $(top_win32dir) -type f -name \*.dll -print | wc -l` -ne 0 || \
	{ echo "ERROR: external libraries not found in $(top_win32dir). Please, manually copy them."; exit 1; }
	cd $(top_win32dir) && \
	zip -q -9 -r $(abs_top_builddir)/$(package_win32).zip .
	-sha1sum $(top_builddir)/$(package_win32).zip > $(top_builddir)/$(package_win32).zip.sha1 && \
	{ test -z "$(UNIX2DOS)" || $(UNIX2DOS) $(top_builddir)/$(package_win32).zip.sha1; }

dist-win32-7z: dist-win32-dir
	rm -f -- $(top_builddir)/$(package_win32).7z
	rm -f -- $(top_builddir)/$(package_win32).7z.sha1
	test -n "$(top_win32dir)" || exit 1
	@test `find $(top_win32dir) -type f -name \*.dll -print | wc -l` -ne 0 || \
	{ echo "ERROR: external libraries not found in $(top_win32dir). Please, manually copy them."; exit 1; }
	cd $(top_win32dir) && \
	7z a -t7z -m0=lzma -mx=9 -mfb=64 -md=32m -ms=on -bd $(abs_top_builddir)/$(package_win32).7z .
	-sha1sum $(top_builddir)/$(package_win32).7z > $(top_builddir)/$(package_win32).7z.sha1 && \
	{ test -z "$(UNIX2DOS)" || $(UNIX2DOS) $(top_builddir)/$(package_win32).7z.sha1; }

dist-win32: dist-win32-zip dist-win32-7z

distclean-win32:
	if test -d "$(top_builddir)/$(package_win32)"; then \
	  rm -rf -- "$(top_builddir)/$(package_win32)"; \
	fi
	rm -f -- $(top_builddir)/$(package_win32).zip
	rm -f -- $(top_builddir)/$(package_win32).zip.sha1
	rm -f -- $(top_builddir)/$(package_win32).7z
	rm -f -- $(top_builddir)/$(package_win32).7z.sha1

.PHONY: install-win32 install-win32-strip \
	dist-win32 dist-win32-dir dist-win32-zip dist-win32-7z distclean-win32

endif
