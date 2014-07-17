# initialize from the image

FROM centos:centos7

# update package repositories

RUN rpm -Uvh http://download.fedoraproject.org/pub/epel/beta/7/x86_64/epel-release-7-0.2.noarch.rpm
RUN yum install -y cmake gcc gcc-c++ git make pkgconfig wget

# install dependencies for Linux build

RUN yum install -y glibc-static libgcc libstdc++-static zlib
RUN yum install -y boost-static libusbx-devel protobuf-devel protobuf-static

RUN yum install -y glibc-static.i686 libgcc.i686 libstdc++-static.i686 zlib.i686
RUN yum install -y boost-static.i686 libusbx-devel.i686

# install 32bit protobuf package manually until 32bit build of CentOS EPEL is ready

ADD linux/protobuf-static-2.5.0-6.fc21.i686.tar.xz /

# install dependencies for Windows build

RUN yum install -y mingw32-boost-static mingw64-boost-static
ADD windows/mingw32-protobuf-devel-2.5.0.tar.xz /
ADD windows/mingw64-protobuf-devel-2.5.0.tar.xz /
