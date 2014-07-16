# initialize from the image

FROM centos:centos7

# update package repositories

RUN rpm -Uvh http://download.fedoraproject.org/pub/epel/beta/7/x86_64/epel-release-7-0.2.noarch.rpm
RUN yum install -y cmake gcc gcc-c++ git make pkgconfig wget

# install dependencies for Linux build

RUN yum install -y boost-static libusbx-devel protobuf-devel protobuf-static
RUN yum install -y boost-static.i686 libusbx-devel.i686
RUN yum install -y glibc-devel.i686 libgcc.i686 libstdc++.i686 zlib.i686

# install 32bit protobuf packages from Fedora Koji until 32bit build of CentOS is ready

RUN wget https://kojipkgs.fedoraproject.org/packages/protobuf/2.5.0/6.fc21/i686/protobuf-2.5.0-6.fc21.i686.rpm https://kojipkgs.fedoraproject.org/packages/protobuf/2.5.0/6.fc21/i686/protobuf-devel-2.5.0-6.fc21.i686.rpm https://kojipkgs.fedoraproject.org/packages/protobuf/2.5.0/6.fc21/i686/protobuf-static-2.5.0-6.fc21.i686.rpm https://kojipkgs.fedoraproject.org/packages/protobuf/2.5.0/6.fc21/i686/protobuf-compiler-2.5.0-6.fc21.i686.rpm
RUN rpm -ivh protobuf-2.5.0-6.fc21.i686.rpm protobuf-devel-2.5.0-6.fc21.i686.rpm protobuf-static-2.5.0-6.fc21.i686.rpm protobuf-compiler-2.5.0-6.fc21.i686.rpm
