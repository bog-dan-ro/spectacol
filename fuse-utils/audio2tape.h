/* audio2tape.h: Convert audio files (wav, voc, etc.) to tape files (.tzx,.csw)
   Copyright (c) 2008 Fredrick Meunier

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

   E-mail: fredm@spamcop.net

*/

#ifndef AUDIO2TAPE_H
#define AUDIO2TAPE_H

#include <string.h>

class audio2tape_exception : public std::exception
{
  public:
  audio2tape_exception( const char* message ) {
    strncpy( text, message, sizeof(text)-1 );
    text[sizeof(text)-1] = '\0';
  }
  ~audio2tape_exception() throw();

  char const* what() const throw() {
    return text;
  }

  private:
  static const unsigned int BUFLEN = 255;
  char text[BUFLEN];
};

#endif /* #ifndef AUDIO2TAPE_H */
