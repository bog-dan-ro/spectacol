#!/bin/bash

set -e
set -o nounset

ANDROID=$HOME/android
NDK=$ANDROID/android-ndk
ABI=armeabi-v7a
export API=21
OUT_DIR=$(dirname $0)

while getopts ":hn:a:p:o:" optname
do
  case "$optname" in
    "h")
      echo "Usage:
      android.sh -n NDK -a ABI [-o OUTDIR]
      "
      exit 1
      ;;
    "a")
      ABI="$OPTARG"
      ;;
    "n")
      NDK="$OPTARG"
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

HOST_ARCH=`uname -m`
case "$HOST_ARCH" in
    i?86) HOST_ARCH=x86
    ;;
    amd64) HOST_ARCH=x86_64
    ;;
    powerpc) HOST_ARCH=ppc
    ;;
esac

HOST_OS=`uname -s`
case "$HOST_OS" in
    Darwin)
        HOST_OS=darwin
        ;;
    Linux)
        # note that building  32-bit binaries on x86_64 is handled later
        HOST_OS=linux
        ;;
esac

export TOOLCHAIN=$NDK/toolchains/llvm/prebuilt/$HOST_OS-$HOST_ARCH
SYSROOT=$TOOLCHAIN/sysroot/usr

case $ABI in
  "armeabi-v7a")
    export TARGET=armv7a-linux-androideabi
    CFLAGS=" -march=armv7-a -mfloat-abi=softfp -mfpu=vfp -fno-builtin-memmove -marm "
    ;;
  "arm64-v8a")
    export TARGET=aarch64-linux-android
    CFLAGS=""
    ;;
  "x86")
    export TARGET=i686-linux-android
    CFLAGS=""
    ;;
  "x86_64")
    export TARGET=x86_64-linux-android
    CFLAGS=""
    ;;
  *)
    echo "Unknown/Unhandled ABI $ABI"
    exit 1
    ;;
esac

export AR=$TOOLCHAIN/bin/llvm-ar
export CC=$TOOLCHAIN/bin/$TARGET$API-clang
export AS=$CC
export CXX=$TOOLCHAIN/bin/$TARGET$API-clang++
export LD=$TOOLCHAIN/bin/ld
export RANLIB=$TOOLCHAIN/bin/llvm-ranlib
export STRIP=$TOOLCHAIN/bin/llvm-strip

OUT_DIR=$(cd $OUT_DIR && pwd)
SRC_DIR=$(dirname $0)
SRC_DIR=$(cd $SRC_DIR/.. && pwd)
INSTALL_PREFIX=$OUT_DIR/$ABI
BUILD_DIR=$OUT_DIR/build/$ABI

rm -fr $INSTALL_PREFIX
mkdir -p $INSTALL_PREFIX
rm -fr $BUILD_DIR
mkdir -p $BUILD_DIR/fuse
mkdir -p $BUILD_DIR/libspectrum

JOBS="-j$(nproc)"

export PATH=$TOOLCHAIN/bin:$PATH
export CFLAGS="${CFLAGS} -fstack-protector-strong -g -Ofast -DANDROID -D_REENTRANT -fPIC -I${INSTALL_PREFIX}/include"
export CPPFLAGS="${CFLAGS}"
export CXXFLAGS="${CFLAGS}"

pushd $BUILD_DIR/libspectrum
  if [ ! -f $SRC_DIR/libspectrum/configure ]; then
    pushd $SRC_DIR/libspectrum
    ./autogen.sh
    popd
  fi
  $SRC_DIR/libspectrum/configure --host $TARGET --with-sysroot=$SYSROOT --prefix=$INSTALL_PREFIX \
                                 --disable-shared --with-fake-glib --without-libaudiofile
  make $JOBS
  make install
popd

export PKG_CONFIG_PATH=$INSTALL_PREFIX/lib/pkgconfig
pushd $BUILD_DIR/fuse
  if [ ! -f $SRC_DIR/fuse/configure ]; then
    pushd $SRC_DIR/fuse
    ./autogen.sh
    popd
  fi
  $SRC_DIR/fuse/configure --host $TARGET --with-sysroot=$SYSROOT --prefix=$INSTALL_PREFIX \
                          --disable-shared --without-gtk --without-alsa --without-sdl --without-libxml2 \
                          --with-joystick --with-no-ui --without-png
  make $JOBS
  make install
popd
