#
# CMake build file for hidapi
#

cmake_minimum_required(VERSION 2.8)

project(hidapi)
include_directories(hidapi)

if(APPLE)
  add_subdirectory(mac)
elseif(WIN32)
  add_subdirectory(windows)
elseif(UNIX)
  add_subdirectory(libusb)
endif(APPLE)
