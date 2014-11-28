set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++ -Wl,--wrap=memcpy -Wl,--wrap=secure_getenv" CACHE STRING "")
