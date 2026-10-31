/*
 * SPDX-License-Identifier: MIT
 *
 * TypeIds.hpp
 *
 * Maya node type IDs for every Resona node, in one place.
 *
 * These live in Maya's LOCAL range (0x00000–0x7FFFF), which is fine for
 * development but must not ship: scenes saved with local IDs clash with other
 * studios' local plugins.  Request a block from Autodesk before the first
 * public release and remap here (the IDs are stored in .mb files).
 *
 * Attribute short names: always prefix with "rs".  Short names share one
 * namespace with every inherited attribute (dagNode, shape, locator…) and a
 * clash — e.g. "pm" vs. dagNode.parentMatrix — makes initialize() fail and
 * Maya refuses to register the node type.
 */
#pragma once

#include <maya/MTypeId.h>

namespace resona::maya::type_ids
{

inline constexpr unsigned int kSolver = 0x0007F100;
inline constexpr unsigned int kGarment = 0x0007F101;
inline constexpr unsigned int kPanel = 0x0007F102;

} // namespace resona::maya::type_ids
