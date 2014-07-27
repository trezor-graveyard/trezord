#!/bin/sh

set -e

cd $(dirname $0)

TARGET=$1
INSTALLER=trezor-bridge-$TARGET-install.exe
BUILDDIR=build${TARGET:+-$TARGET}

cp trezord.nsis ../../$BUILDDIR
cd ../../$BUILDDIR
makensis -X"OutFile $INSTALLER" trezord.nsis
osslsigncode -spc ../release/windows/authenticode.spc -key ../release/windows/authenticode.key -t http://timestamp.verisign.com/scripts/timstamp.dll -in $INSTALLER -out $INSTALLER.signed
mv $INSTALLER $INSTALLER.unsigned
mv $INSTALLER.signed $INSTALLER
