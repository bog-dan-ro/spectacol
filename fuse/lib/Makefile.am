## Process this file with automake to produce Makefile.in
## Copyright (c) 1999-2004 Philip Kendall

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

lib_files = \
            lib/cassette.bmp \
            lib/keyboard.scr \
            lib/microdrive.bmp \
            lib/plus3disk.bmp

lib_snaps = \
            disk_plus3.szx \
            tape_128.szx \
            tape_16.szx \
            tape_2048.szx \
            tape_2068.szx \
            tape_48.szx \
            tape_48_ntsc.szx \
            tape_plus2.szx \
            tape_plus2a.szx \
            tape_plus3.szx \
            tape_plus3e.szx \
            tape_pentagon.szx \
            tape_scorpion.szx \
            tape_se.szx \
            tape_ts2068.szx

lib_compressed_snaps = \
                       lib/compressed/disk_plus3.szx \
                       lib/compressed/tape_128.szx \
                       lib/compressed/tape_16.szx \
                       lib/compressed/tape_2048.szx \
                       lib/compressed/tape_2068.szx \
                       lib/compressed/tape_48.szx \
                       lib/compressed/tape_48_ntsc.szx \
                       lib/compressed/tape_plus2.szx \
                       lib/compressed/tape_plus2a.szx \
                       lib/compressed/tape_plus3.szx \
                       lib/compressed/tape_plus3e.szx \
                       lib/compressed/tape_pentagon.szx \
                       lib/compressed/tape_scorpion.szx \
                       lib/compressed/tape_se.szx \
                       lib/compressed/tape_ts2068.szx

lib_uncompressed_snaps = \
                         lib/uncompressed/disk_plus3.szx \
                         lib/uncompressed/tape_128.szx \
                         lib/uncompressed/tape_16.szx \
                         lib/uncompressed/tape_2048.szx \
                         lib/uncompressed/tape_2068.szx \
                         lib/uncompressed/tape_48.szx \
                         lib/uncompressed/tape_48_ntsc.szx \
                         lib/uncompressed/tape_plus2.szx \
                         lib/uncompressed/tape_plus2a.szx \
                         lib/uncompressed/tape_plus3.szx \
                         lib/uncompressed/tape_plus3e.szx \
                         lib/uncompressed/tape_pentagon.szx \
                         lib/uncompressed/tape_scorpion.szx \
                         lib/uncompressed/tape_se.szx \
                         lib/uncompressed/tape_ts2068.szx

lib_tests = \
            lib/tests/fatware-success.hdf.bz2 \
            lib/tests/idedos-success.hdf.bz2 \
            lib/tests/success.mdr \
            lib/tests/success.mgt.bz2 \
            lib/tests/success.opd \
            lib/tests/success.udi

pkgdata_DATA += \
                $(lib_@AUTOLOAD_SNAPS@_snaps) \
                $(lib_files)

EXTRA_DIST += \
              $(lib_compressed_snaps) \
              $(lib_files) \
              $(lib_tests) \
              $(lib_uncompressed_snaps)
