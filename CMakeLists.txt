cmake_minimum_required(VERSION 3.21)
project(resona-native LANGUAGES CXX)

# ── C++ standard ─────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Maya version ────────────────────────────────────────────────────────
# Set via CMakePresets.json (see MAYA_VERSION cache variable).
if(NOT DEFINED MAYA_VERSION)
    set(MAYA_VERSION "2026" CACHE STRING "Maya version to target")
endif()
message(STATUS "Targeting Maya ${MAYA_VERSION}")

# ── Options ──────────────────────────────────────────────────────────────
option(BUILD_NATIVE_MODULE "Build the C++ native plugin (Resona.mll)" ON)
option(BUILD_TESTS "Build tests — Catch2 C++ unit tests + Maya integration suites" OFF)
option(BUILD_DEV_INSTALL "Generate dev-install layout with .mod file for local development" ON)

# ── Python (for SDK download / packaging scripts) ───────────────────────
set(Python_FIND_STRATEGY LOCATION)
find_package(Python 3.10 QUIET COMPONENTS Interpreter)
if(Python_FOUND)
    message(STATUS "Python ${Python_VERSION} (for helper scripts)")
endif()

# ── Resolve project version from git tags ────────────────────────────
include("${CMAKE_SOURCE_DIR}/cmake/GitVersion.cmake")
get_git_version(RESONA_PROJECT_VERSION)

if(NOT RESONA_PROJECT_VERSION)
    # No git available — try reading the last generated _version.py
    set(_version_py "${CMAKE_SOURCE_DIR}/resona/_version.py")
    if(EXISTS "${_version_py}")
        file(STRINGS "${_version_py}" _version_line REGEX "^__version__[ \t]*=")
        if(_version_line)
            string(REGEX REPLACE ".*\"([^\"]+)\".*" "\\1" RESONA_PROJECT_VERSION "${_version_line}")
        endif()
    endif()
endif()

if(NOT RESONA_PROJECT_VERSION)
    set(RESONA_PROJECT_VERSION "0.1.0")  # last-resort fallback
endif()
message(STATUS "Resona version: ${RESONA_PROJECT_VERSION}")

# ── Generate version files from templates ────────────────────────────
# Python: resona/_version.py  (read by resona.plugin at import time)
configure_file(
    "${CMAKE_SOURCE_DIR}/cmake/_version.py.in"
    "${CMAKE_SOURCE_DIR}/resona/_version.py"
    @ONLY
)
# C++: version.hpp  (included by Resona.cpp)
configure_file(
    "${CMAKE_SOURCE_DIR}/cmake/version.hpp.in"
    "${CMAKE_BINARY_DIR}/generated/version.hpp"
    @ONLY
)

# =========================================================================
# Maya-free C++ core (cloth / pattern / solver code) — static lib.
# Builds without the Maya SDK so it can be unit-tested on its own.
# =========================================================================
add_subdirectory(resona/core)

