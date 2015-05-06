# initialize from base image

FROM ubuntu:latest

# install dependencies

RUN apt-get update && apt-get install -y \
    git wget cmake \
    gcc g++ zlib1g-dev libmpc-dev libmpfr-dev libgmp-dev

# build and install osxcross and osx sdk

RUN git clone https://github.com/jpochyla/osxcross.git /opt/osxcross
COPY ./MacOSX10.8.sdk.tar.xz /opt/osxcross/tarballs/

WORKDIR /opt/osxcross

ENV MACOSX_DEPLOYMENT_TARGET 10.8
ENV PATH /opt/osxcross/target/bin:$PATH

RUN ./tools/get_dependencies.sh
RUN echo | ./build.sh
RUN echo | GCC_VERSION=4.9.1 ./build_gcc.sh

# install trezord dependencies from macports

RUN osxcross-macports install \
    gdbm-1.11 zlib-1.2.8 xz-5.2.1 libgpg-error-1.19 \
    protobuf-cpp-2.5.0 curl boost libmicrohttpd

# make cmake and dylibbundler happy

RUN mkdir /Applications
RUN ln -s x86_64-apple-darwin12-otool /opt/osxcross/target/bin/otool
RUN ln -s x86_64-apple-darwin12-install_name_tool /opt/osxcross/target/bin/install_name_tool
RUN ln -s /opt/osxcross/target/macports/pkgs/opt/local /opt/local
