/* scl2trd.c: Convert .SCL disk images to .TRD disk images
   Copyright (c) 2002-2007 Dmitry Sanarin, Philip Kendall and Fredrick Meunier

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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#ifdef HAVE_STRINGS_H
#include <strings.h>		/* Needed for strncasecmp() on QNX6 */
#endif				/* #ifdef HAVE_STRINGS_H */
#include <fcntl.h>

#include "compat.h"
#include "libspectrum.h"

struct options {

  char *sclfile;		/* The SCL file we'll operate on */

  char *trdfile;		/* The TRD file we'll write to */

};

char *progname;			/* argv[0] */

#define TRD_NAMEOFFSET 0x08F5
#define TRD_DIRSTART 0x08E2
#define TRD_DIRLEN 32
#define TRD_MAXNAMELENGTH 8
#define BLOCKSIZE 10240

typedef union {
#ifdef WORDS_BIGENDIAN
  struct { libspectrum_byte b3, b2, b1, b0; } b;
#else
  struct { libspectrum_byte b0, b1, b2, b3; } b;
#endif
  libspectrum_dword dword;
} lsb_dword;

static unsigned int 
lsb2ui(unsigned char *mem)
{
  return (mem[0] + (mem[1] * 256) + (mem[2] * 256 * 256)
          + (mem[3] * 256 * 256 * 256));
}

static void
ui2lsb(unsigned char *mem, unsigned int value)
{
  lsb_dword ret;

  ret.dword = value;

  mem[0] = ret.b.b0;
  mem[1] = ret.b.b1;
  mem[2] = ret.b.b2;
  mem[3] = ret.b.b3;
}

