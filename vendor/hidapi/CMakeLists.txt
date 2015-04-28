#
# CMake build file for hidapi
#

cmake_minimum_required(VERSION 2.8)

project(hidapi)
include_directories(hidapi)

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  add_subdirectory(mac)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  add_subdirectory(windows)
else()
  add_subdirectory(libusb)
endif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
