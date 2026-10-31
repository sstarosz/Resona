/*
 * SPDX-License-Identifier: MIT
 *
 * GarmentShape.hpp
 *
 * GarmentShape (`resonaGarment`) — one Maya node per garment.
 *
 * Collects its panels through the `panels[]` compound array (membership comes
 * from these connections, not from the Outliner hierarchy) and passes them on
 * to the solver as two packed arrays.  Will also hold the garment's fabric
 * and mesh resolution.  Draws nothing.
 *
 *   garment.outPanelMatrices / outPanelSizes ─▶ solver.garments[i]
 */
#pragma once

#include <maya/MDataBlock.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MPxLocatorNode.h>
#include <maya/MTypeId.h>

namespace resona::maya
{

class GarmentShape : public MPxLocatorNode
{
  public:
    static const MTypeId kTypeId;
    static constexpr const char* kNodeName = "resonaGarment";

    static void* creator();
    static MStatus initialize();

    MStatus compute(const MPlug& plug, MDataBlock& data) override;
    bool isBounded() const override;

    // Inputs — one element per connected panel.
    static MObject aPanels;      // compound array
    static MObject aPanelMatrix; //   matrix  <- panelShape.worldMatrix[0]
    static MObject aPanelWidth;  //   double  <- panelShape.width
    static MObject aPanelHeight; //   double  <- panelShape.height

    // Outputs — packed per-panel data, in panels[] logical-index order.
    static MObject aOutPanelMatrices; // matrixArray
    static MObject aOutPanelSizes;    // vectorArray (width, height, 0)
};

} // namespace resona::maya
