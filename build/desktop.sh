#!/bin/bash

set -e
set -o nounset

export CFLAGS="-g"
export CPPFLAGS="${CFLAGS}"
export CXXFLAGS="${CFLAGS}"

OUT_DIR=$(dirname $0)

while getopts ":ho:" optname
do
  case "$optname" in
    "h")
      echo "Usage:
      desktop.sh [-o OUTDIR]
      "
      exit 1
      ;;
    "o")
      OUT_DIR="$OPTARG"
      ;;
    "?")
      echo "Unknown option $OPTARG"
      exit 1
      ;;
    ":")
      echo "Option -$OPTARG requires an argument." >&2
      exit 1
      ;;
  esac
done

OUT_DIR=$(cd $OUT_DIR && pwd)
SRC_DIR=$(dirname $0)
SRC_DIR=$(cd $SRC_DIR/.. && pwd)
INSTALL_PREFIX=$OUT_DIR/desktop
BUILD_DIR=$OUT_DIR/build/desktop

rm -fr $INSTALL_PREFIX
mkdir -p $INSTALL_PREFIX
rm -fr $BUILD_DIR
mkdir -p $BUILD_DIR/fuse
mkdir -p $BUILD_DIR/libspectrum

JOBS=${JOBS:="-j10"}

pushd $BUILD_DIR/libspectrum
  if [ ! -f $SRC_DIR/libspectrum/configure ]; then
    pushd $SRC_DIR/libspectrum
    ./autogen.sh
    popd
  fi
  $SRC_DIR/libspectrum/configure --prefix=$INSTALL_PREFIX --disable-shared --with-fake-glib \
                                 --without-libaudiofile --without-libgcrypt
  make $JOBS
  make install
popd

pushd $BUILD_DIR/fuse
  if [ ! -f $SRC_DIR/fuse/configure ]; then
    pushd $SRC_DIR/fuse
    ./autogen.sh
    popd
  fi
  $SRC_DIR/fuse/configure --prefix=$INSTALL_PREFIX --disable-shared --without-alsa --without-libxml2 \
                          --with-joystick --with-no-ui --without-png
  make $JOBS
  make install
popd
