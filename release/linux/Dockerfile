# initialize from the image

FROM fedora:25

# update package repositories

RUN dnf update -y

# install tools

RUN dnf install -y cmake make wget
RUN dnf install -y gcc gcc-c++ git make patchutils pkgconfig wget

# install dependencies for Linux packaging

RUN dnf install -y ruby-devel rubygems rpm-build
RUN gem install fpm --no-document

# install dependencies for Linux build

RUN dnf install -y glibc-devel glibc-static libgcc libstdc++-static zlib-devel
RUN dnf install -y boost-static libusbx-devel protobuf-static

RUN dnf install -y glibc-devel.i686 glibc-static.i686 libgcc.i686 libstdc++-static.i686 zlib-devel.i686
RUN dnf install -y boost-static.i686 libusbx-devel.i686 protobuf-static.i686

# install used networking libraries

RUN dnf install -y libcurl-devel libmicrohttpd-devel
RUN dnf install -y libcurl.i686 libcurl-devel.i686 libmicrohttpd.i686 libmicrohttpd-devel.i686

# install package signing tools
RUN dnf install -y rpm-sign
RUN ln -s gpg2 /usr/bin/gpg
