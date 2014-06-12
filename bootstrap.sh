#!/bin/sh

set -e

# Install cpp-netlib
mkdir -p vendor && cd vendor
git clone \
    --single-branch \
    --branch cpp-netlib-0.11.0-final \
    https://github.com/cpp-netlib/cpp-netlib.git
cd ..

# Compile cpp-netlib
mkdir -p lib/cpp-netlib && cd lib/cpp-netlib
cmake ../../vendor/cpp-netlib
make
cd ../..
