#!/bin/sh

set -e

# Compile cpp-netlib
mkdir -p lib/cpp-netlib && cd lib/cpp-netlib
cmake ../../vendor/cpp-netlib
make
cd ../..
