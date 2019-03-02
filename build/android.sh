#!/bin/bash

set -e
set -o nounset

ANDROID=$HOME/android
NDK=$ANDROID/android-ndk
ABI=arm
PLATFORM=16
OUT_DIR=$(dirname $0)

while getopts ":hn:a:p:o:" optname
do
  case "$optname" in
    "h")
      echo "Usage:
      android.sh -n NDK -p PLATFORM -a ABI [-o OUTDIR]
      "
      exit 1
      ;;
    "a")
      ABI="$OPTARG"
      ;;
    "n")
      NDK="$OPTARG"
      ;;
    "p")
      PLATFORM="$OPTARG"
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

case $ABI in
  "arm")
    CFLAGS="-target armv7-none-linux-androideabi -march=armv7-a -mfloat-abi=softfp -mfpu=vfp -fno-builtin-memmove -marm"
    LDFLAGS="-L${NDK}/sources/cxx-stl/llvm-libc++/libs/armeabi-v7a"
    HOST="arm-linux-androideabi"
    ;;
  "arm64")
    PLATFORM=21
    CFLAGS="-target aarch64-none-linux-android "
    LDFLAGS="-L${NDK}/sources/cxx-stl/llvm-libc++/libs/arm64-v8a"
    HOST="aarch64-linux-android"
    ;;
  "x86")
    CFLAGS="-target i686-none-linux-android "
    LDFLAGS="-L${NDK}/sources/cxx-stl/llvm-libc++/libs/x86"
    HOST="i686-linux-android"
    ;;
  "x86_64")
    CFLAGS="-target x86_64-none-linux-android "
    LDFLAGS="-L${NDK}/sources/cxx-stl/llvm-libc++/libs/x86_64"
    HOST="x86_64-linux-android"
    ;;
  *)
    echo "Unknown/Unhandled ABI $ABI"
    exit 1
    ;;
esac

export TOOLCHAIN=$NDK/toolchains/llvm/prebuilt/$HOST_OS-$HOST_ARCH
export AR=$TOOLCHAIN/bin/${HOST}-ar
export AS=$TOOLCHAIN/bin/${HOST}-as
export CC=$TOOLCHAIN/bin/clang
export CXX=$TOOLCHAIN/bin/clang++
export LD=$TOOLCHAIN/bin/${HOST}-ld
export RANLIB=$TOOLCHAIN/bin/${HOST}-ranlib
export STRIP=$TOOLCHAIN/bin/${HOST}-strip

SYSROOT=$NDK/sysroot/usr
LDFLAGS="${LDFLAGS} -L$NDK/platforms/android-${PLATFORM}/arch-$ABI/usr/lib"

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

export PATH=$TOOLCHAIN/bin:$NDK:$PATH
export CFLAGS="${CFLAGS} -fstack-protector-strong -g -Ofast -DANDROID -D_REENTRANT -fPIC -D__ANDROID_API__=${PLATFORM} -I${SYSROOT}/include -I${INSTALL_PREFIX}/include"
export CPPFLAGS="${CFLAGS}"
export CXXFLAGS="${CFLAGS}"
export LDFLAGS="--sysroot=$NDK/platforms/android-${PLATFORM}/arch-$ABI -fuse-ld=lld ${LDFLAGS} -L${INSTALL_PREFIX}/lib -lc -lm -ldl"

pushd $BUILD_DIR/libspectrum
  if [ ! -f $SRC_DIR/libspectrum/configure ]; then
    pushd $SRC_DIR/libspectrum
    ./autogen.sh
    popd
  fi
  $SRC_DIR/libspectrum/configure --host $HOST --with-sysroot=$NDK/platforms/android-${PLATFORM}/arch-$ABI --prefix=$INSTALL_PREFIX \
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
  $SRC_DIR/fuse/configure --host $HOST --with-sysroot=$NDK/platforms/android-${PLATFORM}/arch-$ABI --prefix=$INSTALL_PREFIX \
                          --disable-shared --without-gtk --without-alsa --without-sdl --without-libxml2 \
                          --with-joystick --with-no-ui --without-png
  make $JOBS
  make install
popd
