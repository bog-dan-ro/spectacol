/* soundfile.cc: Convert audio files (wav, voc, etc.) to lists of pulses
   Copyright (c) 2007-2008 Fredrick Meunier

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

#include <memory>
#include <iostream>
#include <iomanip>
#include <sstream>

#include <audiofile.h>

#include "interpolator.h"
#include "schmitt.h"
#include "soundfile.h"

soundfile::soundfile( std::string filename, trigger* edge_detector,
                      double source_machine_hz, bool show_stats )
  : source_machine_hz(source_machine_hz), edge_detector(edge_detector)
{
  if( !edge_detector )
    throw audio2tape_exception("missing edge detector");

  amplitude_frequency_table.reserve(256);
  for( size_t i = 0; i < amplitude_frequency_table.capacity(); i++ ) {
    amplitude_frequency_table[i] = 0;
  }

  /* The track we're using in the file */
  int track = AF_DEFAULT_TRACK; 

  /* Our filehandle from libaudiofile */
  AFfilehandle handle = afOpenFile( filename.c_str(), "r", NULL );
  if( handle == AF_NULL_FILEHANDLE ) {
    std::ostringstream err;
    err << "couldn't open '" << filename << "'";
    throw audio2tape_exception(err.str().c_str());
  }

  if( afSetVirtualSampleFormat( handle, track, AF_SAMPFMT_UNSIGNED, 8 ) ) {
    afCloseFile( handle );
    throw audio2tape_exception("couldn't set virtual sample format");
  }

  if( afSetVirtualChannels( handle, track, 1 ) ) {
    afCloseFile( handle );
    throw audio2tape_exception("couldn't set virtual channel count");
  }

  int length = afGetFrameCount( handle, track );

  libspectrum_byte *buffer =
    (libspectrum_byte *)malloc(length * afGetChannels(handle, track));

  int frames = afReadFrames( handle, track, buffer, length );
  if( frames == -1 ) {
    std::ostringstream err;
    err << "Can't calculate number of audio frames in '" << filename << "'";
    free(buffer);
    afCloseFile( handle );
    throw audio2tape_exception(err.str().c_str());
  }

  if( !length ) {
    free(buffer);
    afCloseFile( handle );
    throw audio2tape_exception("Empty audiofile, nothing to convert");
  }

  if( frames != length ) {
    std::ostringstream err;
    err << "Read " << frames << " frames, but expected " << length;
    free(buffer);
    afCloseFile( handle );
    throw audio2tape_exception(err.str().c_str());
  }

  sample_rate = afGetRate( handle, track );

  for( libspectrum_byte* byte = buffer; byte < buffer+length; byte++ ) {
    amplitude_frequency_table[*byte]++;
  }

  interpolator upsampler( buffer, length, afGetRate( handle, track ),
                          source_machine_hz );

  libspectrum_byte last_val =
    edge_detector->get_level( upsampler.get_sample(0) );

  unsigned int val = 0;

  for( double i = 0; i < upsampler.get_length(); i++ ) {
    libspectrum_byte raw_val = upsampler.get_sample(i);
    libspectrum_byte new_val = edge_detector->get_level( raw_val );
    if( last_val != new_val ) {
      pulses.push_back(val);
      val = 0;
      last_val = new_val;
    }
    val++;
  }

  // and the last pulse
  pulses.push_back(val);

  if( afCloseFile( handle ) ) {
    free(buffer);
    throw audio2tape_exception("failed to close audio file");
  }

  free(buffer);

  if( show_stats ) display_stats();
}

soundfile::~soundfile()
{
}

const pulse_list&
soundfile::get_pulse_list()
{
  return pulses;
}

void
soundfile::display_stats()
{
  int i, j;
  std::cout << "Frequency table:\n";
  for( i = 0; i < 32; i++ ) {
    unsigned int subtotal = 0;
    for( j = 0; j < 8; j++ ) {
      subtotal += amplitude_frequency_table[i*8+j];
    }
    std::cout << std::setw(3) << i*8 << " - " << std::setw(3) << i*8+7 << " | "
      << subtotal << "\n";
  }
}

void
soundfile::get_tape_block( libspectrum_tape *tape, double start_tstates,
                           double end_tstates )
{
  if( start_tstates >= end_tstates ) return;

  // Worst case we will write all pulses and they are all larger than 256
  // tstates
  libspectrum_byte *data = (libspectrum_byte *)malloc( pulses.size()*5 );
  size_t data_used = 0;
  
  libspectrum_tape_block *block =
      libspectrum_tape_block_alloc( LIBSPECTRUM_TAPE_BLOCK_RLE_PULSE );

  libspectrum_tape_block_set_scale( block, 1 );

  // Find start of relevant section of pulse_list
  double tstates = 0;
  pulse_list::const_iterator i;
  for( i = pulses.begin(); i != pulses.end() && tstates != start_tstates;
       i++ ) {
    tstates += *i;
  }

  // while position < end_tstates
  while( tstates < end_tstates ) {
    // Convert unsigned int pulse to byte w/zero marking pulses bigger than 256
    // which are LSB unsigned ints then write pulse to pulse buffer
    if( *i <= 0xff ) {
      data[ data_used++ ] = *i;
    } else {
      data[ data_used++ ] = 0;
      data[ data_used++ ] = ( *i & 0x000000ff )      ;
      data[ data_used++ ] = ( *i & 0x0000ff00 ) >>  8;
      data[ data_used++ ] = ( *i & 0x00ff0000 ) >> 16;
      data[ data_used++ ] = ( *i & 0xff000000 ) >> 24;
    }
    tstates += *i++;
  }
  std::cout << "data_used:" << data_used << "\n";
  if( data_used ) {
    libspectrum_tape_block_set_data_length( block, data_used );
    libspectrum_tape_block_set_data( block, data );

    libspectrum_tape_append_block( tape, block );
  }
}
