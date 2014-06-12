#!/bin/sh

set -e

mkdir -p build && cd build
cmake -D CPPNETLIB_ROOT='lib/cpp-netlib' ..
make
