#
# CMake build file for trezord
#

cmake_minimum_required(VERSION 2.8)

project(trezord)

option(BUILD_TESTS "Build tests?" off)

set(BUILD_SHARED_LIBS off)
if (WIN32)
  set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
  set(CMAKE_EXE_LINKER_FLAGS "-static -static-libgcc -static-libstdc++")
endif(WIN32)

include_directories(src)

set (SRCS
  src/main.cpp
  src/api.hpp
  src/core.hpp
  src/wire.hpp
  src/utils.hpp
  src/protobuf/state.hpp
  src/protobuf/json_codec.hpp
  src/protobuf/wire_codec.hpp
  src/config/config.pb.cc
  src/config/config.pb.h)

if (WIN32)
  set (SRCS src/trezord.rc ${SRCS})
endif(WIN32)

add_executable(trezord ${SRCS})

set(CMAKE_CXX_FLAGS "-std=c++11 -fpermissive ${CMAKE_CXX_FLAGS}")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
  "${CMAKE_SOURCE_DIR}/cmake/modules"
  "${CMAKE_BINARY_DIR}/lib/cpp-netlib")

set(Boost_USE_STATIC_LIBS on)
find_package(Boost 1.53.0 REQUIRED regex thread system unit_test_framework)
find_package(Protobuf 2.5.0 REQUIRED)
find_package(cppnetlib REQUIRED)
find_package(jsoncpp REQUIRED)

add_subdirectory(vendor/hidapi)

add_subdirectory(vendor/trezor-crypto)

include_directories(
  ${Boost_INCLUDE_DIRS}
  ${PROTOBUF_INCLUDE_DIRS}
  ${CPPNETLIB_INCLUDE_DIRS}
  ${JSONCPP_INCLUDE_DIRS}
  vendor/hidapi/hidapi
  vendor/trezor-crypto
  vendor/easyloggingpp)

if (WIN32)
  set(OS_LIBRARIES wsock32 ws2_32)
else(WIN32)
  set(OS_LIBRARIES pthread)
endif(WIN32)

target_link_libraries(trezord
  ${Boost_LIBRARIES}
  ${PROTOBUF_LIBRARIES}
  ${CPPNETLIB_LIBRARIES}
  ${JSONCPP_LIBRARIES}
  ${OS_LIBRARIES}
  hidapi
  TrezorCrypto)

set_target_properties(trezord PROPERTIES LINK_SEARCH_START_STATIC 1)
set_target_properties(trezord PROPERTIES LINK_SEARCH_END_STATIC 1)

if(BUILD_TESTS)

  include_directories(test)

  add_executable(test-protobuf_codecs test/protobuf_codecs.cpp)

  target_link_libraries(test-protobuf_codecs
    ${Boost_LIBRARIES}
    ${PROTOBUF_LIBRARIES}
    ${JSONCPP_LIBRARIES})

  enable_testing()
  add_test(ProtobufCodecs test-protobuf_codecs)

endif(BUILD_TESTS)
