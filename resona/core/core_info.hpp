/*
 * SPDX-License-Identifier: MIT
 *
 * core_info.hpp
 *
 * Placeholder translation unit for the Maya-free Resona core library.
 * Real modules (pattern, mesher, seams, solver) will live next to it.
 */
#pragma once

#include <string_view>

namespace resona::core
{

/// Name of the core library — lets the plugin and tests verify linkage.
[[nodiscard]] std::string_view library_name() noexcept;

} // namespace resona::core
