// SPDX-License-Identifier: MIT
#include "nodes/PanelShape.hpp"

#include "nodes/TypeIds.hpp"

#include <maya/MFnNumericAttribute.h>
#include <maya/MPlug.h>
#include <maya/MPoint.h>

namespace resona::maya
{

const MTypeId PanelShape::kTypeId(type_ids::kPanel);

MObject PanelShape::aWidth;
MObject PanelShape::aHeight;

void* PanelShape::creator()
{
    return new PanelShape();
}

MStatus PanelShape::initialize()
{
    MStatus stat;
    MFnNumericAttribute nAttr;

    aWidth = nAttr.create("width", "rsW", MFnNumericData::kDouble, 50.0, &stat);
    CHECK_MSTATUS_AND_RETURN_IT(stat);
    nAttr.setMin(0.1);
    nAttr.setSoftMax(200.0);
    nAttr.setKeyable(true);
    CHECK_MSTATUS_AND_RETURN_IT(addAttribute(aWidth));

    aHeight = nAttr.create("height", "rsH", MFnNumericData::kDouble, 60.0, &stat);
    CHECK_MSTATUS_AND_RETURN_IT(stat);
    nAttr.setMin(0.1);
    nAttr.setSoftMax(200.0);
    nAttr.setKeyable(true);
    CHECK_MSTATUS_AND_RETURN_IT(addAttribute(aHeight));

    return MS::kSuccess;
}

bool PanelShape::isBounded() const
{
    return true;
}

MBoundingBox PanelShape::boundingBox() const
{
    const MObject self = thisMObject();
    const double halfW = MPlug(self, aWidth).asDouble() * 0.5;
    const double halfH = MPlug(self, aHeight).asDouble() * 0.5;
    constexpr double kThickness = 0.01; // keep the box non-degenerate for framing / culling
    return {MPoint(-halfW, -halfH, -kThickness), MPoint(halfW, halfH, kThickness)};
}

} // namespace resona::maya
