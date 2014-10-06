#!/bin/sh

set -e

cd $(dirname $0)

TARGET=$1
BUILDDIR=build${TARGET:+-$TARGET}
BUILDTYPE=Release

case "$TARGET" in
  lin32 | lin64 | win32 | win64 ) # cross build
    PLATFORM_FILE="-C $(pwd)/cmake/Platform-$TARGET.cmake"
    ;;
  * ) # native build
    JOBS="-j 4"
    ;;
esac

# Compile cpp-netlib
if [ \! -d $BUILDDIR/lib/cpp-netlib ]; then
  mkdir -p $BUILDDIR/lib/cpp-netlib && cd $BUILDDIR/lib/cpp-netlib
  cmake -DCMAKE_BUILD_TYPE=$BUILDTYPE $PLATFORM_FILE ../../../vendor/cpp-netlib
  make $JOBS
  cd ../../..
fi

# Compile jsoncpp
if [ \! -d $BUILDDIR/lib/jsoncpp ]; then
  mkdir -p $BUILDDIR/lib/jsoncpp && cd $BUILDDIR/lib/jsoncpp
  cmake -DCMAKE_BUILD_TYPE=$BUILDTYPE $PLATFORM_FILE ../../../vendor/jsoncpp
  make $JOBS
  cd ../../..
fi

mkdir -p $BUILDDIR && cd $BUILDDIR
cmake -DCMAKE_BUILD_TYPE=$BUILDTYPE $PLATFORM_FILE ..
make $JOBS
