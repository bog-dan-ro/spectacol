#!/bin/bash

set -e
set -o nounset

ANDROID=$HOME/android
NDK=$ANDROID/android-ndk
TOOLCHAIN_VERSION=4.9
ABI=arm
PLATFORM=android-9
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
    SYSROOT=$NDK/platforms/$PLATFORM/arch-$ABI
    TOOLCHAIN=$NDK/toolchains/arm-linux-androideabi-$TOOLCHAIN_VERSION/prebuilt/$HOST_OS-$HOST_ARCH
    TOOLCHAIN_PREFIX=arm-linux-androideabi
    CFLAGS="-mandroid -mthumb -Wno-psabi -march=armv7-a -mfloat-abi=softfp -mfpu=vfp -ffunction-sections -funwind-tables -fstack-protector -fno-short-enums -DANDROID -Wa,--noexecstack -g -Os -fomit-frame-pointer -fno-strict-aliasing -finline-limit=64"
    LDFLAGS="-L${NDK}/sources/cxx-stl/gnu-libstdc++/${TOOLCHAIN_VERSION}/libs/armeabi-v7a"
    ;;
  "arm64")
    PLATFORM=android-21
    SYSROOT=$NDK/platforms/$PLATFORM/arch-$ABI
    TOOLCHAIN=$NDK/toolchains/aarch64-linux-android-$TOOLCHAIN_VERSION/prebuilt/$HOST_OS-$HOST_ARCH
    TOOLCHAIN_PREFIX=aarch64-linux-android
    CFLAGS="-mandroid -ffunction-sections -funwind-tables -fstack-protector -fomit-frame-pointer -fstrict-aliasing -funswitch-loops -finline-limit=300 -DANDROID -Wa,--noexecstac -g -Os -I${NDK}/sources/cxx-stl/gnu-libstdc++/${TOOLCHAIN_VERSION}/libs/arm64-v8a/include"
    LDFLAGS="-L${NDK}/sources/cxx-stl/gnu-libstdc++/${TOOLCHAIN_VERSION}/libs/arm64-v8a"
    ;;
  *)
    echo "Unknown/Unhandled ABI $ABI"
    exit 1
    ;;
esac

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

export CFLAGS="${CFLAGS} --sysroot=${SYSROOT} -I${INSTALL_PREFIX}/include"
export CPPFLAGS="${CFLAGS}"
export CXXFLAGS="${CFLAGS}"
export LDFLAGS="${LDFLAGS} -L${SYSROOT}/usr/lib -L${INSTALL_PREFIX}/lib -lm -ldl"

pushd $BUILD_DIR/libspectrum
  if [ ! -f $SRC_DIR/libspectrum/configure ]; then
    pushd $SRC_DIR/libspectrum
    ./autogen.sh
    popd
  fi
  $SRC_DIR/libspectrum/configure --host=$TOOLCHAIN_PREFIX --with-sysroot=$SYSROOT --prefix=$INSTALL_PREFIX \
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
  $SRC_DIR/fuse/configure --host=$TOOLCHAIN_PREFIX --with-sysroot=$SYSROOT --prefix=$INSTALL_PREFIX \
                          --disable-shared --without-gtk --without-alsa --without-sdl --without-libxml2 \
                          --with-joystick --with-no-ui --without-png
  make $JOBS
  make install
popd
