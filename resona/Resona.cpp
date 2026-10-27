/*
 * SPDX-License-Identifier: MIT
 *
 * Resona.cpp
 *
 * Resona — Maya C++ plugin entry point (.mll).
 *
 * initializePlugin does TWO things on the same plugin handle:
 *   1. Registers native C++ nodes/commands via MFnPlugin (none yet)
 *   2. Calls Python (via MGlobal::executePythonCommand) to register
 *      Python-based nodes/commands and set up the UI.
 *      Python entry point is resona.plugin.initializePlugin()
 *
 * Everything registers under the same "Resona" identity —
 * one Plugin Manager entry.
 */

#include <maya/MFnPlugin.h>
#include <maya/MGlobal.h>
#include <maya/MObject.h>
#include <maya/MStatus.h>

#include "version.hpp"

namespace
{

// importlib.reload ensures plugin.py source changes take effect when the
// .mll is reloaded without restarting Maya.  The project root is injected
// into sys.path so Python can locate the resona/ package (PYTHONPATH is NOT
// set globally — that would load Maya-API imports too early).
MStatus run_python_initialization()
{
    const MStatus stat = MGlobal::executePythonCommand("import sys; "
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

} // namespace

// ===========================================================================
// Plugin entry points
// ===========================================================================
PLUGIN_EXPORT MStatus initializePlugin(MObject mobject)
{
    MStatus stat;
    MFnPlugin plugin(mobject, "Sebastian Starosz", PROJECT_VERSION, "Any", &stat);
    // NOLINTNEXTLINE(performance-unnecessary-copy-initialization)
    CHECK_MSTATUS_AND_RETURN_IT(stat);

    // 1. Native C++ nodes / commands — registered here as they are added.

    // 2. Python side (nodes, commands, UI).
    run_python_initialization();

    return MS::kSuccess;
}

PLUGIN_EXPORT MStatus uninitializePlugin(MObject mobject)
{
    MFnPlugin plugin(mobject);

    // 1. Deregister Python components first (LIFO).
    run_python_uninitialization();

    // 2. Deregister native C++ nodes / commands (none yet).

    return MS::kSuccess;
}
