/* utils.h: some useful helper functions
   Copyright (c) 1999-2003 Philip Kendall

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

#ifndef FUSE_UTILS_H
#define FUSE_UTILS_H

#include <libspectrum.h>

#include "compat.h"

typedef struct utils_file {

  unsigned char *buffer;
  size_t length;

} utils_file;

/* The types of auxiliary file we can look for */
typedef enum utils_aux_type {

  UTILS_AUXILIARY_LIB,		/* Something from the lib/ directory */
  UTILS_AUXILIARY_ROM,		/* Something from the roms/ directory */
  UTILS_AUXILIARY_WIDGET,	/* Something from the widget/ directory */

} utils_aux_type;

int utils_open_file( const char *filename, int autoload,
		     libspectrum_id_t *type );
int utils_open_snap( void );
int utils_read_auxiliary_file( const char *filename, utils_file *file,
                               utils_aux_type type );

int utils_read_file( const char *filename, utils_file *file );
int utils_read_fd( compat_fd fd, const char *filename, utils_file *file );
int utils_close_file( utils_file *file );

int utils_write_file( const char *filename, const unsigned char *buffer,
		      size_t length );
int utils_make_temp_file( int *fd, char *tempfilename, const char *filename,
			  const char *template );

int utils_find_file_path( const char *filename, char *path,
                          utils_aux_type type );

int utils_read_screen( const char *filename, utils_file *screen );

char* utils_safe_strdup( const char *src );

#endif			/* #ifndef FUSE_UTILS_H */
