// SPDX-License-Identifier: MIT
#include <catch2/catch_test_macros.hpp>

#include "core_info.hpp"

TEST_CASE("resona_core links and reports its name", "[core]")
{
    REQUIRE(resona::core::library_name() == "resona_core");
}
