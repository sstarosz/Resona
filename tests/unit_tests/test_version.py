# SPDX-License-Identifier: MIT
"""Smoke test — the generated version module is importable (run after CMake configure)."""

import pytest


def test_version_module() -> None:
    version = pytest.importorskip("resona._version")
    assert isinstance(version.__version__, str)
    assert version.__version__
