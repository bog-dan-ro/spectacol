/* svg.h: Routines to capture ROM graphics statements to Scalable Vector
          Graphics files
   Copyright (c) 2014 Stefano Bodrato
   Portions taken from svgwrite.c, (c) J.J. Green 2005

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

   E-mail: stefano@bodrato.it

*/

#ifndef FUSE_SVG_H
#define FUSE_SVG_H

extern int svg_capture_active;     /* SVG capture enabled? */

extern void svg_startcapture( const char *name );

extern void svg_stopcapture( void );

void svg_capture( void );
void svg_capture_end( void );

#define SVG_CAPTURE_DOTS   1
#define SVG_CAPTURE_LINES  2

extern int svg_capture_mode;     /* SVG capture enabled? */

#endif				/* #ifndef FUSE_SVG_H */
