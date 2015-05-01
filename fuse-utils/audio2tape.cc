/* audio2tape.cc: Convert audio files (wav, voc, etc.) to tape files (.tzx,.csw)
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

#include <config.h>

#include <exception>
#include <iostream>
#include <map>
#include <sstream>

#include <errno.h>
#include <getopt.h>
#include <math.h>
#include <string.h>
#include <unistd.h>

#include <libspectrum.h>

extern "C" {
#include "utils.h"
}

#include "audio2tape.h"
#include "importer/schmitt.h"
#include "importer/simple.h"
#include "importer/soundfile.h"
#include "converter/romloader.h"

static void write_tape( char *filename, libspectrum_tape *tape );
static void get_pause_block( libspectrum_tape *tape, double source_machine_hz,
                             double start_tstates, double end_tstates );
static trigger*
set_trigger_type_from_string( const std::string trigger_str,
                              libspectrum_byte simple_crossover_point,
                              libspectrum_byte schmitt_low2high_crossover_point,
                              libspectrum_byte schmitt_high2low_crossover_point
                            );
static void
show_version( void );

static void
show_help( void );

std::string progname;

const std::string schmitt_str("schmitt");
const std::string simple_str("simple");

static trigger*
set_trigger_type_from_string( const std::string trigger_str,
                              libspectrum_byte simple_crossover_point,
                              libspectrum_byte schmitt_low2high_crossover_point,
                              libspectrum_byte schmitt_high2low_crossover_point
                            )
{
  if( trigger_str == simple_str ) {
    return new simple(simple_crossover_point);
  } else if( trigger_str == schmitt_str ) {
    return new schmitt( schmitt_high2low_crossover_point,
                        schmitt_low2high_crossover_point );
  } else {
    std::ostringstream err;
    err << "unrecognised trigger type: " << trigger_str;
    throw audio2tape_exception(err.str().c_str());
  }
}

audio2tape_exception::~audio2tape_exception() throw()
{
}

int
main( int argc, char **argv )
{
  int c, error;
  libspectrum_tape *tzx = 0;
  bool standard_rom_timings = false;
  bool keep_unrecognised_blocks = false;
  libspectrum_byte zero_point = 0x7f;
  libspectrum_byte schmitt_noise_threshold = 8;
  bool show_stats = false;
  double source_machine_hz = 3500000.0;

  struct option long_options[] = {
    { "version", 0, NULL, 'V' },
    { "help", 0, NULL, 'h' },
    { 0, 0, 0, 0 }
  };

  progname = argv[0];

  error = init_libspectrum(); if( error ) return error;

  std::string trigger_type_string = schmitt_str;

  while( ( c = getopt_long( argc, argv, "t:srkz:c:hV", long_options,
                            NULL ) ) != -1 ) {

    switch( c ) {

    case 't':
      trigger_type_string = optarg;
      break;

    case 'r':
      standard_rom_timings = true;
      break;

    case 'k':
      keep_unrecognised_blocks = true;
      break;

    case 's':
      show_stats = true;
      break;

    case 'z':
      zero_point = atol( optarg );
      break;

    case 'c':
      schmitt_noise_threshold = atol( optarg );
      break;

    case 'h':
      show_help();
      return 0;

    case 'V':
      show_version();
      return 0;

    case '?':
      /* getopt prints an error message to stderr */
      error = 1;
      break;

    default:
      std::cerr << progname << ": unknown option `" << (char) c << "'\n";
      error = 1;
      break;

    }
  }

  argc -= optind;
  argv += optind;

  if( error ) {
    std::cerr <<  "Try `audio2tape --help' for more information.\n";
    return error;
  }

  if( argc < 2 ) {
    std::cerr << progname << ": usage: " << progname <<
      " [-r] [-k] [-s] [-t <trigger type>] [-z <zero level>] "
      "[-c <schmitt noise threshold>] <infile> <outfile>\n";

    std::cerr <<  "Try `audio2tape --help' for more information.\n";
    return 1;
  }

  try {
    trigger* t =
      set_trigger_type_from_string( trigger_type_string,
                                    zero_point,
                                    zero_point+schmitt_noise_threshold,
                                    zero_point-schmitt_noise_threshold
                                  );
    soundfile sf(argv[0], t, source_machine_hz, show_stats);

    const pulse_list& pulses(sf.get_pulse_list());

    romloader rl( source_machine_hz, show_stats );
    double tstates = 0;

    for( pulse_list::const_iterator i = pulses.begin();
         i != pulses.end(); i++ ) {
      rl.handle_pulse( tstates, *i );
      tstates += *i;
    }

    // get blocks from ROMLoader, filling gaps with data from original tape
    std::cout << "found " << rl.get_block_count() << " ROM blocks\n";

    tzx = libspectrum_tape_alloc();
    double tstate_start = 0;
    double tstate_end = 0;

    for( size_t i = 0; i < rl.get_block_count(); i++ ) {
      tstate_end = rl.get_block_start( i ); 
      if( tstate_start != tstate_end ) {
        if( keep_unrecognised_blocks || tstate_start ) {
          if( keep_unrecognised_blocks ) {
            // get some original tape pulses and pop it in the tape
            sf.get_tape_block( tzx, tstate_start, tstate_end );
          } else {
            get_pause_block( tzx, source_machine_hz, tstate_start,
                             tstate_end );
          }
        }

        // and now the ROM block
        rl.get_block( tzx, i, standard_rom_timings );

        tstate_start = rl.get_block_end( i ); 
      }
    }

    // last tape tstates
    if( keep_unrecognised_blocks && tstate_start != tstates ) {
      // the last original tape pulses and pop it in the tape
      sf.get_tape_block( tzx, tstate_start, tstates );
    }

    write_tape( argv[1], tzx );

    libspectrum_tape_free( tzx );
  }
  catch ( std::exception& e ) {
    if( tzx ) libspectrum_tape_free( tzx );
    std::cerr << progname << ": Fatal error: " << e.what() << "\n";
    return 1;
  }

  return 0;
}

