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

const char *
compat_socket_get_strerror( void )
{
  static TCHAR buffer[256];
  TCHAR *ptr;
  DWORD msg_size;

  /* get description of last winsock error */
  msg_size = FormatMessage( 
               FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
               NULL, WSAGetLastError(),
               MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
               buffer, sizeof( buffer ) / sizeof( TCHAR ), NULL );

  if( !msg_size ) return NULL;

  /* skip 'new line' like chars */
  for( ptr = buffer; *ptr; ptr++ ) {
    if( ( *ptr == '\r' ) || ( *ptr == '\n' ) ) {
      *ptr = '\0';
      break;
    }
  }

  return (const char *)buffer;
}

compat_socket_selfpipe_t *compat_socket_selfpipe_alloc( void )
{
  fuse_abort();
  return NULL;
}

void compat_socket_selfpipe_free( compat_socket_selfpipe_t *self )
{
  fuse_abort();
}

compat_socket_t compat_socket_selfpipe_get_read_fd( compat_socket_selfpipe_t *self )
{
  fuse_abort();
  return compat_socket_invalid;
}

void compat_socket_selfpipe_wake( compat_socket_selfpipe_t *self )
{
  fuse_abort();
}

void compat_socket_selfpipe_discard_data( compat_socket_selfpipe_t *self )
{
  fuse_abort();
}
