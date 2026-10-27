# =========================================================================
# tests/CMakeLists.txt — included via add_subdirectory(tests) when
# BUILD_TESTS=ON.
# =========================================================================

# ── Catch2 C++ unit tests (no Maya SDK required) ──────────────────────
include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.7.1
)
FetchContent_MakeAvailable(Catch2)

add_subdirectory(unit_tests/core)

# ── Maya integration tests via CTest (requires Maya SDK + mayapy) ─────
if(BUILD_NATIVE_MODULE AND Maya_FOUND)
    get_filename_component(_maya_root "${Maya_INCLUDE_DIR}" DIRECTORY)
    find_program(MAYAPY_EXECUTABLE
        NAMES mayapy mayapy.exe
        PATHS "${_maya_root}/bin"
              "C:/Program Files/Autodesk/Maya${MAYA_VERSION}/bin"
    )

    if(MAYAPY_EXECUTABLE)
        message(STATUS "mayapy: ${MAYAPY_EXECUTABLE}")
        add_test(NAME "maya-integration-load"
            COMMAND "${MAYAPY_EXECUTABLE}"
                    "${CMAKE_SOURCE_DIR}/tests/integration/maya/run_all_integration_tests.py"
        )
        set_tests_properties("maya-integration-load" PROPERTIES
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
            TIMEOUT 600
            LABELS "maya;integration"
            ENVIRONMENT "PYTHONIOENCODING=utf-8;PYTHONUNBUFFERED=1;MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/plug-ins;PYTHONPATH=${CMAKE_SOURCE_DIR}"
        )
    else()
        message(WARNING "mayapy not found — Maya integration tests disabled.")
    endif()
    unset(_maya_root)
endif()
