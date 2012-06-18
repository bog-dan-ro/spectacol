/* gtkcompat.h: various compatibility bits between GTK+ versions
   Copyright (c) 2012 Philip Kendall

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

#ifndef FUSE_GTKCOMPAT_H
#define FUSE_GTKCOMPAT_H

#include <gtk/gtk.h>

#ifndef GTK_TYPE_COMBO_BOX_TEXT

#define GTK_COMBO_BOX_TEXT( X ) GTK_COMBO_BOX( X )
#define gtk_combo_box_text_new() gtk_combo_box_new_text()
#define gtk_combo_box_text_append_text( X, Y ) gtk_combo_box_append_text( X, Y )

#endif				/* #ifndef GTK_TYPE_COMBO_BOX_TEXT */

#endif				/* #ifndef FUSE_GTKCOMPAT_H */

