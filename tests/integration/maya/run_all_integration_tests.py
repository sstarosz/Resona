# SPDX-License-Identifier: MIT
"""Maya integration tests — run with mayapy (registered in CTest).

    mayapy tests/integration/maya/run_all_integration_tests.py [suite ...]

Suites: load, nodeset (default: all).
"""

from __future__ import annotations

import os
import sys
import tempfile
import traceback
from collections.abc import Callable

TOLERANCE = 1e-3


def _approx(a: float, b: float) -> bool:
    return abs(a - b) <= TOLERANCE


def test_load() -> None:
    from maya import cmds

    cmds.file(new=True, force=True)
    cmds.loadPlugin("Resona", quiet=True)
    assert cmds.pluginInfo("Resona", query=True, loaded=True), "Resona did not load"
    for node_type in ("resonaSolver", "resonaGarment", "resonaPanel"):
        assert node_type in (cmds.pluginInfo("Resona", query=True, dependNode=True) or []), node_type
    cmds.unloadPlugin("Resona")
    cmds.loadPlugin("Resona", quiet=True)


def test_nodeset() -> None:
    """M0 spike: panels drive the solver's output mesh through the DG."""
    from maya import cmds

    from resona.spikes import node_set

    cmds.file(new=True, force=True)
    cmds.undoInfo(state=True, infinity=True)
    nodes = node_set.build()
    mesh = nodes.output_mesh
    front, back = list(nodes.panels)

    assert cmds.polyEvaluate(mesh, vertex=True) == 8, "expected 2 quads"
    assert cmds.polyEvaluate(mesh, face=True) == 2
    assert cmds.polyEvaluate(mesh, uv=True) == 8, "expected per-panel UVs"
    before = cmds.exactWorldBoundingBox(mesh)

    # Multi-select move with Maya's own tool command.
    cmds.select(front, back, replace=True)
    cmds.move(0, 10, 0, relative=True)
    after = cmds.exactWorldBoundingBox(mesh)
    assert _approx(after[1] - before[1], 10.0), f"mesh did not follow panels: {before} -> {after}"

    # Undo of the move restores the mesh.
    cmds.undo()
    assert _approx(cmds.exactWorldBoundingBox(mesh)[1], before[1]), "undo did not restore"

    # Rotating one panel only moves its quad.
    cmds.rotate(0, 90, 0, front, relative=True, objectSpace=True)
    rotated = cmds.exactWorldBoundingBox(mesh)
    # front spans X (±25) at z=15; rotated 90° it spans Z instead → zmax grows.
    assert rotated[5] > before[5] + 10.0, f"rotation not reflected: {before} -> {rotated}"

    # Panel attribute (stand-in for the pattern) drives the output too.
    front_shape = nodes.panels[front]
    cmds.setAttr(f"{front_shape}.height", 100.0)
    tall = cmds.exactWorldBoundingBox(mesh)
    assert tall[4] - tall[1] > before[4] - before[1] + 10.0, "height change not reflected"

    # Moving the whole solver hierarchy moves everything.
    cmds.move(5, 0, 0, "resonaSolver1", relative=True)

    # Save / reopen keeps the connections and the result.
    path = os.path.join(tempfile.mkdtemp(prefix="resona-"), "nodeset.ma")
    expected = cmds.exactWorldBoundingBox(mesh)
    cmds.file(rename=path)
    cmds.file(save=True, type="mayaAscii", force=True)
    cmds.file(path, open=True, force=True)
    reopened = cmds.exactWorldBoundingBox("skirt_meshShape")
    assert all(_approx(a, b) for a, b in zip(expected, reopened)), f"{expected} != {reopened}"
    assert cmds.polyEvaluate("skirt_meshShape", vertex=True) == 8


SUITES: dict[str, Callable[[], None]] = {
    "load": test_load,
    "nodeset": test_nodeset,
}


def main(argv: list[str]) -> int:
    import maya.standalone

    names = [a for a in argv if a != "all"] or list(SUITES)
    unknown = [n for n in names if n not in SUITES]
    if unknown:
        print(f"[FAIL] unknown suites: {unknown}; available: {list(SUITES)}")
        return 2

    maya.standalone.initialize(name="python")
    failed = 0
    try:
        from maya import cmds

        cmds.loadPlugin("Resona", quiet=True)
        print(f"Resona {cmds.pluginInfo('Resona', query=True, version=True)}")
        for name in names:
            try:
                SUITES[name]()
                print(f"[OK]   {name}")
            except Exception:  # noqa: BLE001 - report every suite
                failed += 1
                print(f"[FAIL] {name}")
                traceback.print_exc()
    finally:
        maya.standalone.uninitialize()
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
