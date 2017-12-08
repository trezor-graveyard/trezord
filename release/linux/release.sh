#!/bin/sh

set -e

cd $(dirname $0)

GPGSIGNKEY=86E6792FC27BFD478860C11091F3B339B9A02A3D
TARGET=$1
BUILDDIR=build${TARGET:+-$TARGET}
VERSION=$(cat ../../VERSION)

cd ../../$BUILDDIR

install -D -m 0755 trezord                          ./usr/bin/trezord
install -D -m 0644 ../release/linux/trezor.rules    ./lib/udev/rules.d/51-trezor.rules
install -D -m 0755 ../release/linux/trezord.init    ./etc/init.d/trezord
install -D -m 0644 ../release/linux/trezord.service ./usr/lib/systemd/system/trezord.service

strip ./usr/bin/trezord

# prepare GPG signing environment
GPG_PRIVKEY=../release/linux/privkey.asc
if [ -r $GPG_PRIVKEY ]; then
    export GPG_TTY=$(tty)
    export LC_ALL=en_US.UTF-8
    gpg --import ../release/linux/privkey.asc
    GPG_SIGN=gpg
fi

NAME=trezor-bridge

rm -f *.deb *.rpm *.tar.bz2
tar -cjf $NAME-$VERSION.tar.bz2 --exclude=./lib/jsoncpp ./etc ./usr ./lib

for TYPE in "deb" "rpm"; do
	case "$TARGET-$TYPE" in
		lin32-deb)
			ARCH=i386
			DEPS="-d libcurl4 -d libmicrohttpd12 -d libusb-1.0-0"
			;;
		lin64-deb)
			ARCH=amd64
			DEPS="-d libcurl4 -d libmicrohttpd12 -d libusb-1.0-0"
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
		--url "https://trezor.io/" \
		--category "Productivity/Security" \
		--before-install ../release/linux/fpm.before-install.sh \
		--after-install ../release/linux/fpm.after-install.sh \
		--before-remove ../release/linux/fpm.before-remove.sh \
		$DEPS \
		$NAME-$VERSION.tar.bz2
	case "$TYPE-$GPG_SIGN" in
		deb-gpg)
			../release/linux/dpkg-sig -k $GPGSIGNKEY --sign builder trezor-bridge_${VERSION}_${ARCH}.deb
			;;
		rpm-gpg)
			rpm --addsign -D "%_gpg_name $GPGSIGNKEY" trezor-bridge-${VERSION}-1.${ARCH}.rpm
			;;
	esac
done


rm -rf ./etc ./usr ./lib
