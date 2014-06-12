#
# CMake build file for trezord
#

cmake_minimum_required(VERSION 2.8)

# trezord project
project(trezord)
include_directories(src)
add_executable(trezord
  src/main.cpp)

# Boost dependency
find_package(Boost 1.54.0 REQUIRED thread system)
include_directories(${Boost_INCLUDE_DIR})

# cpp-netlib dependency
set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${CPPNETLIB_ROOT})
find_package(cppnetlib 0.11.0 REQUIRED)
include_directories(${CPPNETLIB_INCLUDE_DIRS})

# linking
target_link_libraries(trezord
  ${Boost_THREAD_LIBRARY}
  ${Boost_SYSTEM_LIBRARY}
  ${CPPNETLIB_LIBRARIES})
