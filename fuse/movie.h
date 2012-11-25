/* movie.h: header for movie.c
   Copyright (c) 2006 Gergely Szasz

   $Id: $

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

   Author contact information:

   E-mail: pak21-fuse@srcf.ucam.org
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

*/

/*
    FSMF - Fuse Spectrum Movie File
*/
extern int movie_recording;

void movie_init( void );
void movie_start( const char *name );
void movie_stop( void );
void movie_pause( void );
void movie_add_area( int x, int y, int w, int h );
void movie_start_frame( void );
void movie_init_sound( int f, int s );
void movie_add_sound( libspectrum_signed_word *buf, int len );
