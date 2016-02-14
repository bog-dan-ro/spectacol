#!/bin/bash

set -e
set -o nounset

JOBS=${JOBS:="-j4"}

pushd ../libspectrum
  if [ ! -f configure ]; then
    ./autogen.sh
  fi
  ./configure --with-fake-glib --prefix=$PWD/../install --without-libgcrypt
  make $JOBS
  make install
popd

pushd ../fuse
  if [ ! -f configure ]; then
    ./autogen.sh
  fi
  ./configure --with-libspectrum-prefix=$PWD/../install --disable-shared \
              --without-gtk --without-alsa --without-sdl --with-joystick \
              --with-nullui --without-png
  make $JOBS
popd
