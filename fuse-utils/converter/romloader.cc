/* romloader.h: Class attempting to reproduce the Spectrum standard ROM loader
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

#include <iostream>
#include <limits>

#include <math.h>

#include "findpilot.h"
#include "romloader.h"

romloader::romloader( double source_machine_hz, bool show_stats ) :
  show_stats(show_stats), source_machine_hz(source_machine_hz)
{
  m_rom_loader_state = findpilot::instance();
  first_pilot_tstates = 0;
  current_byte = 0;
  num_bits = 0;
  sync1_length = 0;
  sync2_length = 0;
  data.reserve( 65536 );
  blocks.reserve( 100 );
}

romloader::~romloader() {}

void
romloader::handle_pulse( double tstates, unsigned int pulse_length )
{
  m_rom_loader_state->handle_pulse( this, tstates, pulse_length );
}

void
romloader::add_pilot_pulse( double tstates, unsigned int pulse_length )
{
  if( pilot_pulses.size() == 0 ) {
    first_pilot_tstates = tstates;
  }
  pilot_pulses.push_back( pulse_length );
}

size_t
romloader::get_block_count()
{
  return blocks.size();
}

double
romloader::get_block_start( size_t number )
{
  return blocks[number].tstate_start;
}

double
romloader::get_block_end( size_t number )
{
  return blocks[number].tstate_end;
}

void
romloader::get_block( libspectrum_tape *tape, size_t number,
                      bool standard_timings )
{
  libspectrum_byte *data =
    (libspectrum_byte *)malloc( blocks[number].data.size() );

  libspectrum_tape_block *block;

  if( standard_timings ) {
    block = libspectrum_tape_block_alloc( LIBSPECTRUM_TAPE_BLOCK_ROM );
  } else {
    block = libspectrum_tape_block_alloc( LIBSPECTRUM_TAPE_BLOCK_TURBO );

    libspectrum_tape_block_set_pilot_length( block,
                                             blocks[number].pilot_length );
    libspectrum_tape_block_set_sync1_length( block,
                                             blocks[number].sync1_length );
    libspectrum_tape_block_set_sync2_length( block,
                                             blocks[number].sync2_length );
    libspectrum_tape_block_set_bit0_length( block,
                                             blocks[number].zero_length );
    libspectrum_tape_block_set_bit1_length( block,
                                             blocks[number].one_length );
    libspectrum_tape_block_set_pilot_pulses( block,
                                             blocks[number].pilot_count );
    libspectrum_tape_block_set_bits_in_last_byte( block, 8 );
  }

  for( size_t i = 0; i < blocks[number].data.size(); i++ ) {
    data[i] = blocks[number].data[i];
  }

  libspectrum_tape_block_set_data_length( block, blocks[number].data.size() );
  libspectrum_tape_block_set_data( block, data );
  double pause_ms = ceil( blocks[number].pause_length /
                          ( source_machine_hz * 1000 ) );
  libspectrum_tape_block_set_pause( block, pause_ms );

  libspectrum_tape_append_block( tape, block );
}

size_t
romloader::get_pilot_count()
{
  return pilot_pulses.size();
}

void
romloader::reset_pilot_count()
{
  pilot_pulses.clear();
}

void
romloader::add_zero_pulse( unsigned int pulse_length )
{
  zero_pulses.push_back( pulse_length );
}

void
romloader::add_one_pulse( unsigned int pulse_length )
{
  one_pulses.push_back( pulse_length );
}

void
romloader::set_sync1_pulse( unsigned int pulse_length )
{
  sync1_length = pulse_length;
}

void
romloader::set_sync2_pulse( unsigned int pulse_length )
{
  sync2_length = pulse_length;
}

void
romloader::add_bit( libspectrum_byte bit )
{
  current_byte <<= 1;
  current_byte |= (bit & 0x01);
  if( ++num_bits == 8 ) {
    data.push_back(current_byte);
    current_byte = 0;
    num_bits = 0;
  }
}

int
romloader::get_bits_through_byte()
{
  return num_bits;
}

void
romloader::end_block( double end_marker, double end_tstates )
{
  std::cout << "Block ended, found " << data.size() << " bytes\n";
  bool isHeader = data.size() == 19 && data[0] == 0x00;
  std::cout << "Type: " << (isHeader ? "Header" : "Data") << "\n";
  if( isHeader ) {
    switch( data[1]) {
    case 0:
      std::cout << "Program: ";
      break;
    case 1:
      std::cout << "Number Array: ";
      break;
    case 2:
      std::cout << "Character Array: ";
      break;
    case 3:
      std::cout << "CODE: ";
      break;
    default:
      std::cout << "Unknown: ";
      break;
    }
    for( int i = 2; i < 12; i++) {
      std::cout << data[i];
    }
    std::cout << "\n";
  }

  if( num_bits != 0 ) {
    std::cout << "Error have incomplete byte (" << num_bits << " bits)\n";
  }

  rom_block new_block;
  new_block.tstate_start = first_pilot_tstates;
  new_block.tstate_end = end_marker + end_tstates;
  std::cout << "First block seconds: " <<
    new_block.tstate_start/source_machine_hz << "\n";
  std::cout << "Last block seconds: " <<
    new_block.tstate_end/source_machine_hz << "\n";
  new_block.pilot_count = pilot_pulses.size();
  new_block.sync1_length = sync1_length;
  new_block.sync2_length = sync2_length;
  new_block.pause_length = end_marker;
  new_block.data = data;
  check_checksum();
  stats ( "pilot", pilot_pulses, PILOT_LENGTH, new_block.pilot_length );
  stats ( "zero", zero_pulses, ZERO, new_block.zero_length );
  stats ( "one", one_pulses, ONE, new_block.one_length );
  if( show_stats ) {
    std::cout << "pilot count:" << pilot_pulses.size() << "\n";
    std::cout << "Sync1 pulse:" << new_block.sync1_length << " tstates, "
      << (double)new_block.sync1_length/SYNC1*100 << "% of expected\n";
    std::cout << "Sync2 pulse:" << new_block.sync2_length << " tstates, "
      << (double)new_block.sync2_length/SYNC2*100 << "% of expected\n";
  }

  blocks.push_back( new_block );

  first_pilot_tstates = 0;
  current_byte = 0;
  num_bits = 0;
  sync1_length = 0;
  sync2_length = 0;
  pilot_pulses.clear();
  zero_pulses.clear();
  one_pulses.clear();
  data.clear();
}

bool
romloader::check_checksum()
{
  libspectrum_byte checksum = 0;
  bool retval = false;
  if( data.size() ) {
    for( size_t i = 0; i < data.size()-1; i++ ) {
      checksum ^= data[i];
    }
    retval = checksum == data[data.size()-1];
  }

  std::cout << "Checksum:" << (retval ? "PASS" : "FAIL") << "\n";
  return retval;
}

void
romloader::stats( std::string type, pulse_list& data, int standardPulse,
                  unsigned int& average )
{
  unsigned int low = std::numeric_limits<unsigned int>::max();
  unsigned int high = 0;
  double total = 0;
  for( pulse_list::const_iterator i = data.begin(); i != data.end(); i++ ) {
    if ( *i < low ) {
      low = *i;
    }
    if ( *i > high ) {
      high = *i;
    }
    total += *i;
  }
  average = total/data.size();
  if( show_stats ) {
    std::cout << "shortest " << type << " pulse:" << low << " tstates, longest "
      << type << " pulse:" << high << " tstates\n";
    std::cout << "shortest " << type << " pulse "
      << (double)low/standardPulse*100 << "% of expected, longest " << type
      << " pulse " << (double)high/standardPulse*100 << "% of expected\n";
    std::cout << "average " << type << " pulse:" << average << "\n";
  }
}

void
romloader::change_state( romloaderstate* new_state )
{
  m_rom_loader_state = new_state;
}
