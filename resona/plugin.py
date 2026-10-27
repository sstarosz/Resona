# SPDX-License-Identifier: MIT
"""
Resona — Maya plugin implementation (Python side).

Loaded by the C++ native entry point ``Resona.mll``, which calls
:func:`initializePlugin` after registering its own C++ nodes/commands.
Everything registers under the same "Resona" plugin identity.
"""

from __future__ import annotations

import importlib
import logging
import os
import sys
import traceback

import maya.api.OpenMaya as om

# Dev mode — enabled when running from source. Disable with RESONA_DEV=0.
_DEV_MODE = os.environ.get("RESONA_DEV", "1") == "1"

# ---------------------------------------------------------------------------
# Plugin metadata
# ---------------------------------------------------------------------------
try:
    from resona._version import __version__ as _resona_version

    PLUGIN_VERSION = _resona_version
except ImportError:
    PLUGIN_VERSION = "0.0.0+dev"

PLUGIN_NAME = "Resona"
PLUGIN_AUTHOR = "Sebastian Starosz"

log = logging.getLogger(PLUGIN_NAME)

# Python modules (re)loaded on every plugin load, so source edits take effect
# after an unload/load cycle without restarting Maya.  Add new modules here.
_RESONA_MODULES: list[str] = []


def _reload_modules() -> int:
    """Import (or reload) all ``resona.*`` modules. Returns the success count."""
    headless = os.environ.get("QT_QPA_PLATFORM") == "offscreen"
    success = 0
    for module_name in _RESONA_MODULES:
        if headless and module_name.startswith("resona.ui."):
            success += 1  # UI is never built headless — count as OK
            continue
        try:
            if module_name in sys.modules:
                importlib.reload(sys.modules[module_name])
            else:
                importlib.import_module(module_name)
            success += 1
        except Exception as e:  # noqa: BLE001 - report and keep loading
            log.error("  %-45s FAILED — %s", module_name, e)
    return success


# ---------------------------------------------------------------------------
# Entry points — called from the C++ .mll via MGlobal::executePythonCommand
# ---------------------------------------------------------------------------
def initializePlugin() -> None:
    """Called by ``Resona.mll`` after native C++ registration."""
    logging.getLogger().setLevel(logging.INFO)
    if _DEV_MODE:
        log.info("Resona v%s (DEV MODE)", PLUGIN_VERSION)

    try:
        loaded = _reload_modules()
        if loaded < len(_RESONA_MODULES):
            log.warning("%d/%d modules loaded", loaded, len(_RESONA_MODULES))

        # Same plugin MObject the C++ side registered → one Plugin Manager entry.
        mobject = om.MFnPlugin.findPlugin(PLUGIN_NAME)
        if mobject.isNull():
            raise RuntimeError(f"Could not find plugin '{PLUGIN_NAME}' via findPlugin")
        _mplugin = om.MFnPlugin(mobject, PLUGIN_AUTHOR, PLUGIN_VERSION, "Any")
        # Python nodes / commands get registered on _mplugin here.

        print(f"[OK] Resona v{PLUGIN_VERSION} ready")
    except Exception:
        log.error("Failed to initialize %s", PLUGIN_NAME)
        traceback.print_exc()
        print("[FAIL] Resona failed to load — see Script Editor for details.")


def uninitializePlugin() -> None:
    """Called by ``Resona.mll`` before native C++ deregistration."""
    try:
        mobject = om.MFnPlugin.findPlugin(PLUGIN_NAME)
        if not mobject.isNull():
            _mplugin = om.MFnPlugin(mobject)
            # Python nodes / commands get deregistered on _mplugin here.
        print(f"  Resona v{PLUGIN_VERSION} unloaded.")
    except Exception:
        log.error("Failed to unload %s", PLUGIN_NAME)
        traceback.print_exc()
