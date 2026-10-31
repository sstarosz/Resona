/*
 * SPDX-License-Identifier: MIT
 *
 * SolverShape.hpp
 *
 * SolverShape (`resonaSolver`) — one per scene, nucleus-style.
 *
 * Holds the simulation globals (gravity, substeps, …) and — later — the
 * Resona scene (patterns, seams, edit history, caches).  Garments connect
 * into `garments[]`; the solver writes one regular Maya mesh per garment to
 * `outMesh[]` (same logical index).
 *
 * M0 spike: no simulation — each garment's output is its panels as flat
 * quads in world space, with per-panel UVs side by side.
 */
#pragma once

#include <maya/MDataBlock.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MPxLocatorNode.h>
#include <maya/MTypeId.h>

namespace resona::maya
{

class SolverShape : public MPxLocatorNode
{
  public:
    static const MTypeId kTypeId;
    static constexpr const char* kNodeName = "resonaSolver";

    static void* creator();
    static MStatus initialize();

    MStatus compute(const MPlug& plug, MDataBlock& data) override;
    bool isBounded() const override;

    // Simulation globals (unused by the spike, but this is where they live).
    static MObject aGravity; // float3, cm/s²

    // Inputs — one element per garment.
    static MObject aGarments;             // compound array
    static MObject aGarmentPanelMatrices; //   matrixArray <- garment.outPanelMatrices
    static MObject aGarmentPanelSizes;    //   vectorArray <- garment.outPanelSizes

    // Outputs — one mesh per garment (same logical index as garments[]).
    static MObject aOutMesh; // mesh array
};

} // namespace resona::maya
