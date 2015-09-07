# initialize from the image

FROM fedora:22

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

RUN wget https://copr-be.cloud.fedoraproject.org/results/prusnak/private/fedora-22-x86_64/00113678-mingw-protobuf/mingw32-protobuf{,-static}-2.5.0-9.fc22.noarch.rpm
RUN dnf install -y mingw32-protobuf{,-static}-2.5.0-9.fc22.noarch.rpm
