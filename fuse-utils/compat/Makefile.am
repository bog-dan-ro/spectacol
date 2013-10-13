## Process this file with automake to produce Makefile.in
## Copyright (c) 2013 Philip Kendall

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

noinst_LIBRARIES += compat/libcompatos.a

compat_libcompatos_a_SOURCES =

if COMPAT_AMIGA
compat_libcompatos_a_SOURCES += \
                                compat/amiga/basename.c \
                                compat/amiga/osname.c
endif

if COMPAT_UNIX
compat_libcompatos_a_SOURCES += \
                                compat/unix/basename.c \
                                compat/unix/osname.c
endif

if COMPAT_WIN32
compat_libcompatos_a_SOURCES += \
                                compat/unix/basename.c \
                                compat/win32/osname.c
endif

if COMPAT_WIN32
include compat/win32/distribution.mk
endif
