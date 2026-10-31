// SPDX-License-Identifier: MIT
#include "viewport/PanelGeometryOverride.hpp"

#include "nodes/PanelShape.hpp"

#include <maya/MColor.h>
#include <maya/MFloatVector.h>
#include <maya/MHWGeometryUtilities.h>
#include <maya/MMatrix.h>
#include <maya/MPlug.h>
#include <maya/MViewport2Renderer.h>

#include <array>
#include <cstring>

namespace resona::maya
{

namespace
{

constexpr const char* kSolidItem = "resonaPanelSolid";
constexpr const char* kOutlineItem = "resonaPanelOutline";

// Spike colours: a neutral fabric tone and a dark outline.
const MColor kClothColor(0.78f, 0.62f, 0.52f);
const MColor kOutlineColor(0.08f, 0.08f, 0.08f);

constexpr std::array<unsigned int, 6> kSolidIndices = {0, 1, 2, 0, 2, 3};
constexpr std::array<unsigned int, 8> kOutlineIndices = {0, 1, 1, 2, 2, 3, 3, 0};

MHWRender::MShaderManager* shaderManager()
{
    MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer();
    return renderer != nullptr
               ? const_cast<MHWRender::MShaderManager*>(renderer->getShaderManager())
               : nullptr;
}

MHWRender::MShaderInstance* createSurfaceShader(const MHWRender::MShaderManager& mgr)
{
#if MAYA_API_VERSION >= 20260000
    return mgr.getStockShader(MHWRender::MShaderManager::k3dIsotropicOpenPBRSurfaceShader);
#else
    return mgr.getStockShader(MHWRender::MShaderManager::k3dBlinnShader);
#endif
}

void setSurfaceColor(MHWRender::MShaderInstance& shader, const MColor& color)
{
#if MAYA_API_VERSION >= 20260000
    shader.setParameter("baseColor", MFloatVector(color.r, color.g, color.b));
#else
    const float diffuse[4] = {color.r, color.g, color.b, 1.0f};
    shader.setParameter("diffuseColor", diffuse);
#endif
}

void setSolidColor(MHWRender::MShaderInstance& shader, const MColor& color)
{
    const float rgba[4] = {color.r, color.g, color.b, 1.0f};
    shader.setParameter("solidColor", rgba);
}

bool isSelected(const MDagPath& path)
{
    const MHWRender::DisplayStatus status = MHWRender::MGeometryUtilities::displayStatus(path);
    return status == MHWRender::kLead || status == MHWRender::kActive;
}

MHWRender::MRenderItem* findOrCreate(MHWRender::MRenderItemList& items, const char* name,
                                     MHWRender::MGeometry::Primitive primitive,
                                     MHWRender::MGeometry::DrawMode drawMode,
                                     unsigned int depthPriority)
{
    const int index = items.indexOf(name);
    if (index >= 0)
    {
        return items.itemAt(index);
    }

    MHWRender::MRenderItem* item = MHWRender::MRenderItem::Create(
        name, MHWRender::MRenderItem::NonMaterialSceneItem, primitive);
    if (item == nullptr)
    {
        return nullptr;
    }
    const MMatrix identity;
    item->setMatrix(&identity);
    item->setDrawMode(drawMode);
    item->depthPriority(depthPriority);
    items.append(item);
    return item;
}

template <typename T>
void fillVertexBuffer(MHWRender::MGeometry& data, const MHWRender::MVertexBufferDescriptor& desc,
                      const T& values, unsigned int numVerts, int dimension)
{
    if (desc.dataType() != MHWRender::MGeometry::kFloat || desc.dimension() != dimension)
    {
        return;
    }
    MHWRender::MVertexBuffer* buffer = data.createVertexBuffer(desc);
    if (buffer == nullptr)
    {
        return;
    }
    void* ptr = buffer->acquire(numVerts, /*writeOnly=*/true);
    if (ptr == nullptr)
    {
        return;
    }
    std::memcpy(ptr, values.data(), sizeof(float) * numVerts * dimension);
    buffer->commit(ptr);
}

template <std::size_t N>
void fillIndexBuffer(MHWRender::MGeometry& data, MHWRender::MRenderItem& item,
                     const std::array<unsigned int, N>& indices)
{
    MHWRender::MIndexBuffer* buffer = data.createIndexBuffer(MHWRender::MGeometry::kUnsignedInt32);
    if (buffer == nullptr)
    {
        return;
    }
    void* ptr = buffer->acquire(static_cast<unsigned int>(N), /*writeOnly=*/true);
    if (ptr == nullptr)
    {
        return;
    }
    std::memcpy(ptr, indices.data(), sizeof(unsigned int) * N);
    buffer->commit(ptr);
    item.associateWithIndexBuffer(buffer);
}

} // namespace

MHWRender::MPxGeometryOverride* PanelGeometryOverride::creator(const MObject& obj)
{
    return new PanelGeometryOverride(obj);
}

PanelGeometryOverride::PanelGeometryOverride(const MObject& obj)
    : MHWRender::MPxGeometryOverride(obj), fShape(obj)
{
}

PanelGeometryOverride::~PanelGeometryOverride()
{
    // Released here (Maya alive: node deleted / plugin unloaded), never from a
    // global destructor at DLL exit.
    releaseShaders();
}

void PanelGeometryOverride::releaseShaders()
{
    MHWRender::MShaderManager* mgr = shaderManager();
    if (mgr == nullptr)
    {
        return;
    }
    if (fSolidShader != nullptr)
    {
        mgr->releaseShader(fSolidShader);
    }
    if (fOutlineShader != nullptr)
    {
        mgr->releaseShader(fOutlineShader);
    }
    fSolidShader = nullptr;
    fOutlineShader = nullptr;
}

MHWRender::DrawAPI PanelGeometryOverride::supportedDrawAPIs() const
{
    return MHWRender::kOpenGL | MHWRender::kDirectX11 | MHWRender::kOpenGLCoreProfile;
}

bool PanelGeometryOverride::requiresGeometryUpdate() const
{
    return true;
}

bool PanelGeometryOverride::requiresUpdateRenderItems(const MDagPath& /*dagPath*/) const
{
    return true;
}

void PanelGeometryOverride::updateDG()
{
    fWidth = MPlug(fShape, PanelShape::aWidth).asDouble();
    fHeight = MPlug(fShape, PanelShape::aHeight).asDouble();
}

void PanelGeometryOverride::updateRenderItems(const MDagPath& dagPath,
                                              MHWRender::MRenderItemList& renderItems)
{
    using MHWRender::MGeometry;

    MHWRender::MRenderItem* solid =
        findOrCreate(renderItems, kSolidItem, MGeometry::kTriangles,
                     static_cast<MGeometry::DrawMode>(MGeometry::kShaded | MGeometry::kTextured),
                     MHWRender::MRenderItem::sDormantFilledDepthPriority);
    MHWRender::MRenderItem* outline =
        findOrCreate(renderItems, kOutlineItem, MGeometry::kLines, MGeometry::kAll,
                     MHWRender::MRenderItem::sDormantWireDepthPriority);

    if (MHWRender::MShaderManager* mgr = shaderManager())
    {
        if (fSolidShader == nullptr)
        {
            fSolidShader = createSurfaceShader(*mgr);
        }
        if (fOutlineShader == nullptr)
        {
            fOutlineShader = mgr->getStockShader(MHWRender::MShaderManager::k3dSolidShader);
        }
    }

    const bool selected = isSelected(dagPath);
    if (solid != nullptr && fSolidShader != nullptr)
    {
        setSurfaceColor(*fSolidShader, kClothColor);
        solid->setShader(fSolidShader);
        solid->enable(true);
    }
    if (outline != nullptr && fOutlineShader != nullptr)
    {
        setSolidColor(*fOutlineShader, selected
                                           ? MHWRender::MGeometryUtilities::wireframeColor(dagPath)
                                           : kOutlineColor);
        outline->setShader(fOutlineShader);
        outline->depthPriority(selected ? MHWRender::MRenderItem::sActiveWireDepthPriority
                                        : MHWRender::MRenderItem::sDormantWireDepthPriority);
        outline->enable(true);
    }
}

void PanelGeometryOverride::populateGeometry(const MHWRender::MGeometryRequirements& requirements,
                                             const MHWRender::MRenderItemList& renderItems,
                                             MHWRender::MGeometry& data)
{
    using MHWRender::MGeometry;

    const auto halfW = static_cast<float>(fWidth * 0.5);
    const auto halfH = static_cast<float>(fHeight * 0.5);
    constexpr unsigned int kNumVerts = 4;
    // Counter-clockwise seen from +Z, matching the solver's output quads.
    const std::array<float, 12> positions = {-halfW, -halfH, 0, halfW,  -halfH, 0,
                                             halfW,  halfH,  0, -halfW, halfH,  0};
    const std::array<float, 12> normals = {0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1};
    const std::array<float, 8> uvs = {0, 0, 1, 0, 1, 1, 0, 1};

    const MHWRender::MVertexBufferDescriptorList& descs = requirements.vertexRequirements();
    MHWRender::MVertexBufferDescriptor desc;
    for (int i = 0; i < descs.length(); ++i)
    {
        if (!descs.getDescriptor(i, desc))
        {
            continue;
        }
        switch (desc.semantic())
        {
        case MGeometry::kPosition:
            fillVertexBuffer(data, desc, positions, kNumVerts, 3);
            break;
        case MGeometry::kNormal:
            fillVertexBuffer(data, desc, normals, kNumVerts, 3);
            break;
        case MGeometry::kTexture:
            fillVertexBuffer(data, desc, uvs, kNumVerts, 2);
            break;
        default:
            break;
        }
    }

    for (int i = 0; i < renderItems.length(); ++i)
    {
        MHWRender::MRenderItem* item = const_cast<MHWRender::MRenderItem*>(renderItems.itemAt(i));
        if (item == nullptr)
        {
            continue;
        }
        if (item->name() == kSolidItem)
        {
            fillIndexBuffer(data, *item, kSolidIndices);
        }
        else if (item->name() == kOutlineItem)
        {
            fillIndexBuffer(data, *item, kOutlineIndices);
        }
    }
}

// Pure virtual in the base class; render items are owned by VP2 and there is
// no per-frame cache to release.
void PanelGeometryOverride::cleanUp() {}

bool PanelGeometryOverride::refineSelectionPath(const MHWRender::MSelectionInfo& /*selectInfo*/,
                                                const MHWRender::MRenderItem& /*hitItem*/,
                                                MDagPath& dagPath, MObject& /*geomComponents*/,
                                                MSelectionMask& /*objectMask*/)
{
    MDagPath shapePath;
    if (!MDagPath::getAPathTo(fShape, shapePath))
    {
        return false;
    }
    dagPath = shapePath;
    return true;
}

void PanelGeometryOverride::updateSelectionGranularity(
    const MDagPath& /*dagPath*/, MHWRender::MSelectionContext& selectionContext)
{
    selectionContext.setSelectionLevel(MHWRender::MSelectionContext::kObject);
}

} // namespace resona::maya
