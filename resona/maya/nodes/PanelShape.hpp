/*
 * SPDX-License-Identifier: MIT
 *
 * PanelShape.hpp
 *
 * PanelShape (`resonaPanel`) — one Maya node per pattern panel.
 *
 * Lives under its own transform, which IS the panel's placement: Maya's
 * Move / Rotate tools, multi-select, pivots and undo all work on panels for
 * free.  The shape draws the panel's piece of cloth (a plane in this spike)
 * through PanelGeometryOverride and is selectable, so clicking the cloth
 * selects the panel.
 *
 * Data flow (M0 spike):
 *   panelShape.worldMatrix[0] ─▶ garment.panels[i].panelMatrix
 *   panelShape.width / height ─▶ garment.panels[i].panelWidth / panelHeight
 */
#pragma once

#include <maya/MBoundingBox.h>
#include <maya/MObject.h>
#include <maya/MPxLocatorNode.h>
#include <maya/MTypeId.h>

namespace resona::maya
{

class PanelShape : public MPxLocatorNode
{
  public:
    static const MTypeId kTypeId;
    static constexpr const char* kNodeName = "resonaPanel";
    // VP2 draw-database classification — PanelGeometryOverride is registered under it.
    static constexpr const char* kNodeClassify = "drawdb/geometry/resonaPanel";

    static void* creator();
    static MStatus initialize();

    bool isBounded() const override;
    MBoundingBox boundingBox() const override;

    // Spike stand-in for the real pattern: a width × height plane (cm),
    // centred on the panel's origin in its local XY plane, facing +Z.
    static MObject aWidth;
    static MObject aHeight;
};

} // namespace resona::maya
