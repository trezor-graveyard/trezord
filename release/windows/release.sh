#!/bin/sh

set -e

cd $(dirname $0)

TARGET=$1
BUILDDIR=build${TARGET:+-$TARGET}

cp trezord.nsis ../../$BUILDDIR
cd ../../$BUILDDIR
makensis -X"OutFile trezor-bridge-$TARGET-install.exe" trezord.nsis
