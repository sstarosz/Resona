// SPDX-License-Identifier: MIT
#include "nodes/GarmentShape.hpp"

#include "nodes/TypeIds.hpp"

#include <maya/MArrayDataHandle.h>
#include <maya/MFnCompoundAttribute.h>
#include <maya/MFnMatrixArrayData.h>
#include <maya/MFnMatrixAttribute.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnVectorArrayData.h>
#include <maya/MMatrixArray.h>
#include <maya/MVectorArray.h>

namespace resona::maya
{

const MTypeId GarmentShape::kTypeId(type_ids::kGarment);

MObject GarmentShape::aPanels;
MObject GarmentShape::aPanelMatrix;
MObject GarmentShape::aPanelWidth;
MObject GarmentShape::aPanelHeight;
MObject GarmentShape::aOutPanelMatrices;
MObject GarmentShape::aOutPanelSizes;

void* GarmentShape::creator()
{
    return new GarmentShape();
}

MStatus GarmentShape::initialize()
{
    MStatus stat;
    MFnMatrixAttribute mAttr;
    MFnNumericAttribute nAttr;
    MFnCompoundAttribute cAttr;
    MFnTypedAttribute tAttr;

    aPanelMatrix = mAttr.create("panelMatrix", "rsPm", MFnMatrixAttribute::kDouble, &stat);
    CHECK_MSTATUS_AND_RETURN_IT(stat);
    mAttr.setHidden(true);

    aPanelWidth = nAttr.create("panelWidth", "rsPw", MFnNumericData::kDouble, 50.0, &stat);
    CHECK_MSTATUS_AND_RETURN_IT(stat);
    nAttr.setHidden(true);

    aPanelHeight = nAttr.create("panelHeight", "rsPh", MFnNumericData::kDouble, 60.0, &stat);
    CHECK_MSTATUS_AND_RETURN_IT(stat);
    nAttr.setHidden(true);

    aPanels = cAttr.create("panels", "rsPnl", &stat);
    CHECK_MSTATUS_AND_RETURN_IT(stat);
    cAttr.addChild(aPanelMatrix);
    cAttr.addChild(aPanelWidth);
    cAttr.addChild(aPanelHeight);
    cAttr.setArray(true);
    cAttr.setIndexMatters(false); // connectAttr -nextAvailable works
    cAttr.setHidden(true);
    CHECK_MSTATUS_AND_RETURN_IT(addAttribute(aPanels));

    aOutPanelMatrices = tAttr.create("outPanelMatrices", "rsOpm", MFnData::kMatrixArray,
                                     MFnMatrixArrayData().create(), &stat);
    CHECK_MSTATUS_AND_RETURN_IT(stat);
    tAttr.setWritable(false);
    tAttr.setStorable(false);
    tAttr.setHidden(true);
    CHECK_MSTATUS_AND_RETURN_IT(addAttribute(aOutPanelMatrices));

    aOutPanelSizes = tAttr.create("outPanelSizes", "rsOps", MFnData::kVectorArray,
                                  MFnVectorArrayData().create(), &stat);
    CHECK_MSTATUS_AND_RETURN_IT(stat);
    tAttr.setWritable(false);
    tAttr.setStorable(false);
    tAttr.setHidden(true);
    CHECK_MSTATUS_AND_RETURN_IT(addAttribute(aOutPanelSizes));

    for (const MObject& in : {aPanels, aPanelMatrix, aPanelWidth, aPanelHeight})
    {
        attributeAffects(in, aOutPanelMatrices);
        attributeAffects(in, aOutPanelSizes);
    }
    return MS::kSuccess;
}

bool GarmentShape::isBounded() const
{
    return false;
}

MStatus GarmentShape::compute(const MPlug& plug, MDataBlock& data)
{
    if (plug != aOutPanelMatrices && plug != aOutPanelSizes)
    {
        return MS::kUnknownParameter;
    }

    MStatus stat;
    MArrayDataHandle panels = data.inputArrayValue(aPanels, &stat);
    CHECK_MSTATUS_AND_RETURN_IT(stat);

    MMatrixArray matrices;
    MVectorArray sizes;
    const unsigned int count = panels.elementCount();
    for (unsigned int i = 0; i < count; ++i)
    {
        panels.jumpToArrayElement(i);
        MDataHandle panel = panels.inputValue();
        matrices.append(panel.child(aPanelMatrix).asMatrix());
        sizes.append(MVector(panel.child(aPanelWidth).asDouble(),
                             panel.child(aPanelHeight).asDouble(), 0.0));
    }

    MFnMatrixArrayData matrixData;
    MObject matrixObj = matrixData.create(matrices, &stat);
    CHECK_MSTATUS_AND_RETURN_IT(stat);
    MFnVectorArrayData sizeData;
    MObject sizeObj = sizeData.create(sizes, &stat);
    CHECK_MSTATUS_AND_RETURN_IT(stat);

    MDataHandle outMatrices = data.outputValue(aOutPanelMatrices);
    outMatrices.set(matrixObj);
    outMatrices.setClean();
    MDataHandle outSizes = data.outputValue(aOutPanelSizes);
    outSizes.set(sizeObj);
    outSizes.setClean();
    return MS::kSuccess;
}

} // namespace resona::maya
