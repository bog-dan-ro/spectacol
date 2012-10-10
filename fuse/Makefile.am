# Process this file with automake to produce Makefile.in
## Copyright (c) 1999-2009 Philip Kendall

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

bin_PROGRAMS = fuse

SUBDIRS = @COMPAT_SUBDIR@ \
	  data \
	  debugger \
	  hacking \
	  lib \
	  machines \
	  man \
          peripherals \
	  perl \
	  pokefinder \
	  roms \
	  sound \
	  timer \
	  ui \
	  unittests \
	  z80

DIST_SUBDIRS = data \
	       debugger \
	       hacking \
	       lib \
	       machines \
	       man \
               peripherals \
	       perl \
	       pokefinder \
	       roms \
	       sound \
	       timer \
	       ui \
	       unittests \
	       z80

fuse_SOURCES = display.c \
	event.c \
	fuse.c \
	input.c \
	keyboard.c \
	loader.c \
	machine.c \
	memory.c \
	mempool.c \
	menu.c \
	movie.c \
	module.c \
	periph.c \
	profile.c \
	psg.c \
	rectangle.c \
	rzx.c \
	screenshot.c \
	settings.c \
	slt.c \
	snapshot.c \
	sound.c \
	spectrum.c \
	tape.c \
	ui.c \
	uidisplay.c \
	utils.c \
	windres.rc

if COMPAT_DIRNAME
fuse_SOURCES += compat/dirname.c
endif

if COMPAT_GETOPT
fuse_SOURCES += compat/getopt.c compat/getopt1.c
endif

if COMPAT_MKSTEMP
fuse_SOURCES += compat/mkstemp.c
endif

EXTRA_fuse_SOURCES = windres.rc

fuse_LDADD = debugger/libdebugger.a \
@UI_LIBS@ \
debugger/libdebugger.a \
machines/libmachines.a \
peripherals/libperipherals.a \
peripherals/disk/libdisk.a \
peripherals/flash/libflash.a \
peripherals/ide/libide.a \
peripherals/nic/libnic.a \
pokefinder/libpokefinder.a \
sound/libsound.a \
timer/libtimer.a \
ui/scaler/libscaler.a \
unittests/libunittests.a \
z80/libz80.a \
@LIBSPEC_LIBS@ \
@GLIB_LIBS@ \
@PNG_LIBS@ \
@SOUND_LIBS@ \
@SDL_LIBS@ \
@X_LIBS@ \
@XML_LIBS@ \
compat/@COMPAT_OSNAME@/libcompatos.a \
@WINDRES_OBJ@

fuse_DEPENDENCIES = debugger/libdebugger.a \
@UI_LIBS@ \
debugger/libdebugger.a \
machines/libmachines.a \
peripherals/libperipherals.a \
peripherals/disk/libdisk.a \
peripherals/flash/libflash.a \
peripherals/ide/libide.a \
peripherals/nic/libnic.a \
pokefinder/libpokefinder.a \
sound/libsound.a \
timer/libtimer.a \
ui/scaler/libscaler.a \
unittests/libunittests.a \
z80/libz80.a \
compat/@COMPAT_OSNAME@/libcompatos.a \
@WINDRES_OBJ@

BUILT_SOURCES = options.h settings.c settings.h

windres.o: windres.rc winfuse.ico ui/win32/*.rc
	@WINDRES@ -I$(srcdir) -I. $(srcdir)/windres.rc @LIBSPEC_CFLAGS@ @CPPFLAGS@ windres.o

settings.c: settings.pl settings.dat
	@PERL@ -I$(srcdir)/perl $(srcdir)/settings.pl $(srcdir)/settings.dat > $@.tmp && mv $@.tmp $@

settings.h: settings-header.pl settings.dat
	@PERL@ -I$(srcdir)/perl $(srcdir)/settings-header.pl $(srcdir)/settings.dat > $@.tmp && mv $@.tmp $@

if WIDGET
options.h: $(srcdir)/perl/cpp-perl.pl config.h $(srcdir)/ui/@WIDGET@/options-header.pl $(srcdir)/ui/options.dat $(srcdir)/perl/Fuse.pm $(srcdir)/perl/Fuse/Dialog.pm
	@PERL@ $(srcdir)/perl/cpp-perl.pl config.h $(srcdir)/ui/options.dat | @PERL@ -I$(srcdir)/perl $(srcdir)/ui/@WIDGET@/options-header.pl - public > $@.tmp && mv $@.tmp $@
else
options.h: $(srcdir)/perl/cpp-perl.pl config.h $(srcdir)/ui/@UI@/options-header.pl $(srcdir)/ui/options.dat $(srcdir)/perl/Fuse.pm $(srcdir)/perl/Fuse/Dialog.pm
	@PERL@ $(srcdir)/perl/cpp-perl.pl config.h $(srcdir)/ui/options.dat | @PERL@ -I$(srcdir)/perl $(srcdir)/ui/@UI@/options-header.pl - public > $@.tmp && mv $@.tmp $@
endif

AM_CPPFLAGS = @GLIB_CFLAGS@ @GTK_CFLAGS@ @LIBSPEC_CFLAGS@ \
@XML_CFLAGS@ -DFUSEDATADIR="\"${pkgdatadir}\"" @SDL_CFLAGS@

noinst_HEADERS = bitmap.h \
	compat.h \
	display.h \
	event.h \
	fuse.h \
	input.h \
	keyboard.h \
	loader.h \
	machine.h \
	memory.h \
	mempool.h \
	menu.h \
	movie.h \
	movie_tables.h \
	module.h \
	periph.h \
	psg.h \
	rectangle.h \
	rzx.h \
	screenshot.h \
	settings.h \
	slt.h \
	snapshot.h \
	sound.h \
	spectrum.h \
	tape.h \
	utils.h \
	options.h \
	profile.h

EXTRA_DIST = AUTHORS \
	     README \
	     THANKS \
	     keysyms.dat \
	     keysyms.pl \
	     menu_data.dat \
	     menu_data.pl \
	     settings.pl \
	     settings-header.pl \
	     settings.dat \
	     winfuse.ico

CLEANFILES = options.h \
	     settings.c \
	     settings.h

include compat/Makefile.am
