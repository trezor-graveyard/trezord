#!/bin/sh

set -e

cd $(dirname $0)

VERSION=$(cat VERSION)
DISTNAME=trezord-$VERSION
TARBALL=$DISTNAME.tar.bz2

echo -n "Creating $TARBALL tarball..."

git submodule update --init

tar -c --exclude='*.tar.bz2' --exclude='.git*' -s "|^|$DISTNAME/|" -j -f $TARBALL *

echo " Done!"
