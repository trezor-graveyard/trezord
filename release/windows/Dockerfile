# initialize from the image

FROM fedora:23

# update package repositories

RUN dnf update -y

# install tools

RUN dnf install -y cmake make wget
RUN dnf install -y osslsigncode mingw32-nsis

# install dependencies for Windows build

RUN dnf install -y mingw32-boost-static
RUN dnf install -y mingw32-curl
RUN dnf install -y mingw32-libmicrohttpd
RUN dnf install -y mingw32-winpthreads
RUN dnf install -y mingw32-zlib-static

# install dependencies from COPR

RUN wget https://copr-be.cloud.fedoraproject.org/results/prusnak/private/fedora-23-x86_64/00157154-mingw-protobuf/mingw32-protobuf{,-static}-2.6.1-3.fc23.noarch.rpm
RUN dnf install -y mingw32-protobuf{,-static}-2.6.1-3.fc23.noarch.rpm
