#!/usr/bin/env python3
"""Generate a deterministic difficult mesh inside Nebula's 45k-50k tier."""

from __future__ import annotations

import math
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "data/diagnostic_core/tier4_bumpy_torus.obj"
NU = 228
NV = 204


def vertex(i: int, j: int) -> tuple[float, float, float]:
    u = 2.0 * math.pi * i / NU
    v = 2.0 * math.pi * j / NV
    modulation = 1.0 + 0.10 * math.cos(7.0 * u) * math.cos(5.0 * v)
    modulation += 0.035 * math.cos(13.0 * u + 6.0 * v)
    minor = 0.205 * modulation
    major = 0.56
    radial = major + minor * math.cos(v)
    return radial * math.cos(u), radial * math.sin(u), minor * math.sin(v)


def index(i: int, j: int) -> int:
    return (i % NU) * NV + (j % NV) + 1


def main() -> None:
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    vertices = NU * NV
    faces = 2 * vertices
    with OUTPUT.open("w", encoding="ascii") as target:
        target.write(f"{vertices} {faces}\n")
        for i in range(NU):
            for j in range(NV):
                x, y, z = vertex(i, j)
                target.write(f"v {x:.10g} {y:.10g} {z:.10g}\n")
        for i in range(NU):
            for j in range(NV):
                a = index(i, j)
                b = index(i + 1, j)
                c = index(i + 1, j + 1)
                d = index(i, j + 1)
                target.write(f"f {a} {b} {c}\n")
                target.write(f"f {a} {c} {d}\n")
    print(f"generated {OUTPUT.relative_to(ROOT)}: {vertices} vertices, {faces} faces")


if __name__ == "__main__":
    main()
