#!/bin/sh

set -e

cd $(dirname $0)

TARGET=$1
BUILDDIR=build${TARGET:+-$TARGET}
VERSION=$(cat ../../VERSION)

cd ../../$BUILDDIR

install -D -m 0755 trezord                          ./usr/bin/trezord
install -D -m 0644 ../release/linux/trezor.rules    ./usr/lib/udev/rules.d/23-trezor.rules
install -D -m 0755 ../release/linux/trezord.init    ./etc/init.d/trezord
install -D -m 0644 ../release/linux/trezord.service ./usr/lib/systemd/system/trezord.service

NAME=trezor-bridge

rm -f *.deb *.rpm *.tar.bz2
tar cfj $NAME-$VERSION.tar.bz2 ./etc ./usr

for TYPE in "deb" "rpm"; do
	case "$TARGET-$TYPE" in
		lin32-deb)
			ARCH=i386
			DEPS="-d libcurl3 -d libgcrypt20 -d libgnutls28 -d libmicrohttpd10 -d libusb-1.0-0"
			;;
		lin64-deb)
			ARCH=amd64
			DEPS="-d libcurl3 -d libgcrypt20 -d libgnutls28 -d libmicrohttpd10 -d libusb-1.0-0"
			;;
		lin32-rpm)
			ARCH=i386
			DEPS="-d libcurl.so.4 -d libgcrypt.so.20 -d libgnutls.so.28 -d libmicrohttpd.so.10 -d libusb-1.0.so.0"
			;;
		lin64-rpm)
			ARCH=x86_64
			DEPS="-d libcurl.so.4()(64bit) -d libgcrypt.so.20()(64bit) -d libgnutls.so.28()(64bit) -d libmicrohttpd.so.10()(64bit) -d libusb-1.0.so.0()(64bit)"
			;;
	esac
	fpm \
		-s tar \
		-t $TYPE \
		-a $ARCH \
		-n $NAME \
		-v $VERSION \
		--license "LGPL-3.0" \
		--vendor "SatoshiLabs" \
		--maintainer "stick@satoshilabs.com" \
		--url "http://bitcointrezor.com/" \
		--category "Productivity/Security" \
		--before-install ../release/linux/fpm.before-install.sh \
		--after-install ../release/linux/fpm.after-install.sh \
		--before-remove ../release/linux/fpm.before-remove.sh \
		$DEPS \
		$NAME-$VERSION.tar.bz2
done

rm -rf ./etc ./usr