static void
write_tape( char *filename, libspectrum_tape *tape )
{
  libspectrum_byte *buffer; size_t length;
  FILE *f;
  libspectrum_id_t type;
  libspectrum_class_t cls;
  int error;

  /* Work out what sort of file we want from the filename; default to
     .tzx if we couldn't guess */
  error = libspectrum_identify_file_with_class( &type, &cls, filename, NULL,
						0 );
  if( error )
    throw audio2tape_exception("Couldn't identify type of output file");

  if( cls != LIBSPECTRUM_CLASS_TAPE || type == LIBSPECTRUM_ID_UNKNOWN )
    type = LIBSPECTRUM_ID_TAPE_TZX;

  length = 0;

  if( libspectrum_tape_write( &buffer, &length, tape, type ) ) {
    std::string err("error writing tape buffer");
    throw audio2tape_exception(err.c_str());
  }

  f = fopen( filename, "wb" );
  if( !f ) {
    std::ostringstream err;
    err << "couldn't open '" << filename << "': " << strerror( errno );
    free( buffer );
    throw audio2tape_exception(err.str().c_str());
  }
    
  if( fwrite( buffer, 1, length, f ) != length ) {
    std::ostringstream err;
    err << "error writing to '" << filename << "'";
    free( buffer );
    fclose( f );
    throw audio2tape_exception(err.str().c_str());
  }

  free( buffer );

  if( fclose( f ) ) {
    std::ostringstream err;
    err << "couldn't close '" << filename << "': " << strerror( errno );
    throw audio2tape_exception(err.str().c_str());
  }
}

void
get_pause_block( libspectrum_tape *tape, double source_machine_hz,
                 double start_tstates, double end_tstates )
{
  if( start_tstates >= end_tstates ) return;

  libspectrum_tape_block *block =
      libspectrum_tape_block_alloc( LIBSPECTRUM_TAPE_BLOCK_PAUSE );

  double pause_ms = ceil( ( end_tstates - start_tstates ) /
                          ( source_machine_hz * 1000 ) );
  libspectrum_tape_block_set_pause( block, pause_ms );
  libspectrum_tape_append_block( tape, block );
}

static void
show_version( void )
{
  std::cout <<
    "audio2tape (" PACKAGE ") " PACKAGE_VERSION "\n"
    "Copyright (c) 2007-2008 Fredrick Meunier\n"
    "License GPLv2+: GNU GPL version 2 or later "
    "<http://gnu.org/licenses/gpl.html>\n"
    "This is free software: you are free to change and redistribute it.\n"
    "There is NO WARRANTY, to the extent permitted by law.\n";
}

static void
show_help( void )
{
  std::cout <<
    "Usage: audio2tape [OPTION]... <infile> <outfile>\n"
    "Converts audio files to ZX Spectrum tape images.\n"
    "\n"
    "Options:\n"
    "  -t <type>      Set the level detection trigger: 'simple' or 'schmitt';\n"
    "                   defaults to 'schmitt'.\n"
    "  -s             Display frequency graph showing the sound levels.\n"
    "  -r             Use idealised timing figures for ROM blocks that are\n"
    "                   recognised.\n"
    "  -k             Keep any audio data from the original audio file that isn't\n"
    "                   recognised.\n"
    "  -z <level>     Set the level to be used as zero point in the input file.\n"
    "                   Defaults to 127 from a range of 0-255.\n"
    "  -c <level>     Set the level to be used as as the Schmitt trigger noise\n"
    "                   threshold. Defaults to 8 from a range of 0-127.\n"
    "  -h, --help     Display this help and exit.\n"
    "  -V, --version  Output version information and exit.\n"
    "\n"
    "Report audio2tape bugs to <" PACKAGE_BUGREPORT ">\n"
    "fuse-utils home page: <" PACKAGE_URL ">\n"
    "For complete documentation, see the manual page of audio2tape.\n";
}
