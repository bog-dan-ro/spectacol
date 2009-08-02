/* compat.h: various compatbility bits
   Copyright (c) 2003 Philip Kendall

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

   E-mail: philip-fuse@shadowmagic.org.uk

*/

#ifndef FUSE_UTILS_COMPAT_H
#define FUSE_UTILS_COMPAT_H

/* Certain brain damaged operating systems (DOS/Windows) treat text
   and binary files different in open(2) and need to be given the
   O_BINARY flag to tell them it's a binary file */
#ifndef O_BINARY
#define O_BINARY 0
#endif				/* #ifndef O_BINARY */

/* Certain operating systems don't define group/other permissions */
#ifndef S_IRGRP
#define S_IRGRP 0
#endif				/* #ifndef S_IRGRP */
#ifndef S_IWGRP
#define S_IWGRP 0
#endif				/* #ifndef S_IWGRP */
#ifndef S_IROTH
#define S_IROTH 0
#endif				/* #ifndef S_IROTH */
#ifndef S_IWOTH
#define S_IWOTH 0
#endif				/* #ifndef S_IWOTH */

char *compat_basename( char *path );
int compat_osname( char *buffer, size_t length );

#endif				/* #ifndef FUSE_UTILS_COMPAT_H */
