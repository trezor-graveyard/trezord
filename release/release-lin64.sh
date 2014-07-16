#!/bin/sh
IMAGETAG=trezord-build-env
docker build -t $IMAGETAG .
docker run -t -v $(pwd)/..:/trezord-src $IMAGETAG /trezord-src/build.sh lin64
