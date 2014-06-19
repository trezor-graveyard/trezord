# Copyright (c) 2013, EMC Corporation (Isilon Division)
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# -- Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#
# -- Redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
# ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

CXX=		g++
PROTOC=		LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/usr/local/lib protoc
CXXFLAGS=	-I/usr/local/include
LDFLAGS=	-L/usr/local/lib -lprotobuf -ljson

all:
	${CXX} ${CXXFLAGS} -fPIC -shared json_protobuf.cpp -o libjson_protobuf.so

test: all
	${PROTOC} --cpp_out=. check_protobuf_types.proto
	${CXX} ${CXXFLAGS} check_protobuf_types.pb.cc check_json_protobuf.cpp \
		-o check_json_protobuf ${LDFLAGS} -L. -ljson_protobuf
	LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/usr/local/lib:. ./check_json_protobuf

clean:
	rm -rf *.so check_json_protobuf *.pb.*