static void 
Scl2Trd(char *oldname, char *newname)
{
  int TRD, SCL, i;

  void *TRDh = NULL;
  void *tmp;

  unsigned char *trd_fsec;
  unsigned char *trd_ftrk;
  unsigned char *trd_files;
  unsigned char size;

  char signature[8];
  unsigned char blocks;
  char headers[256][14];
  void *tmpscl = NULL;
  unsigned long left;
  unsigned long fptr;
  int x;
  ssize_t bytes_read;
  ssize_t bytes_written;

  unsigned char template[34] =
  {0x01, 0x16, 0x00, 0xF0,
    0x09, 0x10, 0x00, 0x00,
    0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20,
    0x20, 0x00, 0x00, 0x64,
    0x69, 0x73, 0x6B, 0x6E,
    0x61, 0x6D, 0x65, 0x00,
    0x00, 0x00, 0x46, 0x55
  };

  FILE *fh;
  unsigned char *mem;

  unlink(newname);

  fh = fopen(newname, "wb");
  if (fh) {
    mem = (unsigned char *) malloc(BLOCKSIZE);
    memset(mem, 0, BLOCKSIZE);

    if (mem) {
      memcpy(&mem[TRD_DIRSTART], template, TRD_DIRLEN);
      strncpy((char*)&mem[TRD_NAMEOFFSET], "Fuse", TRD_MAXNAMELENGTH);
      fwrite((void *) mem, 1, BLOCKSIZE, fh);
      memset(mem, 0, BLOCKSIZE);

      for (i = 0; i < 63; i++)
	fwrite((void *) mem, 1, BLOCKSIZE, fh);

      free(mem);
      fclose(fh);
    }
  }

  if ((TRD = open(newname, O_RDWR | O_BINARY)) == -1) {
    printf("Error - cannot open TRD disk image %s !\n", newname);
    return;
  }

  TRDh = malloc(4096);
  bytes_read = read(TRD, TRDh, 4096);
  if (bytes_read < 4096) {
    printf("Error - cannot read TRD header from %s\n", newname);
    close(TRD);
    free(TRDh);
    return;
  }

  tmp = (char *) TRDh + 0x8E5;
  trd_files = (unsigned char *) TRDh + 0x8E4;
  trd_fsec = (unsigned char *) TRDh + 0x8E1;
  trd_ftrk = (unsigned char *) TRDh + 0x8E2;

  if ((SCL = open(oldname, O_RDONLY | O_BINARY)) == -1) {
    printf("Can't open SCL file %s.\n", oldname);
    goto Abort;
  }

  bytes_read = read(SCL, &signature, 8);
  if (bytes_read < 8) {
    printf("Error - cannot read signature from SCL file %s\n", oldname);
    goto Abort;
  }

  if (strncasecmp(signature, "SINCLAIR", 8)) {
    printf("Wrong signature=%s. \n", signature);
    goto Abort;
  }

  bytes_read = read(SCL, &blocks, 1);
  if (bytes_read < 1) {
    printf("Error - cannot read number of files in SCL file %s\n", oldname);
    goto Abort;
  }

  for (x = 0; x < blocks; x++) {
    bytes_read = read(SCL, &(headers[x][0]), 14);
    if (bytes_read < 14) {
      printf("Error - cannot read header %d from SCL file %s\n", x, oldname);
      goto Abort;
    }
  }

  for (x = 0; x < blocks; x++) {
    size = headers[x][13];
    if (lsb2ui(tmp) < size) {
      printf("file is too long to fit in the image *trd_free=%u < size=%u\n",
              lsb2ui(tmp), size);
      close(SCL);
      goto Finish;
    }

    if (*trd_files > 127) {
      printf("image is full\n");
      close(SCL);
      goto Finish;
    }

    memcpy((void *) ((char *) TRDh + *trd_files * 16),
	   (void *) headers[x], 14);

    memcpy((void *) ((char *) TRDh + *trd_files * 16 + 0x0E),
	   (void *) trd_fsec, 2);

    tmpscl = malloc(32000);

    left = (unsigned long) ((unsigned char) headers[x][13]) * 256L;
    fptr = (*trd_ftrk) * 4096L + (*trd_fsec) * 256L;
    lseek(TRD, fptr, SEEK_SET);

    while (left > 32000) {
      bytes_read = read(SCL, tmpscl, 32000);
      if (bytes_read <= 0) {
        printf("Error - reading file %d from SCL %s\n", x, oldname);
        goto Abort;
      }

      bytes_written = write(TRD, tmpscl, bytes_read);
      if (bytes_written < bytes_read) {
        printf("Error - writing to TRD file %s\n", newname);
        goto Abort;
      }

      left -= bytes_read;
    }

    if (left > 0) {
      bytes_read = read(SCL, tmpscl, left);
      if (bytes_read < (ssize_t)left) {
        printf("Error - reading file %d from SCL %s\n", x, oldname);
        goto Abort;
      }

      bytes_written = write(TRD, tmpscl, bytes_read);
      if (bytes_written < bytes_read) {
        printf("Error - writing to TRD file %s\n", newname);
        goto Abort;
      }
    }

    free(tmpscl);

    (*trd_files)++;

    ui2lsb(tmp, lsb2ui(tmp) - size);

    while (size > 15) {
      (*trd_ftrk)++;
      size -= 16;
    }

    (*trd_fsec) += size;
    while ((*trd_fsec) > 15) {
      (*trd_fsec) -= 16;
      (*trd_ftrk)++;
    }
  }

  close(SCL);

Finish:
  lseek(TRD, 0L, SEEK_SET);
  bytes_written = write(TRD, TRDh, 4096);
  if (bytes_written < 4096) {
    printf("Error - writing header to TRD file %s\n", newname);
  }
  close(TRD);
  free(TRDh);
  return;

Abort:
  close(SCL);
  close(TRD);
  free(TRDh);
  free(tmpscl);
}

static int
parse_options(int argc, char **argv, struct options * options)
{
  /* Defined by getopt */
  extern int optind;

  int c;

  int unknown = 0;

  while ((c = getopt(argc, argv, "")) != EOF)
    switch (c) {
    case '?':
      unknown = c;
      break;
    }

  if( unknown ) return 1;

  if (argv[optind] == NULL) {
    fprintf(stderr, "%s: no SCL file given\n", progname);
    return 1;
  }
  options->sclfile = argv[optind];
  if (argv[optind+1] == NULL) {
    fprintf(stderr, "%s: no TRD file given\n", progname);
    return 1;
  }
  options->trdfile = argv[optind+1];

  return 0;
}

static void
init_options( struct options *options )
{
  options->sclfile = NULL;
  options->trdfile = NULL;
}

int
main(int argc, char **argv)
{
  struct options options;

  progname = argv[0];

  init_options(&options);
  if (parse_options(argc, argv, &options))
    return 1;

  Scl2Trd(options.sclfile, options.trdfile);

  return 0;
}
