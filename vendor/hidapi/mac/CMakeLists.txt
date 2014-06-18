#
# CMake build file for Mac OS X hidapi
#

add_library(hidapi hid.c)

find_library(COREFOUNDATION_LIBRARY CoreFoundation)
find_library(IOKIT_LIBRARY IOKit)

target_link_libraries(hidapi
  ${COREFOUNDATION_LIBRARY}
  ${IOKIT_LIBRARY})
