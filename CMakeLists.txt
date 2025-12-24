cmake_minimum_required(VERSION 3.21)

project(arcdps_squad_ready VERSION 0.0.1 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows-specific settings for cross-compilation
if(WIN32)
    add_compile_definitions(
        WIN32
        _WINDOWS
        NOMINMAX
        UNICODE
        _UNICODE
        IMGUI_DEFINE_MATH_OPERATORS
        MAGIC_ENUM_RANGE_MAX=256
        ARCDPS_EXTENSION_NO_LANG_H
    )
    
    # Static runtime for portability
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# Find vcpkg packages
find_package(cpr CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(magic_enum CONFIG REQUIRED)

# Add subdirectories
add_subdirectory(modules/imgui)
add_subdirectory(modules/unofficial_extras)
add_subdirectory(modules/extension)
add_subdirectory(squad_ready)
