set(CMAKE_SYSTEM_NAME "Darwin" CACHE STRING "")
set(CMAKE_SYSTEM_VERSION "10.8" CACHE STRING "")
set(TARGET_ARCH "x86_64" CACHE STRING "")

set(CMAKE_C_COMPILER "o64-gcc" CACHE STRING "")
set(CMAKE_CXX_COMPILER "o64-g++" CACHE STRING "")
set(CMAKE_AR "x86_64-apple-darwin12-ar" CACHE STRING "")
set(CMAKE_RANLIB "x86_64-apple-darwin12-ranlib" CACHE STRING "")
set(PKG_CONFIG_EXECUTABLE "x86_64-apple-darwin12-pkg-config" CACHE STRING "")
set(CMAKE_CXX_FLAGS "-v" CACHE STRING "")

set(CMAKE_OSX_SYSROOT "/opt/osxcross/target/SDK/MacOSX10.8.sdk" CACHE STRING "")
set(CMAKE_FIND_ROOT_PATH "/opt/osxcross/target/macports/pkgs/opt/local" CACHE STRING "")

include_directories("/opt/osxcross/target/macports/pkgs/opt/local/include")
link_directories("/opt/osxcross/target/macports/pkgs/opt/local/lib")
