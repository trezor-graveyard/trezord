#
# CMake build file for libusb hidapi
#

add_library(hidapi hid.c)

include(Findiconv.cmake)
include(Findlibusb-1.0.cmake)

find_library(RT_LIBRARY rt REQUIRED)
find_library(ICONV_LIBRARIES iconv REQUIRED)
find_library(LIBUSB_1_LIBRARIES libusb-1.0 REQUIRED)

include_directories(${RT_INCLUDE_DIRS})
include_directories(${ICONV_INCLUDE_DIRS})
include_directories(${LIBUSB_1_INCLUDE_DIRS})

target_link_libraries(hidapi
  ${LIBUSB_1_LIBRARIES}
  ${ICONV_LIBRARIES}
  ${RT_LIBRARY})
