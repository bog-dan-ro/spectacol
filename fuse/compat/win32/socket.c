/* file.c: Socket-related compatibility routines
   Copyright (c) 2011 Sergio Baldoví, Philip Kendall

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

#include <config.h>

#include <winsock2.h>

#include "compat.h"

const compat_socket_t compat_socket_invalid = INVALID_SOCKET;

int
compat_socket_close( compat_socket_t fd )
{
  return closesocket( fd );
}

int compat_socket_get_error( void )
{
  return WSAGetLastError();
}
