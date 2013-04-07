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

#ifndef ROMLOADER_H
#define ROMLOADER_H

#include <vector>

#include <libspectrum.h>

#include "converter/romloaderstate.h"
#include "importer/soundfile.h"

typedef std::vector<libspectrum_byte> byte_list;

class rom_block {
  public:
  double tstate_start;
  double tstate_end;
  size_t pilot_count;
  unsigned int pilot_length;
  unsigned int sync1_length;
  unsigned int sync2_length;
  unsigned int zero_length;
  unsigned int one_length;
  unsigned int pause_length;
  byte_list data;
};

typedef std::vector<rom_block> rom_block_list;

class romloader {
  public:
    romloader( double source_machine_hz, bool show_stats );
    virtual ~romloader();

    void handle_pulse( double tstates, unsigned int pulse_length );

    void add_pilot_pulse( double tstates, unsigned int pulse_length );

    size_t get_block_count();
    void get_block( libspectrum_tape *tape, size_t number,
                    bool standard_timings );
    double get_block_start( size_t number ); 
    double get_block_end( size_t number );

    size_t get_pilot_count();
    void reset_pilot_count();
    void add_zero_pulse( unsigned int pulse_length );
    void add_one_pulse( unsigned int pulse_length );
    void set_sync1_pulse( unsigned int pulse_length );
    void set_sync2_pulse( unsigned int pulse_length );

    void add_bit( libspectrum_byte bit );
    int get_bits_through_byte();
    void end_block( double end_marker, double end_tstates );

    bool check_checksum();
    void stats( std::string type, pulse_list& data, int standardPulse,
                unsigned int& average );

    void change_state( romloaderstate* new_state );

    static const unsigned int PILOT_LENGTH = 2168;
    static const unsigned int SYNC1 = 667;
    static const unsigned int SYNC2 = 735;
    static const unsigned int ZERO = 855;
    static const unsigned int ONE = 1710;
    static const unsigned int ROM_END_MARKER = 945;

    static const unsigned int PILOT_MAX = 3000;
    static const unsigned int SYNC1_MAX = 1100;
    static const unsigned int SYNC_TOTAL_MAX = 3594;
    static const unsigned int ZERO_THRESHOLD = 2400;
    static const unsigned int DATA_TOTAL_MAX = 5044;
    static const unsigned int MIN_PILOT_COUNT = 256;

  private:
    std::string uchar2hex(libspectrum_byte inbyte);

    romloaderstate* m_rom_loader_state;

    bool show_stats;
    double source_machine_hz;
    double first_pilot_tstates;
    pulse_list pilot_pulses;
    pulse_list zero_pulses;
    pulse_list one_pulses;

    byte_list data;

    libspectrum_byte current_byte;
    int num_bits;

    unsigned int sync1_length;
    unsigned int sync2_length;

    rom_block_list blocks;
};

#endif /* #define ROMLOADER_H */
