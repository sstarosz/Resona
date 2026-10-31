/*
 * SPDX-License-Identifier: MIT
 *
 * Resona.cpp
 *
 * Resona — Maya C++ plugin entry point (.mll).
 *
 * initializePlugin does TWO things on the same plugin handle:
 *   1. Registers native C++ nodes/commands via MFnPlugin
 *   2. Calls Python (via MGlobal::executePythonCommand) to register
 *      Python-based nodes/commands and set up the UI.
 *      Python entry point is resona.plugin.initializePlugin()
 *
 * Everything registers under the same "Resona" identity —
 * one Plugin Manager entry.
 */

#include <maya/MDrawRegistry.h>
#include <maya/MFnPlugin.h>
#include <maya/MGlobal.h>
#include <maya/MObject.h>
#include <maya/MStatus.h>
#include <maya/MString.h>

#include "nodes/GarmentShape.hpp"
#include "nodes/PanelShape.hpp"
#include "nodes/SolverShape.hpp"
#include "version.hpp"
#include "viewport/PanelGeometryOverride.hpp"

namespace
{

constexpr const char* kDrawRegistrantId = "Resona";

// importlib.reload ensures plugin.py source changes take effect when the
// .mll is reloaded without restarting Maya.  The project root is injected
// into sys.path so Python can locate the resona/ package (PYTHONPATH is NOT
// set globally — that would load Maya-API imports too early).
MStatus run_python_initialization()
{
    const MStatus stat =
        MGlobal::executePythonCommand("import sys; "
                                      "sys.path.insert(0, '" PROJECT_ROOT_DIR "'); "
                                      "import importlib, resona.plugin; "
                                      "importlib.reload(resona.plugin); "
                                      "resona.plugin.initializePlugin()");
    if (!stat)
    {
        MGlobal::displayWarning("Resona: Python initialization failed — "
                                "Python components unavailable");
    }
    return stat;
}

void run_python_uninitialization()
{
    MGlobal::executePythonCommand("import sys; "
                                  "sys.path.insert(0, '" PROJECT_ROOT_DIR "'); "
                                  "import resona.plugin; "
                                  "resona.plugin.uninitializePlugin()");
}

void warnOnFailure(const MStatus& stat, const char* what)
{
    if (!stat)
    {
        MGlobal::displayWarning(MString("Resona: ") + what + " registration failed");
    }
}

} // namespace

// ===========================================================================
// Plugin entry points
// ===========================================================================
PLUGIN_EXPORT MStatus initializePlugin(MObject mobject)
{
    using resona::maya::GarmentShape;
    using resona::maya::PanelGeometryOverride;
    using resona::maya::PanelShape;
    using resona::maya::SolverShape;

    MStatus stat;
    MFnPlugin plugin(mobject, "Sebastian Starosz", PROJECT_VERSION, "Any", &stat);
    // NOLINTNEXTLINE(performance-unnecessary-copy-initialization)
    CHECK_MSTATUS_AND_RETURN_IT(stat);

    // 1. Native nodes (solver ← garments ← panels, nucleus-style).
    warnOnFailure(plugin.registerNode(SolverShape::kNodeName, SolverShape::kTypeId,
                                      SolverShape::creator, SolverShape::initialize,
                                      MPxNode::kLocatorNode),
                  SolverShape::kNodeName);
    warnOnFailure(plugin.registerNode(GarmentShape::kNodeName, GarmentShape::kTypeId,
                                      GarmentShape::creator, GarmentShape::initialize,
                                      MPxNode::kLocatorNode),
                  GarmentShape::kNodeName);
    {
        const MString classification(PanelShape::kNodeClassify);
        warnOnFailure(plugin.registerNode(PanelShape::kNodeName, PanelShape::kTypeId,
                                          PanelShape::creator, PanelShape::initialize,
                                          MPxNode::kLocatorNode, &classification),
                      PanelShape::kNodeName);
    }

    // 2. Viewport (MDrawRegistry, deregistered separately).
    warnOnFailure(MHWRender::MDrawRegistry::registerGeometryOverrideCreator(
                      PanelShape::kNodeClassify, kDrawRegistrantId, PanelGeometryOverride::creator),
                  "resonaPanel geometry override");

    // 3. Python side (nodes, commands, UI).
    run_python_initialization();

    return MS::kSuccess;
}

PLUGIN_EXPORT MStatus uninitializePlugin(MObject mobject)
{
    using resona::maya::GarmentShape;
    using resona::maya::PanelShape;
    using resona::maya::SolverShape;

    MFnPlugin plugin(mobject);

    // Reverse order of registration (LIFO).
    run_python_uninitialization();

    MHWRender::MDrawRegistry::deregisterGeometryOverrideCreator(PanelShape::kNodeClassify,
                                                                kDrawRegistrantId);

    plugin.deregisterNode(PanelShape::kTypeId);
    plugin.deregisterNode(GarmentShape::kTypeId);
    plugin.deregisterNode(SolverShape::kTypeId);

    return MS::kSuccess;
}
