# CMake toolchain file for cross-compiling to Windows using MinGW-w64
# Usage: cmake -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-mingw-w64.cmake ..

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Specify the cross-compiler
# On macOS, install via: brew install mingw-w64
# On Linux, install via: apt install mingw-w64 (or equivalent)
set(TOOLCHAIN_PREFIX x86_64-w64-mingw32)

# Find the compilers - try common locations
find_program(CMAKE_C_COMPILER NAMES
    ${TOOLCHAIN_PREFIX}-gcc
    ${TOOLCHAIN_PREFIX}-clang
    PATHS
    /opt/homebrew/bin
    /usr/local/bin
    /usr/bin
    REQUIRED
)

find_program(CMAKE_CXX_COMPILER NAMES
    ${TOOLCHAIN_PREFIX}-g++
    ${TOOLCHAIN_PREFIX}-clang++
    PATHS
    /opt/homebrew/bin
    /usr/local/bin
    /usr/bin
    REQUIRED
)

find_program(CMAKE_RC_COMPILER NAMES
    ${TOOLCHAIN_PREFIX}-windres
    PATHS
    /opt/homebrew/bin
    /usr/local/bin
    /usr/bin
    REQUIRED
)

# Where to look for the target environment
set(CMAKE_FIND_ROOT_PATH
    /opt/homebrew/opt/mingw-w64
    /opt/homebrew/${TOOLCHAIN_PREFIX}
    /usr/local/${TOOLCHAIN_PREFIX}
    /usr/${TOOLCHAIN_PREFIX}
)

# Add vcpkg installed directory to find root path if available
if(DEFINED _VCPKG_INSTALLED_DIR AND DEFINED VCPKG_TARGET_TRIPLET)
    list(APPEND CMAKE_FIND_ROOT_PATH "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}")
endif()

# Adjust the default behavior of FIND_XXX() commands:
# - Search headers and libraries in the target environment (BOTH allows vcpkg paths)
# - Search programs in the host environment
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)

# Windows-specific settings
set(WIN32 TRUE)
set(MINGW TRUE)

# Set vcpkg triplet for cross-compilation
set(VCPKG_TARGET_TRIPLET "x64-mingw-static" CACHE STRING "")
set(VCPKG_HOST_TRIPLET "x64-osx" CACHE STRING "")

# Compiler flags for better compatibility
set(CMAKE_C_FLAGS_INIT "-static-libgcc")
set(CMAKE_CXX_FLAGS_INIT "-static-libgcc -static-libstdc++")

# Enable position independent code
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Shared library suffix for Windows
set(CMAKE_SHARED_LIBRARY_SUFFIX ".dll")
set(CMAKE_SHARED_LIBRARY_PREFIX "")

# Import library for DLLs
set(CMAKE_IMPORT_LIBRARY_SUFFIX ".dll.a")

# Resource compiler
set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> -O coff <DEFINES> <INCLUDES> <FLAGS> <SOURCE> <OBJECT>")


