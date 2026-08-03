#!/usr/bin/env python3
"""Procedurally generate missing tier-coverage fixtures.

Outputs (under data/synth_bench/):

  tier2_bumpy_hard.obj  - 10k vertices, aggressive high-frequency bumps
  tier5_bumpy.obj       - 500k vertices, moderate-frequency bumps
  tier4_boundary.obj    - 48k vertices, exercises the 45k-50k solver band

The synth_bench tier3_bumpy_hard.obj and friends already cover tiers 3 and 4;
this script adds a tier-2 high-detail mesh and a tier-5 stress mesh so every
Kattis tier has at least one local fixture.
"""
from __future__ import annotations

import math
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT_DIR = ROOT / "data/synth_bench"


def write_sphere_with_bumps(path: Path, nu: int, nv: int,
                            base_freq: float, amp_freq: float,
                            bump_amp: float) -> None:
    """Generate a UV sphere with sinusoidal radial bumps."""
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    verts: list[tuple[float, float, float]] = []
    for i in range(nu):
        u = 2.0 * math.pi * i / nu
        for j in range(nv):
            v = math.pi * (j + 0.5) / nv  # avoid poles
            bump = (1.0
                    + bump_amp * (0.5 * math.cos(base_freq * u) * math.cos(amp_freq * v)
                                  + 0.25 * math.sin(2.0 * base_freq * u + 3.0 * v)
                                  + 0.15 * math.cos(5.0 * u - 4.0 * v)))
            r = 0.7 * bump
            x = r * math.sin(v) * math.cos(u)
            y = r * math.sin(v) * math.sin(u)
            z = r * math.cos(v)
            verts.append((x, y, z))
    faces = []
    for i in range(nu):
        for j in range(nv):
            jp = (j + 1) % nv
            a = i * nv + j + 1
            b = ((i + 1) % nu) * nv + j + 1
            c = ((i + 1) % nu) * nv + jp + 1
            d = i * nv + jp + 1
            faces.append((a, b, c))
            faces.append((a, c, d))
    with path.open("w", encoding="ascii") as f:
        f.write(f"{len(verts)} {len(faces)}\n")
        for v in verts:
            f.write(f"v {v[0]:.10g} {v[1]:.10g} {v[2]:.10g}\n")
        for fa in faces:
            f.write(f"f {fa[0]} {fa[1]} {fa[2]}\n")
    print(f"generated {path.relative_to(ROOT)}: {len(verts)} verts, {len(faces)} faces")


def main() -> None:
    # tier2_bumpy_hard: ~10k verts, high-frequency detail
    write_sphere_with_bumps(
        OUT_DIR / "tier2_bumpy_hard.obj",
        nu=100, nv=100,
        base_freq=14.0, amp_freq=11.0,
        bump_amp=0.18,
    )
    # tier5_bumpy: ~500k verts, moderate detail
    write_sphere_with_bumps(
        OUT_DIR / "tier5_bumpy.obj",
        nu=708, nv=708,
        base_freq=18.0, amp_freq=14.0,
        bump_amp=0.12,
    )
    write_sphere_with_bumps(
        OUT_DIR / "tier4_boundary.obj",
        nu=240, nv=200,
        base_freq=16.0, amp_freq=13.0,
        bump_amp=0.14,
    )


if __name__ == "__main__":
    main()