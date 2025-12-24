cmake_minimum_required(VERSION 3.25)

# Must be set before project() for vcpkg toolchain
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "Vcpkg toolchain file")
endif()

project(arcdps-squad-ready-plugin
    VERSION 0.1.0
    DESCRIPTION "ArcDPS Squad Ready Plugin"
    LANGUAGES CXX C
)

# C++ Standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# C Standard for miniaudio
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Export compile commands for IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Common compile definitions
add_compile_definitions(
    UNICODE
    _UNICODE
    NOMINMAX
    IMGUI_DEFINE_MATH_OPERATORS
    MAGIC_ENUM_RANGE_MAX=256
)

# Configuration-specific definitions
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(_DEBUG)
else()
    add_compile_definitions(NDEBUG)
endif()

# Find vcpkg packages
find_package(CURL CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(magic_enum CONFIG REQUIRED)

# =============================================================================
# ImGui Library (from submodule)
# =============================================================================
add_library(imgui STATIC
    modules/imgui/imgui.cpp
    modules/imgui/imgui_demo.cpp
    modules/imgui/imgui_draw.cpp
    modules/imgui/imgui_tables.cpp
    modules/imgui/imgui_widgets.cpp
)

target_include_directories(imgui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/imgui
)

target_compile_definitions(imgui PUBLIC
    IMGUI_DEFINE_MATH_OPERATORS
)

# =============================================================================
# Unofficial Extras Library (from submodule)
# =============================================================================
add_library(unofficial_extras STATIC
    modules/unofficial_extras/KeyBindHelper.cpp
    modules/unofficial_extras/KeyBindsTranslation.cpp
)

target_include_directories(unofficial_extras PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/unofficial_extras
)

target_link_libraries(unofficial_extras PUBLIC
    magic_enum::magic_enum
)

# Create an ALIAS target so ArcdpsUnofficialExtras can be found via include paths
# The extension expects <ArcdpsUnofficialExtras/KeyBindHelper.h> style includes
# We accomplish this by creating a directory-based include mapping
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/include/ArcdpsUnofficialExtras)
# Copy headers to the expected location (using configure_file for proper cmake dependency tracking)
foreach(HEADER Definitions.h KeyBindHelper.h KeyBindsTranslation.h KeyBindStructs.h)
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/modules/unofficial_extras/${HEADER}
        ${CMAKE_CURRENT_BINARY_DIR}/include/ArcdpsUnofficialExtras/${HEADER}
        COPYONLY
    )
endforeach()

# Add the include path for ArcdpsUnofficialExtras-style includes
target_include_directories(unofficial_extras PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}/include
)

# Create magic_enum directory mapping for <magic_enum/magic_enum.hpp> style includes
# vcpkg puts magic_enum headers directly in include/, but the extension expects them in include/magic_enum/
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/include/magic_enum)
file(GLOB MAGIC_ENUM_HEADERS "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include/magic_enum*.hpp")
foreach(HEADER_PATH ${MAGIC_ENUM_HEADERS})
    get_filename_component(HEADER_NAME ${HEADER_PATH} NAME)
    configure_file(
        ${HEADER_PATH}
        ${CMAKE_CURRENT_BINARY_DIR}/include/magic_enum/${HEADER_NAME}
        COPYONLY
    )
endforeach()

# =============================================================================
# ArcDPS Extension Library (from submodule)
# =============================================================================
set(ARCDPS_EXTENSION_SOURCES
    modules/extension/arcdps_structs.cpp
    modules/extension/CombatEventHandler.cpp
    modules/extension/EventSequencer.cpp
    modules/extension/imgui_stdlib.cpp
    modules/extension/KeyBindHandler.cpp
    modules/extension/KeyInput.cpp
    modules/extension/Localization.cpp
    modules/extension/SimpleNetworkStack.cpp
    modules/extension/Singleton.cpp
    modules/extension/UpdateChecker.cpp
    modules/extension/UpdateCheckerBase.cpp
    modules/extension/Widgets.cpp
    modules/extension/Windows/KeyBindComponent.cpp
    modules/extension/Windows/MainWindow.cpp
    modules/extension/Windows/PositioningComponent.cpp
)

# Demo files have issues with MinGW (missing <atomic> include in submodule headers)
if(NOT MINGW)
    list(APPEND ARCDPS_EXTENSION_SOURCES
        modules/extension/Windows/Demo/DemoKeyBindComponent.cpp
        modules/extension/Windows/Demo/DemoPositioningComponent.cpp
        modules/extension/Windows/Demo/DemoTable.cpp
        modules/extension/Windows/Demo/DemoTableWindow.cpp
        modules/extension/Windows/Demo/DemoWindow.cpp
    )
endif()

# IconLoader.cpp and ArcdpsExtension.cpp use ATL headers (atlbase.h) which are not available in MinGW
if(NOT MINGW)
    list(APPEND ARCDPS_EXTENSION_SOURCES 
        modules/extension/IconLoader.cpp
        modules/extension/ArcdpsExtension.cpp
    )
endif()

add_library(arcdps_extension STATIC ${ARCDPS_EXTENSION_SOURCES})

target_include_directories(arcdps_extension PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/extension
    ${CMAKE_CURRENT_SOURCE_DIR}/modules
)

target_link_libraries(arcdps_extension PUBLIC
    imgui
    unofficial_extras
    CURL::libcurl
    nlohmann_json::nlohmann_json
    magic_enum::magic_enum
)

# =============================================================================
# ImGuiFileDialog Library (from submodule)
# =============================================================================
add_library(imgui_file_dialog STATIC
    modules/ImGuiFileDialog/ImGuiFileDialog.cpp
)

target_include_directories(imgui_file_dialog PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/ImGuiFileDialog
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/ImGuiFileDialog/dirent
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/ImGuiFileDialog/stb
)

target_link_libraries(imgui_file_dialog PUBLIC
    imgui
)

# =============================================================================
# Miniaudio Library (from submodule)
# =============================================================================
add_library(miniaudio STATIC
    modules/miniaudio/extras/miniaudio_split/miniaudio.c
)

target_include_directories(miniaudio PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/miniaudio/extras/miniaudio_split
)

# =============================================================================
# Main Plugin
# =============================================================================
add_subdirectory(squad_ready)


