#
# CMake build file for trezord
#

cmake_minimum_required(VERSION 2.8)

project(trezord)

option(BUILD_TESTS "Build tests?" off)

include_directories(src)

add_executable(trezord
  src/main.cpp
  src/api.hpp
  src/core.hpp
  src/wire.hpp
  src/protobuf/state.hpp
  src/protobuf/json_codec.hpp
  src/protobuf/wire_codec.hpp)

set(CMAKE_CXX_FLAGS "-std=c++11 ${CMAKE_CXX_FLAGS}")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
  "${CMAKE_SOURCE_DIR}/cmake/modules"
  "${CMAKE_SOURCE_DIR}/lib/cpp-netlib")

find_package(Boost 1.54.0 REQUIRED thread system unit_test_framework)
find_package(Protobuf 2.5.0 REQUIRED)
find_package(cppnetlib REQUIRED)
find_package(jsoncpp REQUIRED)

add_subdirectory(vendor/hidapi)

include_directories(
  ${Boost_INCLUDE_DIRS}
  ${PROTOBUF_INCLUDE_DIRS}
  ${CPPNETLIB_INCLUDE_DIRS}
  ${JSONCPP_INCLUDE_DIRS}
  vendor/hidapi/hidapi)

target_link_libraries(trezord
  ${Boost_LIBRARIES}
  ${PROTOBUF_LIBRARIES}
  ${CPPNETLIB_LIBRARIES}
  ${JSONCPP_LIBRARIES}
  hidapi)

if(BUILD_TESTS)

  include_directories(test)

  add_executable(test-protobuf_codecs test/protobuf_codecs.cpp)

  target_link_libraries(test-protobuf_codecs
    ${Boost_LIBRARIES}
    ${PROTOBUF_LIBRARIES}
    ${JSONCPP_LIBRARIES}
    hidapi)

  enable_testing()
  add_test(ProtobufCodecs test-protobuf_codecs)

endif(BUILD_TESTS)
