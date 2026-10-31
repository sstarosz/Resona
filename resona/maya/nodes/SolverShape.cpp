// SPDX-License-Identifier: MIT
#include "nodes/SolverShape.hpp"

#include "nodes/TypeIds.hpp"

#include <maya/MArrayDataBuilder.h>
#include <maya/MArrayDataHandle.h>
#include <maya/MFloatArray.h>
#include <maya/MFloatPointArray.h>
#include <maya/MFnCompoundAttribute.h>
#include <maya/MFnMatrixArrayData.h>
#include <maya/MFnMesh.h>
#include <maya/MFnMeshData.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnVectorArrayData.h>
#include <maya/MIntArray.h>
#include <maya/MMatrixArray.h>
#include <maya/MPoint.h>
#include <maya/MVectorArray.h>

#include <algorithm>

namespace resona::maya
{

const MTypeId SolverShape::kTypeId(type_ids::kSolver);

MObject SolverShape::aGravity;
MObject SolverShape::aGarments;
MObject SolverShape::aGarmentPanelMatrices;
MObject SolverShape::aGarmentPanelSizes;
MObject SolverShape::aOutMesh;

namespace
{

// Build one garment's mesh: every panel as a world-space quad.  UVs lay the
// panels side by side in [0,1]² (stand-in for real pattern-space UVs).
MObject buildGarmentMesh(const MMatrixArray& matrices, const MVectorArray& sizes, MStatus& stat)
{
    MFnMeshData meshDataFn;
    MObject meshData = meshDataFn.create(&stat);
    if (!stat)
    {
        return MObject::kNullObj;
    }

    const unsigned int panelCount = std::min(matrices.length(), sizes.length());
    MFloatPointArray points;
    MIntArray counts;
    MIntArray connects;
    MFloatArray us;
    MFloatArray vs;

    for (unsigned int p = 0; p < panelCount; ++p)
    {
        const double halfW = sizes[p].x * 0.5;
        const double halfH = sizes[p].y * 0.5;
        // Counter-clockwise seen from +Z (the panel's front).
        const MPoint corners[4] = {MPoint(-halfW, -halfH, 0.0), MPoint(halfW, -halfH, 0.0),
                                   MPoint(halfW, halfH, 0.0), MPoint(-halfW, halfH, 0.0)};
        const float u0 = static_cast<float>(p) / static_cast<float>(panelCount);
        const float uSpan = 1.0f / static_cast<float>(panelCount);
        const float uvs[4][2] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};

        const int base = static_cast<int>(points.length());
        for (int c = 0; c < 4; ++c)
        {
            const MPoint world = corners[c] * matrices[p];
            points.append(MFloatPoint(static_cast<float>(world.x), static_cast<float>(world.y),
                                      static_cast<float>(world.z)));
            connects.append(base + c);
            us.append(u0 + uvs[c][0] * uSpan * 0.95f);
            vs.append(uvs[c][1]);
        }
        counts.append(4);
    }

    MFnMesh meshFn;
    meshFn.create(static_cast<int>(points.length()), static_cast<int>(counts.length()), points,
                  counts, connects, us, vs, meshData, &stat);
    if (!stat)
    {
        return MObject::kNullObj;
    }
    if (counts.length() > 0)
    {
        meshFn.assignUVs(counts, connects);
    }
    return meshData;
}

} // namespace

void* SolverShape::creator()
{
    return new SolverShape();
}

