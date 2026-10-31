/*
 * SPDX-License-Identifier: MIT
 *
 * PanelGeometryOverride.hpp
 *
 * PanelGeometryOverride — VP2 drawing + selection for `resonaPanel`.
 *
 * Two render items, both in the panel's LOCAL space (the panel transform
 * places them):
 *   - solid   triangles, lit stock surface shader, shaded / textured modes
 *   - outline lines, stock solid-colour shader, all display modes — drawn in
 *             Maya's selection colour when the panel is selected, so the
 *             panel is visible and pickable in wireframe too.
 *
 * Selection is object-level: picking either item selects the panel node,
 * which gives native click / marquee / shift-multi-select.
 */
#pragma once

#include <maya/MDagPath.h>
#include <maya/MFrameContext.h>
#include <maya/MHWGeometry.h>
#include <maya/MObject.h>
#include <maya/MPxGeometryOverride.h>
#include <maya/MSelectionContext.h>
#include <maya/MShaderManager.h>

namespace resona::maya
{

class PanelGeometryOverride : public MHWRender::MPxGeometryOverride
{
  public:
    static MHWRender::MPxGeometryOverride* creator(const MObject& obj);

    explicit PanelGeometryOverride(const MObject& obj);
    ~PanelGeometryOverride() override;

    PanelGeometryOverride(const PanelGeometryOverride&) = delete;
    PanelGeometryOverride& operator=(const PanelGeometryOverride&) = delete;

    MHWRender::DrawAPI supportedDrawAPIs() const override;

    // Rebuilt every frame: size changes are DG changes on the shape and the
    // selection colour is not a DG change at all.  4 vertices — negligible.
    bool requiresGeometryUpdate() const override;
    bool requiresUpdateRenderItems(const MDagPath& dagPath) const override;

    void updateDG() override;
    void updateRenderItems(const MDagPath& dagPath,
                           MHWRender::MRenderItemList& renderItems) override;
    void populateGeometry(const MHWRender::MGeometryRequirements& requirements,
                          const MHWRender::MRenderItemList& renderItems,
                          MHWRender::MGeometry& data) override;
    void cleanUp() override;

    bool refineSelectionPath(const MHWRender::MSelectionInfo& selectInfo,
                             const MHWRender::MRenderItem& hitItem, MDagPath& dagPath,
                             MObject& geomComponents, MSelectionMask& objectMask) override;
    void updateSelectionGranularity(const MDagPath& dagPath,
                                    MHWRender::MSelectionContext& selectionContext) override;

  private:
    void releaseShaders();

    MObject fShape;
    double fWidth = 50.0;
    double fHeight = 60.0;
    MHWRender::MShaderInstance* fSolidShader = nullptr;
    MHWRender::MShaderInstance* fOutlineShader = nullptr;
};

} // namespace resona::maya
