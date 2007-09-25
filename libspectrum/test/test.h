#ifndef TEST_TEST_H
#define TEST_TEST_H

#include "libspectrum.h"

extern const char *progname;

typedef enum test_return_t {
  TEST_PASS,
  TEST_FAIL,
  TEST_INCOMPLETE,
} test_return_t;

typedef struct test_edge_sequence_t {

  libspectrum_dword length;
  size_t count;

} test_edge_sequence_t;

int read_file( libspectrum_byte **buffer, size_t *length,
	       const char *filename );

test_return_t check_edges( const char *filename, test_edge_sequence_t *edges );

test_return_t test_15( void );

#endif
