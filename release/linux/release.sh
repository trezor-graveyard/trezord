#!/bin/sh

set -e

cd $(dirname $0)

TARGET=$1
BUILDDIR=build${TARGET:+-$TARGET}
VERSION=$(cat ../../VERSION)

cd ../../$BUILDDIR

install -D -m 0755 trezord                          ./usr/bin/trezord
install -D -m 0644 ../release/linux/trezor.rules    ./lib/udev/rules.d/51-trezor.rules
install -D -m 0755 ../release/linux/trezord.init    ./etc/init.d/trezord
install -D -m 0644 ../release/linux/trezord.service ./usr/lib/systemd/system/trezord.service

strip ./usr/bin/trezord

NAME=trezor-bridge

rm -f *.deb *.rpm *.tar.bz2
tar cfj $NAME-$VERSION.tar.bz2 ./etc ./usr ./lib --exclude=./lib/jsoncpp

for TYPE in "deb" "rpm"; do
	case "$TARGET-$TYPE" in
		lin32-deb)
			ARCH=i386
			DEPS="-d libcurl3 -d libmicrohttpd10 -d libusb-1.0-0"
			;;
		lin64-deb)
			ARCH=amd64
			DEPS="-d libcurl3 -d libmicrohttpd10 -d libusb-1.0-0"
			;;
		lin32-rpm)
			ARCH=i386
			DEPS="--rpm-autoreq"
			;;
		lin64-rpm)
			ARCH=x86_64
			DEPS="--rpm-autoreq"
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
		--description "Communication daemon for TREZOR" \
		--maintainer "SatoshiLabs <stick@satoshilabs.com>" \
		--url "http://bitcointrezor.com/" \
		--category "Productivity/Security" \
		--before-install ../release/linux/fpm.before-install.sh \
		--after-install ../release/linux/fpm.after-install.sh \
		--before-remove ../release/linux/fpm.before-remove.sh \
		$DEPS \
		$NAME-$VERSION.tar.bz2
done

rm -rf ./etc ./usr ./lib
