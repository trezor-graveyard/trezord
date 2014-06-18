#
# CMake build file for Windows hidapi
#

add_library(hidapi hid.c)
target_link_libraries(hidapi setupapi)