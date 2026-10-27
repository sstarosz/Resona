# Resona

*Working name.* Non-destructive, pattern-based cloth creation for Autodesk Maya —
a Marvelous Designer–style workflow (2D patterns → seams → drape) built on Maya's
dependency graph, so measurements can be changed at any point and the garment
re-simulates.

> Status: scaffold only — the plugin loads and unloads, no functionality yet.

## Layout

```
resona/
  Resona.cpp        C++ plugin entry point (.mll) — registers C++ nodes, then calls Python
  plugin.py         Python side: Python nodes/commands, UI
  core/             Maya-free C++ core (pattern, meshing, seams, solver) → resona_core static lib
  maya/nodes/       Maya node wrappers (C++ / Python)
  maya/cmds/        Maya commands
  ui/               PySide6 UI (pattern editor, panels)
  icons/
cmake/              FindMaya, GitVersion, version / .mod templates
scripts/ci/         Maya SDK download + strip (strip_maya_sdk.py)
tests/unit_tests/   Catch2 (core) + pytest
tests/integration/  mayapy load/unload smoke test
```

## Build

Requires CMake ≥ 3.21, Visual Studio 2022 (Windows), Python ≥ 3.10 and
`VCPKG_ROOT` pointing to a bootstrapped vcpkg.

```powershell
cmake --preset maya2026-debug
cmake --build --preset maya2026-debug
```

The Maya SDK is downloaded into `out/.sdk/` on first configure. To reuse an
existing one (e.g. MayaMMD's cache):

```powershell
cmake --preset maya2026-debug -DSDK_DIR=C:/Users/Sebastian/Desktop/MayaMMD/out/.sdk/sdk-maya2026
```

Each build copies `Resona.mll` to `out/install/<preset>/plug-ins/` and deploys
`Resona.mod` to `Documents/maya/<version>/modules/`, so Maya finds it on next start:

```python
cmds.loadPlugin("Resona")
```

### Core only (no Maya SDK)

```powershell
cmake --preset core-only
cmake --build --preset core-only
ctest --preset core-only
```

## License

MIT — see [LICENSE](LICENSE).
