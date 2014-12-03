# initialize from the image

FROM fedora:20

# update package repositories

RUN yum update -y

# install development tools

RUN yum install -y cmake gcc gcc-c++ git make patchutils pkgconfig wget

# install dependencies for Linux build

RUN yum install -y glibc-devel glibc-static libgcc libstdc++-static zlib
RUN yum install -y boost-static libusbx-devel openssl-static protobuf-static

RUN yum install -y glibc-devel.i686 glibc-static.i686 libgcc.i686 libstdc++-static.i686 zlib.i686
RUN yum install -y boost-static.i686 libusbx-devel.i686 openssl-static.i686 protobuf-static.i686

# protobuf-static does not install protobuf-devel, workaround

RUN yum install -y protobuf-devel

# remove .so files so static libs are picked instead

RUN rm -f $( rpm -ql boost-devel | grep \.so$ )
RUN rm -f $( rpm -ql openssl-devel | grep \.so$ )
RUN rm -f $( rpm -ql protobuf-devel | grep \.so$ )

# install dependencies for Linux packaging

RUN yum install -y ruby-devel rubygems rpm-build
RUN gem install fpm --no-document

# install dependencies for Windows build

RUN yum install -y mingw32-boost-static mingw32-openssl-static mingw32-zlib-static
RUN yum install -y mingw64-boost-static mingw64-openssl-static mingw64-zlib-static

COPY windows/mingw32-protobuf-2.5.0-9.fc20.noarch.rpm /
COPY windows/mingw32-protobuf-static-2.5.0-9.fc20.noarch.rpm /
COPY windows/mingw64-protobuf-2.5.0-9.fc20.noarch.rpm /
COPY windows/mingw64-protobuf-static-2.5.0-9.fc20.noarch.rpm /

RUN yum install -y /mingw32-protobuf*.rpm /mingw64-protobuf*.rpm

# install dependencies for Windows packaging

RUN yum install -y osslsigncode mingw32-nsis
