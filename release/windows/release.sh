#!/bin/sh

set -e

cd $(dirname $0)

TARGET=$1
BUILDDIR=build${TARGET:+-$TARGET}
VERSION=$(cat ../../VERSION)

INSTALLER=trezor-bridge-$VERSION-$TARGET-install.exe

cp trezord.nsis ../../$BUILDDIR
cd ../../$BUILDDIR
makensis -X"OutFile $INSTALLER" trezord.nsis
mv $INSTALLER $INSTALLER.unsigned
osslsigncode sign -certs ../release/windows/authenticode.spc -key ../release/windows/authenticode.key -n "TREZOR Bridge" -i "https://mytrezor.com" -t http://timestamp.verisign.com/scripts/timstamp.dll -in $INSTALLER.unsigned -out $INSTALLER
osslsigncode verify -in $INSTALLER
