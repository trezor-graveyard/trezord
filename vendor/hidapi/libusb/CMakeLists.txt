#
# CMake build file for libusb hidapi
#

add_library(hidapi hid.c)

find_package(PkgConfig)
pkg_check_modules(LIBUSB_1 REQUIRED libusb-1.0)

include_directories(${LIBUSB_1_INCLUDE_DIRS})

target_link_libraries(hidapi
  ${LIBUSB_1_LIBRARIES}
  rt)
