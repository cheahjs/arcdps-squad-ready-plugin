# Squad Ready Plugin CMakeLists.txt

# Plugin source files
set(PLUGIN_SOURCES
    Audio.cpp
    dllmain.cpp
    Globals.cpp
    Logging.cpp
    Settings.cpp
    SettingsUI.cpp
    SquadTracker.cpp
)

set(PLUGIN_HEADERS
    Audio.h
    Error.h
    Globals.h
    Logging.h
    resource.h
    Settings.h
    SettingsUI.h
    SquadTracker.h
)

# Windows resource file for embedded assets
set(PLUGIN_RESOURCES
    arcdps-squad-ready-plugin.rc
)

# Set resource compiler include path for MinGW
if(MINGW)
    set(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} -I ${CMAKE_CURRENT_SOURCE_DIR}")
endif()

# Create the shared library (DLL)
add_library(arcdps_squad_ready SHARED
    ${PLUGIN_SOURCES}
    ${PLUGIN_HEADERS}
    ${PLUGIN_RESOURCES}
)

# Set output name
set_target_properties(arcdps_squad_ready PROPERTIES
    OUTPUT_NAME "arcdps_squad_ready"
    PREFIX ""
    SUFFIX ".dll"
)

# Include directories
target_include_directories(arcdps_squad_ready PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/modules/imgui
    ${CMAKE_SOURCE_DIR}/modules
)

# Link libraries
target_link_libraries(arcdps_squad_ready PRIVATE
    arcdps_extension
    imgui
    imgui_file_dialog
    unofficial_extras
    miniaudio
    CURL::libcurl
    nlohmann_json::nlohmann_json
    magic_enum::magic_enum
)

# Windows-specific libraries
if(WIN32)
    target_link_libraries(arcdps_squad_ready PRIVATE
        Version
        Ws2_32
        crypt32
        Wldap32
        winmm
        kernel32
        user32
        gdi32
        winspool
        comdlg32
        advapi32
        shell32
        ole32
        oleaut32
        uuid
        odbc32
        odbccp32
    )
endif()

# Compile definitions specific to the plugin
target_compile_definitions(arcdps_squad_ready PRIVATE
    ARCDPSSQUADREADYPLUGIN_EXPORTS
    _WINDOWS
    _USRDLL
)

# Export all symbols for Windows DLL entry point
if(MINGW)
    target_link_options(arcdps_squad_ready PRIVATE
        -Wl,--export-all-symbols
    )
endif()

# Copy sound files to output directory
add_custom_command(TARGET arcdps_squad_ready POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/sounds
        $<TARGET_FILE_DIR:arcdps_squad_ready>/sounds
    COMMENT "Copying sound files..."
)

# Install target
install(TARGETS arcdps_squad_ready
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
)

install(DIRECTORY sounds/
    DESTINATION bin/sounds
)


