#include "test.h"

test_return_t
check_edges( const char *filename, test_edge_sequence_t *edges,
	     int flags_mask )
{
  libspectrum_byte *buffer = NULL;
  size_t filesize = 0;
  libspectrum_tape *tape;
  test_return_t r = TEST_FAIL;
  test_edge_sequence_t *ptr = edges;
  libspectrum_init_t init = libspectrum_default_init();
  libspectrum_init(&init);
  if( read_file( &buffer, &filesize, filename ) ) return TEST_INCOMPLETE;

  tape = libspectrum_tape_alloc(init.context);

  if( libspectrum_tape_read( tape, buffer, filesize, LIBSPECTRUM_ID_UNKNOWN,
			     filename ) != LIBSPECTRUM_ERROR_NONE ) {
    libspectrum_tape_free( tape );
    libspectrum_free( buffer );
    libspectrum_end( init.context );
    return TEST_INCOMPLETE;
  }

  libspectrum_free( buffer );

  while( 1 ) {

    libspectrum_dword tstates;
    int flags;
    libspectrum_error e;

    e = libspectrum_tape_get_next_edge( &tstates, &flags, tape );
    if( e ) {
      libspectrum_tape_free( tape );
      libspectrum_end( init.context );
      return TEST_INCOMPLETE;
    }

    flags &= flags_mask;

    if( tstates != ptr->length || flags != ptr->flags ) {
      fprintf( stderr, "%s: expected %d tstates and flags %d, got %d tstates and flags %d\n",
	       progname, ptr->length, ptr->flags, tstates, flags );
      break;
    }

    if( tstates != ptr->length ) {
      fprintf( stderr, "%s: expected %d tstates, got %d tstates\n", progname,
	       ptr->length, tstates );
      break;
    }

    if( --ptr->count == 0 ) {
      ptr++;
      if( ptr->length == -1 ) {
	r = TEST_PASS;
	break;
      }
    }
  }

  if( libspectrum_tape_free( tape ) ) {
      libspectrum_end( init.context );
      return TEST_INCOMPLETE;
  }

  libspectrum_end( init.context );
  return r;
}

  
