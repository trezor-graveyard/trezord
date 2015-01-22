# initialize from the image

FROM fedora:21

# update package repositories

RUN yum install -y deltarpm yum-plugin-fastestmirror
RUN yum update -y

# install tools

RUN yum install -y cmake make wget
RUN yum install -y osslsigncode mingw32-nsis

# install dependencies for Windows build

RUN yum install -y mingw32-boost-static
RUN yum install -y mingw32-curl
# RUN yum install -y mingw32-libmicrohttpd # version 0.9.34 contains bug, use 0.9.39 instead
RUN yum install -y mingw32-winpthreads
RUN yum install -y mingw32-zlib-static

# install dependencies from COPR

RUN wget https://copr-be.cloud.fedoraproject.org/results/prusnak/private/fedora-21-x86_64/mingw-protobuf-2.5.0-9/mingw32-protobuf{,-static}-2.5.0-9.fc21.noarch.rpm
RUN wget https://copr-be.cloud.fedoraproject.org/results/prusnak/private/fedora-21-x86_64/mingw-libmicrohttpd-0.9.39-1/mingw32-libmicrohttpd-0.9.39-1.fc21.noarch.rpm

RUN yum install -y mingw32-protobuf{,-static}-2.5.0-9.fc21.noarch.rpm
RUN yum install -y mingw32-libmicrohttpd-0.9.39-1.fc21.noarch.rpm
