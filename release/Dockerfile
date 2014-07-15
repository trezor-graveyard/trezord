# initialize from the image

FROM ubuntu:14.04

# update package repositories

RUN apt-get update

# install build tools

RUN apt-get install -y build-essential cmake git pkg-config

# install dependencies for Linux build

RUN apt-get install -y libboost-test1.54-dev libboost-system1.54-dev libboost-regex1.54-dev libboost-date-time1.54-dev libboost-thread1.54-dev libboost-filesystem1.54-dev libboost-program-options1.54-dev libboost-chrono1.54-dev libboost-atomic1.54-dev libprotobuf-dev libssl-dev libusb-1.0-0-dev

# install dependencies for Windows build

RUN apt-get install -y mingw-w64
