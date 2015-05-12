#!/bin/sh

set -e
set -x

cd $(dirname $0)

TARGET=$1
BUILDDIR=build${TARGET:+-$TARGET}
VERSION=$(cat ../../VERSION)

cd ../../$BUILDDIR

# bundle dependencies

make -C ../vendor/macdylibbundler

../vendor/macdylibbundler/dylibbundler \
    -od -b -x trezord -d libs/ \
    -p @executable_path/libs/

# strip binary and libs

x86_64-apple-darwin12-strip trezord

# fix libs permissions

chmod a+r libs/*
