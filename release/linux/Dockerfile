# initialize from the image

FROM fedora:21

# update package repositories

RUN yum install -y deltarpm yum-plugin-fastestmirror
RUN yum update -y

# install tools

RUN yum install -y cmake make wget
RUN yum install -y gcc gcc-c++ git make patchutils pkgconfig wget

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

RUN yum install -y gnutls-devel libgcrypt-devel libcurl-devel libmicrohttpd-devel
RUN yum install -y gnutls-devel.i686 libgcrypt-devel.i686 libcurl-devel.i686 libmicrohttpd-devel.i686
