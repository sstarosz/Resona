add_executable(resona_core_tests
    test_core_info.cpp
)
target_link_libraries(resona_core_tests PRIVATE resona_core Catch2::Catch2WithMain)

list(APPEND CMAKE_MODULE_PATH "${catch2_SOURCE_DIR}/extras")
include(Catch)
catch_discover_tests(resona_core_tests)
