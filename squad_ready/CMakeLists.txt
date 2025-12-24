# arcdps_squad_ready plugin DLL
add_library(arcdps_squad_ready SHARED
    dllmain.cpp
    Audio.cpp
    Globals.cpp
    Logging.cpp
    Settings.cpp
    SettingsUI.cpp
    SquadTracker.cpp
    # ImGuiFileDialog
    ${CMAKE_SOURCE_DIR}/modules/ImGuiFileDialog/ImGuiFileDialog.cpp
    # miniaudio
    ${CMAKE_SOURCE_DIR}/modules/miniaudio/extras/miniaudio_split/miniaudio.c
)

target_include_directories(arcdps_squad_ready PRIVATE
    ${CMAKE_SOURCE_DIR}/modules/imgui
    ${CMAKE_SOURCE_DIR}/modules
    ${CMAKE_SOURCE_DIR}/modules/ImGuiFileDialog
    ${CMAKE_SOURCE_DIR}/modules/miniaudio/extras/miniaudio_split
)

target_link_libraries(arcdps_squad_ready PRIVATE
    arcdps_extension
    imgui
    unofficial_extras
    cpr::cpr
    nlohmann_json::nlohmann_json
    magic_enum::magic_enum
)

target_compile_definitions(arcdps_squad_ready PRIVATE
    ARCDPSSQUADREADYPLUGIN_EXPORTS
    ARCDPS_EXTENSION_NO_LANG_H
)

# Windows system libraries
if(WIN32)
    target_link_libraries(arcdps_squad_ready PRIVATE
        version
        ws2_32
        crypt32
        wldap32
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

# Set output name without lib prefix
set_target_properties(arcdps_squad_ready PROPERTIES
    PREFIX ""
    OUTPUT_NAME "arcdps_squad_ready"
)