MStatus SolverShape::initialize()
{
    MStatus stat;
    MFnNumericAttribute nAttr;
    MFnCompoundAttribute cAttr;
    MFnTypedAttribute tAttr;

    aGravity = nAttr.create("gravity", "rsGrv", MFnNumericData::k3Float, 0.0, &stat);
    CHECK_MSTATUS_AND_RETURN_IT(stat);
    nAttr.setDefault(0.0f, -980.665f, 0.0f);
    nAttr.setKeyable(true);
    CHECK_MSTATUS_AND_RETURN_IT(addAttribute(aGravity));

    aGarmentPanelMatrices = tAttr.create("garmentPanelMatrices", "rsGpm", MFnData::kMatrixArray,
                                         MFnMatrixArrayData().create(), &stat);
    CHECK_MSTATUS_AND_RETURN_IT(stat);
    tAttr.setStorable(false);
    tAttr.setHidden(true);

    aGarmentPanelSizes = tAttr.create("garmentPanelSizes", "rsGps", MFnData::kVectorArray,
                                      MFnVectorArrayData().create(), &stat);
    CHECK_MSTATUS_AND_RETURN_IT(stat);
    tAttr.setStorable(false);
    tAttr.setHidden(true);

    aGarments = cAttr.create("garments", "rsGar", &stat);
    CHECK_MSTATUS_AND_RETURN_IT(stat);
    cAttr.addChild(aGarmentPanelMatrices);
    cAttr.addChild(aGarmentPanelSizes);
    cAttr.setArray(true);
    cAttr.setIndexMatters(false);
    cAttr.setHidden(true);
    CHECK_MSTATUS_AND_RETURN_IT(addAttribute(aGarments));

    aOutMesh = tAttr.create("outMesh", "rsOm", MFnData::kMesh, MObject::kNullObj, &stat);
    CHECK_MSTATUS_AND_RETURN_IT(stat);
    tAttr.setArray(true);
    tAttr.setUsesArrayDataBuilder(true);
    tAttr.setWritable(false);
    tAttr.setStorable(false);
    tAttr.setHidden(true);
    CHECK_MSTATUS_AND_RETURN_IT(addAttribute(aOutMesh));

    for (const MObject& in : {aGarments, aGarmentPanelMatrices, aGarmentPanelSizes})
    {
        attributeAffects(in, aOutMesh);
    }
    return MS::kSuccess;
}

bool SolverShape::isBounded() const
{
    return false;
}

MStatus SolverShape::compute(const MPlug& plug, MDataBlock& data)
{
    const bool isOutMesh = plug == aOutMesh || (plug.isElement() && plug.array() == aOutMesh);
    if (!isOutMesh)
    {
        return MS::kUnknownParameter;
    }

    MStatus stat;
    MArrayDataHandle garments = data.inputArrayValue(aGarments, &stat);
    CHECK_MSTATUS_AND_RETURN_IT(stat);
    MArrayDataHandle outMeshes = data.outputArrayValue(aOutMesh, &stat);
    CHECK_MSTATUS_AND_RETURN_IT(stat);
    MArrayDataBuilder builder = outMeshes.builder(&stat);
    CHECK_MSTATUS_AND_RETURN_IT(stat);

    // Every garment's mesh is rebuilt on any change — fine for the spike; the
    // real solver tracks dirtiness per garment / panel internally.
    const unsigned int count = garments.elementCount();
    for (unsigned int i = 0; i < count; ++i)
    {
        garments.jumpToArrayElement(i);
        const unsigned int logical = garments.elementIndex();
        MDataHandle garment = garments.inputValue();

        // Unconnected / never-computed elements hold no data — treat as empty.
        MMatrixArray matrices;
        MVectorArray sizes;
        const MObject matrixObj = garment.child(aGarmentPanelMatrices).data();
        const MObject sizeObj = garment.child(aGarmentPanelSizes).data();
        if (!matrixObj.isNull())
        {
            matrices = MFnMatrixArrayData(matrixObj).array();
        }
        if (!sizeObj.isNull())
        {
            sizes = MFnVectorArrayData(sizeObj).array();
        }

        MObject mesh = buildGarmentMesh(matrices, sizes, stat);
        CHECK_MSTATUS_AND_RETURN_IT(stat);
        MDataHandle out = builder.addElement(logical, &stat);
        CHECK_MSTATUS_AND_RETURN_IT(stat);
        out.set(mesh);
    }

    outMeshes.set(builder);
    outMeshes.setAllClean();
    data.setClean(plug);
    return MS::kSuccess;
}

} // namespace resona::maya
