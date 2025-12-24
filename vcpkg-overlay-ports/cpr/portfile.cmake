vcpkg_check_linkage(ONLY_STATIC_LIBRARY)

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO libcpr/cpr
    REF db351ffbbadc6c4e9239daaa26e9aefa9f0ec82d #v1.8.3
    SHA512 4ce4e791c5a29584eaf147b2efc4e5c0fb363cf254e6ec0c66734f7ec8da538b5b295300fee3de218da6daf4107fc11e0eac695885a3380e03a72720d328c3db
    HEAD_REF master
    PATCHES
        001-cpr-config.patch
)

vcpkg_check_features(OUT_FEATURE_OPTIONS FEATURE_OPTIONS
    FEATURES
        ssl CPR_ENABLE_SSL
)

# MinGW: Suppress deprecated curl API warnings (cpr 1.8.3 uses APIs deprecated in curl 7.56.0)
set(CPR_EXTRA_OPTIONS "")
if(VCPKG_TARGET_IS_MINGW)
    list(APPEND CPR_EXTRA_OPTIONS
        "-DCMAKE_CXX_FLAGS=-Wno-deprecated-declarations -Wno-error=deprecated-declarations"
        "-DCMAKE_C_FLAGS=-Wno-deprecated-declarations -Wno-error=deprecated-declarations"
    )
endif()

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS 
        -DCPR_BUILD_TESTS=OFF
        -DCPR_FORCE_USE_SYSTEM_CURL=ON
		-DCPR_FORCE_OPENSSL_BACKEND=ON
        ${FEATURE_OPTIONS}
        ${CPR_EXTRA_OPTIONS}
    OPTIONS_DEBUG
        -DDISABLE_INSTALL_HEADERS=ON
)

vcpkg_cmake_install()
vcpkg_cmake_config_fixup(CONFIG_PATH lib/cmake/cpr)

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")

file(INSTALL "${SOURCE_PATH}/LICENSE" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)
