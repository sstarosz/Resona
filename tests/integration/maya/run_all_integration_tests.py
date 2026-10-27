# SPDX-License-Identifier: MIT
"""Maya integration smoke test — run with mayapy (registered in CTest).

Loads Resona.mll in maya.standalone and checks it loads and unloads cleanly.
"""

from __future__ import annotations

import sys


def main() -> int:
    import maya.standalone

    maya.standalone.initialize(name="python")
    try:
        from maya import cmds

        cmds.loadPlugin("Resona")
        if not cmds.pluginInfo("Resona", query=True, loaded=True):
            print("[FAIL] Resona did not load")
            return 1
        print(f"[OK] Resona {cmds.pluginInfo('Resona', query=True, version=True)} loaded")
        cmds.unloadPlugin("Resona")
        print("[OK] Resona unloaded")
        return 0
    finally:
        maya.standalone.uninitialize()


if __name__ == "__main__":
    sys.exit(main())
