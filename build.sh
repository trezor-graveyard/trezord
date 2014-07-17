#!/bin/sh

set -e

cd $(dirname $0)

TARGET=$1
BUILDDIR=build${TARGET:+-$TARGET}

case "$TARGET" in
  lin32 )
    CMAKE_FLAGS="-DCMAKE_C_FLAGS=-m32 -DCMAKE_CXX_FLAGS=-m32"
    ;;
  win32 )
    TARGET_ARCH=i686-w64-mingw32
    ;;
  win64 )
    TARGET_ARCH=x86_64-w64-mingw32
    ;;
  * ) # native build
    JOBS="-j 4"
    ;;
esac

case "$TARGET" in
  win?? )
    SYSROOT=/usr/$TARGET_ARCH/sys-root/mingw
    CMAKE_FLAGS="-DCMAKE_SYSTEM_NAME=Windows -DCMAKE_C_COMPILER=${TARGET_ARCH}-gcc -DCMAKE_CXX_COMPILER=${TARGET_ARCH}-g++ -DCMAKE_RC_COMPILER_INIT=${TARGET_ARCH}-windres -DCMAKE_SHARED_LIBRARY_LINK_C_FLAGS='' -DCMAKE_FIND_ROOT_PATH='${SYSROOT}'"
    ;;
esac

# Compile cpp-netlib
if [ \! -d $BUILDDIR/lib/cpp-netlib ]; then
  mkdir -p $BUILDDIR/lib/cpp-netlib && cd $BUILDDIR/lib/cpp-netlib
  cmake $CMAKE_FLAGS ../../../vendor/cpp-netlib
  make $JOBS
  cd ../../..
fi

# Compile jsoncpp
if [ \! -d $BUILDDIR/lib/jsoncpp ]; then
  mkdir -p $BUILDDIR/lib/jsoncpp && cd $BUILDDIR/lib/jsoncpp
  cmake $CMAKE_FLAGS ../../../vendor/jsoncpp
  make $JOBS
  cd ../../..
fi

mkdir -p $BUILDDIR && cd $BUILDDIR
cmake $CMAKE_FLAGS ..
make $JOBS
