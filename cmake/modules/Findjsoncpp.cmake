#
# CMake package file for jsoncpp
#

find_path(JSONCPP_INCLUDE_DIR json
  HINTS "${CMAKE_SOURCE_DIR}/vendor/jsoncpp/include")
find_library(JSONCPP_LIBRARY NAMES json
  HINTS "${CMAKE_SOURCE_DIR}/lib/jsoncpp/lib")

set(JSONCPP_LIBRARIES ${JSONCPP_LIBRARY})
set(JSONCPP_INCLUDE_DIRS ${JSONCPP_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(JSONCPP
  DEFAULT_MSG JSONCPP_LIBRARY JSONCPP_INCLUDE_DIR)
