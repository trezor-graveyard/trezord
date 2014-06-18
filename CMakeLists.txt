#
# CMake build file for trezord
#

cmake_minimum_required(VERSION 2.8)

project(trezord)
include_directories(src)
add_executable(trezord src/main.cpp)

# use C++11
set(CMAKE_CXX_FLAGS "-std=c++11")

# setup boost
find_package(Boost 1.54.0 REQUIRED thread system)
include_directories(${Boost_INCLUDE_DIR})

# setup cpp-netlib
set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} lib/cpp-netlib)
find_package(cppnetlib 0.11.0 REQUIRED)
include_directories(${CPPNETLIB_INCLUDE_DIRS})

# include hidapi
add_subdirectory(vendor/hidapi)
include_directories(vendor/hidapi/hidapi)

# linking
target_link_libraries(trezord
  ${Boost_THREAD_LIBRARY}
  ${Boost_SYSTEM_LIBRARY}
  ${CPPNETLIB_LIBRARIES}
  hidapi)
