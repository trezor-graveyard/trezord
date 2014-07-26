# initialize from the image

FROM fedora:20

# update package repositories

RUN yum install -y cmake gcc gcc-c++ git make patchutils pkgconfig wget

# install dependencies for Linux build

RUN yum install -y glibc-devel glibc-static libgcc libstdc++-static zlib
RUN yum install -y boost-static libusbx-devel protobuf-static

RUN yum install -y glibc-devel.i686 glibc-static.i686 libgcc.i686 libstdc++-static.i686 zlib.i686
RUN yum install -y boost-static.i686 libusbx-devel.i686 protobuf-static.i686

# protobuf-static does not install protobuf-devel, workaround
RUN yum install -y protobuf-devel
RUN rm -f $( rpm -ql protobuf-devel | grep \.so$ )

# install dependencies for Windows build

RUN yum install -y mingw32-boost-static mingw64-boost-static osslsigncode
ADD windows/mingw32-protobuf-devel-2.5.0.tar.xz /
ADD windows/mingw64-protobuf-devel-2.5.0.tar.xz /
RUN yum install -y mingw32-nsis
