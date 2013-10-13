## Process this file with automake to produce Makefile.in
## Copyright (c) 2003-2012 Philip Kendall

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

AM_CPPFLAGS = -I$(srcdir)/..

noinst_HEADERS = getopt.h

noinst_LIBRARIES = libcompatos.a

libcompatos_a_SOURCES =

if COMPAT_DIRNAME
libcompatos_a_SOURCES += dirname.c
endif

if COMPAT_GETOPT
libcompatos_a_SOURCES += getopt.c getopt1.c
endif

if COMPAT_MKSTEMP
libcompatos_a_SOURCES += mkstemp.c
endif

## Amiga routines
if COMPAT_AMIGA
libcompatos_a_SOURCES += \
                         unix/dir.c \
                         unix/file.c \
                         amiga/osname.c \
                         amiga/paths.c \
                         unix/timer.c
endif

## Linux routines
if COMPAT_LINUX
libcompatos_a_SOURCES += \
                         unix/dir.c \
                         unix/file.c \
                         unix/osname.c \
                         linux/paths.c \
                         unix/timer.c

if HAVE_SOCKETS
libcompatos_a_SOURCES += unix/socket.c
endif
endif

## Morphos routines
if COMPAT_MORPHOS
libcompatos_a_SOURCES += \
                         unix/dir.c \
                         unix/file.c \
                         morphos/osname.c \
                         amiga/paths.c \
                         unix/timer.c
endif

## Unix routines
if COMPAT_UNIX
libcompatos_a_SOURCES += \
                         unix/dir.c \
                         unix/file.c \
                         unix/osname.c \
                         unix/paths.c \
                         unix/timer.c

if HAVE_SOCKETS
libcompatos_a_SOURCES += unix/socket.c
endif
endif

## Wii routines
if COMPAT_WII
libcompatos_a_SOURCES += \
                         wii/dir.c \
                         unix/file.c \
                         wii/osname.c \
                         wii/paths.c \
                         wii/timer.c
endif

## Windows routines
if COMPAT_WIN32
libcompatos_a_SOURCES += \
                         unix/dir.c \
                         unix/file.c \
                         win32/osname.c \
                         win32/paths.c \
                         win32/timer.c

if HAVE_SOCKETS
libcompatos_a_SOURCES += win32/socket.c
endif
endif

## SpeccyBoot routines
if HAVE_TUNTAP
libcompatos_a_SOURCES += unix/tuntap.c
endif

AM_CPPFLAGS += @GLIB_CFLAGS@ @GTK_CFLAGS@ @LIBSPEC_CFLAGS@ \
-DFUSEDATADIR="\"${pkgdatadir}\""
