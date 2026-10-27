# =========================================================================
# resona/core — Maya-free C++ core (static library)
#
# Pure C++ logic (patterns, meshing, seams, solver) — no Maya SDK required,
# so it can be unit-tested and benchmarked outside Maya.
# =========================================================================

add_library(resona_core STATIC
    core_info.cpp
)

# Linked into the shared Maya plugin, so objects must be PIC on ELF targets.
set_target_properties(resona_core PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(resona_core PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
)
target_compile_features(resona_core PUBLIC cxx_std_17)
