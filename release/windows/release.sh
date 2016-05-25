#!/bin/sh

set -e

cd $(dirname $0)

TARGET=$1
BUILDDIR=build${TARGET:+-$TARGET}
VERSION=$(cat ../../VERSION)

INSTALLER=trezor-bridge-$VERSION-$TARGET-install.exe

cd ../../$BUILDDIR

cp ../release/windows/trezord.nsis trezord.nsis
for i in \
	iconv.dll \
	libcrypto-10.dll \
	libcurl-4.dll \
	libffi-6.dll \
	libgcc_s_sjlj-1.dll \
	libgcrypt-20.dll \
	libgmp-10.dll \
	libgnutls-30.dll \
	libgpg-error-0.dll \
	libhogweed-4-1.dll \
	libidn-11.dll \
	libintl-8.dll \
	libmicrohttpd-10.dll \
	libnettle-6-1.dll \
	libp11-kit-0.dll \
	libssh2-1.dll \
	libssl-10.dll \
	libstdc++-6.dll \
	libtasn1-6.dll \
	libwinpthread-1.dll \
	zlib1.dll \
; do
  if [ $TARGET = win32 ]; then
    cp /usr/i686-w64-mingw32/sys-root/mingw/bin/$i .
  else
    cp /usr/x86_64-w64-mingw32/sys-root/mingw/bin/$i .
  fi
done

mingw-strip *.dll *.exe

if [ $TARGET = win32 ]; then
  makensis -X"OutFile $INSTALLER" -X'InstallDir "$PROGRAMFILES32\TREZOR Bridge"' trezord.nsis
else
  makensis -X"OutFile $INSTALLER" -X'InstallDir "$PROGRAMFILES64\TREZOR Bridge"' trezord.nsis
fi
mv $INSTALLER $INSTALLER.unsigned
osslsigncode sign -certs ../release/windows/authenticode.spc -key ../release/windows/authenticode.key -n "TREZOR Bridge" -i "https://mytrezor.com" -t http://timestamp.verisign.com/scripts/timstamp.dll -in $INSTALLER.unsigned -out $INSTALLER
osslsigncode verify -in $INSTALLER
