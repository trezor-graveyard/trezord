#!/bin/sh

set -e

cd $(dirname $0)

# Compile cpp-netlib
if [ \! -d lib/cpp-netlib ]; then
  mkdir -p lib/cpp-netlib && cd lib/cpp-netlib
  cmake ../../vendor/cpp-netlib
  make
  cd ../..
fi

# Compile jsoncpp
if [ \! -d lib/jsoncpp ]; then
  mkdir -p lib/jsoncpp && cd lib/jsoncpp
  cmake ../../vendor/jsoncpp
  make
  cd ../..
fi

mkdir -p build && cd build
cmake ..
make
