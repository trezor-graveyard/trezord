#!/bin/sh

set -e

cd $(dirname $0)

TARGET=$1
BUILDDIR=build${TARGET:+-$TARGET}
VERSION=$(cat ../../VERSION)

INSTALLER=trezor-bridge-$VERSION-$TARGET-install.exe

cd ../../$BUILDDIR

cp ../release/windows/trezord.nsis trezord.nsis
if [ $TARGET = win32 ]; then
  makensis -X"OutFile $INSTALLER" -X'InstallDir "$PROGRAMFILES32\TREZOR Bridge"' trezord.nsis
else
  makensis -X"OutFile $INSTALLER" -X'InstallDir "$PROGRAMFILES64\TREZOR Bridge"' trezord.nsis
fi
mv $INSTALLER $INSTALLER.unsigned
osslsigncode sign -certs ../release/windows/authenticode.spc -key ../release/windows/authenticode.key -n "TREZOR Bridge" -i "https://mytrezor.com" -t http://timestamp.verisign.com/scripts/timstamp.dll -in $INSTALLER.unsigned -out $INSTALLER
osslsigncode verify -in $INSTALLER
