# Custom triplet for MinGW-w64 static builds (cross-compilation from macOS/Linux)
set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE static)
set(VCPKG_LIBRARY_LINKAGE static)

set(VCPKG_CMAKE_SYSTEM_NAME MinGW)

# Suppress deprecation warnings (cpr uses deprecated curl APIs in version 1.8.3)
set(VCPKG_CXX_FLAGS "-Wno-deprecated-declarations")
set(VCPKG_C_FLAGS "-Wno-deprecated-declarations")

# Chainload the MinGW toolchain
if(NOT DEFINED VCPKG_CHAINLOAD_TOOLCHAIN_FILE)
    if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../toolchain-mingw-w64.cmake")
        set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/../toolchain-mingw-w64.cmake")
    endif()
endif()