# =========================================================================
# Native plugin — single entry point for Resona
# =========================================================================
if(BUILD_NATIVE_MODULE)
    # ── SDK setup ────────────────────────────────────────────────────────
    # The Maya SDK is proprietary Autodesk software and is NOT in this repo.
    # CMake auto-downloads it via scripts/ci/strip_maya_sdk.py ensure, or use
    # an existing one:
    #   cmake --preset maya2026-release -DSDK_DIR=C:/Maya2026/devkitBase
    #   cmake --preset maya2026-release -DSDK_DIR=../../MayaMMD/out/.sdk/sdk-maya2026
    if(NOT DEFINED SDK_DIR)
        set(SDK_DIR "${CMAKE_SOURCE_DIR}/out/.sdk/sdk-maya${MAYA_VERSION}")

        if(NOT EXISTS "${SDK_DIR}/include/maya/MFnPlugin.h")
            if(NOT Python_FOUND)
                message(FATAL_ERROR
                    "Maya ${MAYA_VERSION} SDK not found and Python not available for auto-download.\n"
                    "  Provide the SDK manually: -DSDK_DIR=/path/to/maya-sdk")
            endif()
            message(STATUS "Maya ${MAYA_VERSION} SDK not found — running ensure...")
            execute_process(COMMAND ${Python_EXECUTABLE}
                "${CMAKE_SOURCE_DIR}/scripts/ci/strip_maya_sdk.py" ensure
                --version ${MAYA_VERSION}
                RESULT_VARIABLE _ensure_rc
                WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
            )
            if(NOT _ensure_rc EQUAL 0)
                message(FATAL_ERROR
                    "Failed to download Maya ${MAYA_VERSION} SDK.\n"
                    "  Provide it manually: -DSDK_DIR=/path/to/maya-sdk\n"
                    "  https://www.autodesk.com/developer-network/platform-technologies/maya")
            endif()
        else()
            message(STATUS "Maya ${MAYA_VERSION} SDK found at: ${SDK_DIR}")
        endif()
    else()
        message(STATUS "Using user-provided Maya SDK: ${SDK_DIR}")
        if(NOT EXISTS "${SDK_DIR}/include/maya/MFnPlugin.h")
            message(FATAL_ERROR "MFnPlugin.h not found in SDK_DIR=${SDK_DIR}")
        endif()
    endif()

    # ── Find Maya SDK ────────────────────────────────────────────────────
    set(MAYA_LOCATION "${SDK_DIR}")
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
    find_package(Maya REQUIRED)
    message(STATUS "Maya ${Maya_VERSION} SDK: ${Maya_INCLUDE_DIR} (libs: ${Maya_LIBRARY_DIR})")

    # =====================================================================
    # Resona.mll — SINGLE Maya plugin entry point
    #
    # initializePlugin:
    #   1. Registers native C++ nodes/commands via MFnPlugin
    #   2. Calls Python (resona.plugin.initializePlugin) to register
    #      Python-side nodes/commands and set up the UI — same plugin handle.
    # =====================================================================
    add_library(Resona SHARED
        resona/Resona.cpp
        resona/maya/nodes/GarmentShape.cpp
        resona/maya/nodes/PanelShape.cpp
        resona/maya/nodes/SolverShape.cpp
        resona/maya/viewport/PanelGeometryOverride.cpp
    )
    target_include_directories(Resona PRIVATE
        "${CMAKE_BINARY_DIR}/generated"
        "${CMAKE_CURRENT_SOURCE_DIR}/resona"
        "${CMAKE_CURRENT_SOURCE_DIR}/resona/maya"
    )
    target_link_libraries(Resona PRIVATE Maya::Maya resona_core)

    set_target_properties(Resona PROPERTIES
        PREFIX ""
        SUFFIX "${Maya_PLUGIN_SUFFIX}"
    )

    # PROJECT_ROOT_DIR is used ONLY for sys.path.insert() before importing
    # resona.plugin — it is NOT used for plugin discovery.
    string(REPLACE "\\" "/" _normalized_root "${CMAKE_SOURCE_DIR}")
    target_compile_definitions(Resona PRIVATE
        PROJECT_ROOT_DIR="${_normalized_root}"
    )

    if(WIN32)
        target_compile_definitions(Resona PRIVATE
            $<$<CONFIG:Debug>:_DEBUG>
            _HAS_ITERATOR_DEBUGGING=0
        )
        target_compile_options(Resona PRIVATE /Zi /W4 /utf-8)
    elseif(NOT APPLE)
        target_compile_options(Resona PRIVATE
            -Wall -Wextra -Wno-unused-parameter -Wno-deprecated-declarations
        )
    endif()

    # ── Dev-install: post-build copy + .mod generation ──
    # The .mll is copied to <prefix>/plug-ins/ after every build so Maya and
    # tests always pick up the latest version — no cmake --install needed.
    if(BUILD_DEV_INSTALL)
        file(MAKE_DIRECTORY "${CMAKE_INSTALL_PREFIX}/plug-ins")

        add_custom_command(TARGET Resona POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:Resona>"
                "${CMAKE_INSTALL_PREFIX}/plug-ins/"
            COMMENT "Deploying Resona${Maya_PLUGIN_SUFFIX} → ${CMAKE_INSTALL_PREFIX}/plug-ins/"
        )

        configure_file(
            "${CMAKE_SOURCE_DIR}/cmake/Resona.mod.in"
            "${CMAKE_INSTALL_PREFIX}/Resona.mod"
            @ONLY
        )

        # Auto-deploy the .mod onto Maya's per-version module search path.
        if(WIN32 AND DEFINED ENV{USERPROFILE})
            set(_maya_modules_dir "$ENV{USERPROFILE}/Documents/maya/${MAYA_VERSION}/modules")
            add_custom_command(TARGET Resona POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory "${_maya_modules_dir}"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${CMAKE_INSTALL_PREFIX}/Resona.mod"
                    "${_maya_modules_dir}/Resona.mod"
                COMMENT "Deploying Resona.mod → ${_maya_modules_dir}/"
            )
            unset(_maya_modules_dir)
        endif()

        install(TARGETS Resona
            RUNTIME DESTINATION "plug-ins"
            LIBRARY DESTINATION "plug-ins"
        )
        message(STATUS "Dev-install prefix: ${CMAKE_INSTALL_PREFIX}")
    endif()
endif()

# =========================================================================
# Tests — Catch2 C++ unit tests & Maya integration tests via CTest.
# =========================================================================
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
