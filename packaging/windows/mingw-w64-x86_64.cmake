# CMake toolchain for 64-bit Windows with Debian's MinGW-w64. The posix thread
# model provides std::thread/std::jthread (libwinpthread-1.dll at run time).
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(mingwTriplet x86_64-w64-mingw32)
set(CMAKE_C_COMPILER ${mingwTriplet}-gcc-posix)
set(CMAKE_CXX_COMPILER ${mingwTriplet}-g++-posix)
set(CMAKE_RC_COMPILER ${mingwTriplet}-windres)

# Search the MinGW sysroot (and the Qt prefix, which Qt's toolchain file adds)
# for libraries and packages, never the build machine's
set(CMAKE_FIND_ROOT_PATH /usr/${mingwTriplet})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
