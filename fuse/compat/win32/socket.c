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
const int compat_socket_EBADF = WSAENOTSOCK;

struct compat_socket_selfpipe_t {
  SOCKET self_socket;
};

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

compat_socket_selfpipe_t* compat_socket_selfpipe_alloc( void )
{
  unsigned long mode = 1;
  struct sockaddr_in sa;
  socklen_t sa_len = sizeof(sa);

  compat_socket_selfpipe_t *self = malloc( sizeof( *self ) );
  if( !self ) {
    ui_error( UI_ERROR_ERROR, "%s: %d: out of memory", __FILE__, __LINE__ );
    fuse_abort();
  }
  
  self->self_socket = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
  if( self->self_socket == compat_socket_invalid ) {
    ui_error( UI_ERROR_ERROR,
              "%s: %d: failed to open socket; errno %d: %s\n",
              __FILE__, __LINE__, compat_socket_get_error(),
              compat_socket_get_strerror() );
    fuse_abort();
  }

  /* Set nonblocking mode */
  if( ioctlsocket( self->self_socket, FIONBIO, &mode ) == -1 ) {
    ui_error( UI_ERROR_ERROR, 
              "%s: %d: failed to set socket nonblocking; errno %d: %s\n",
              __FILE__, __LINE__, compat_socket_get_error(),
              compat_socket_get_strerror() );
    fuse_abort();
  }

  memset( &sa, 0, sizeof(sa) );
  sa.sin_family = AF_INET;
  sa.sin_addr.s_addr = htonl( INADDR_LOOPBACK );

  if( bind( self->self_socket, (struct sockaddr*)&sa, sizeof(sa) ) == -1 ) {
    ui_error( UI_ERROR_ERROR,
              "%s: %d: failed to bind socket; errno %d: %s\n",
              __FILE__, __LINE__, compat_socket_get_error(),
              compat_socket_get_strerror() );
    fuse_abort();
  }

  /* Get ephemeral port number */
  if( getsockname( self->self_socket, (struct sockaddr *)&sa, &len ) == -1 ) {
    ui_error( UI_ERROR_ERROR,
              "%s: %d: failed to get socket name; errno %d: %s\n",
              __FILE__, __LINE__, compat_socket_get_error(),
              compat_socket_get_strerror() );
    fuse_abort();
  }

  self->port = sa.sin_port;

  return self;
}

void compat_socket_selfpipe_free( compat_socket_selfpipe_t *self )
{
  compat_socket_close( self->self_socket );
  free( self );
}

compat_socket_t compat_socket_selfpipe_get_read_fd( compat_socket_selfpipe_t *self )
{
  return self->self_socket;
}

void compat_socket_selfpipe_wake( compat_socket_selfpipe_t *self )
{
  struct sockaddr_in sa;

  memset( &sa, 0, sizeof(sa) );
  sa.sin_family = AF_INET;
  sa.sin_addr.s_addr = htonl( INADDR_LOOPBACK );
  memcpy( &sa.sin_port, self->port, 2 );

  sendto( self->self_socket, NULL, 0, 0, (struct sockaddr*)&sa, sizeof(sa) );
}

void compat_socket_selfpipe_discard_data( compat_socket_selfpipe_t *self )
{
  ssize_t bytes_read;
  struct sockaddr_in sa;
  socklen_t sa_length = sizeof(sa);
  static char bitbucket[0x100];

  do {
    /* Socket is non blocking, so we can do this safely */
    bytes_read = recvfrom( self->self_socket, bitbucket, sizeof( bitbucket ),
                           0, (struct sockaddr*)&sa, &sa_length );
  } while( bytes_read != -1 )
}
