#!/usr/bin/env python3
"""Generate closed UV-triangulated diagnostic meshes in the IMC text format.

The generator streams output, so the T7 preset does not require holding two
million faces in Python memory.
"""
from __future__ import annotations

import argparse
import math
from pathlib import Path

PRESETS = {
    # V = segments * rings + 2, F = 2 * segments * rings.
    "t2": (128, 80),          # 10,242 V / 20,480 F
    "t3": (250, 160),         # 40,002 V / 80,000 F
    "t4": (240, 200),         # 48,002 V / 96,000 F
    "t6": (800, 500),         # 400,002 V / 800,000 F
    "t7": (1024, 1000),       # 1,024,002 V / 2,048,000 F; triggers >=1M branch
}


def deform(x: float, y: float, z: float, shape: str) -> tuple[float, float, float]:
    if shape == "sphere":
        return x, y, z
    if shape == "ellipsoid":
        return x, 0.68 * y, 0.43 * z
    if shape == "peanut":
        s = 0.62 + 0.42 * abs(z)
        return s * x, s * y, 1.05 * z
    theta = math.atan2(y, x)
    phi = math.acos(max(-1.0, min(1.0, z)))
    if shape == "wavy":
        r = 1.0 + 0.065 * math.sin(14.0 * theta) * math.sin(9.0 * phi)
        return r * x, r * y, r * z
    if shape == "bumpy":
        r = 1.0 + 0.055 * math.sin(7.0 * theta) * math.sin(5.0 * phi) + 0.035 * math.cos(11.0 * theta + 2.0 * phi)
        return r * x, r * y, r * z
    raise ValueError(shape)


def vertex_index(ring: int, segment: int, segments: int) -> int:
    """Return one-indexed vertex index for an interior ring."""
    return 2 + ring * segments + (segment % segments)


def write_mesh(path: Path, segments: int, rings: int, shape: str) -> None:
    if segments < 3 or rings < 1:
        raise ValueError("segments >= 3 and rings >= 1 are required")
    nv = segments * rings + 2
    nf = 2 * segments * rings
    north = 1
    south = nv
    with path.open("w", encoding="utf-8", buffering=1024 * 1024) as f:
        f.write(f"{nv} {nf}\n")
        for p in ((0.0, 0.0, 1.0),):
            x, y, z = deform(*p, shape)
            f.write(f"v {x:.12g} {y:.12g} {z:.12g}\n")
        for i in range(rings):
            phi = math.pi * (i + 1) / (rings + 1)
            sp, cp = math.sin(phi), math.cos(phi)
            for j in range(segments):
                theta = 2.0 * math.pi * j / segments
                x, y, z = sp * math.cos(theta), sp * math.sin(theta), cp
                x, y, z = deform(x, y, z, shape)
                f.write(f"v {x:.12g} {y:.12g} {z:.12g}\n")
        x, y, z = deform(0.0, 0.0, -1.0, shape)
        f.write(f"v {x:.12g} {y:.12g} {z:.12g}\n")

        # Top cap. Orientation is outward.
        for j in range(segments):
            a = vertex_index(0, j, segments)
            b = vertex_index(0, j + 1, segments)
            f.write(f"f {north} {b} {a}\n")
        # Interior bands.
        for i in range(rings - 1):
            for j in range(segments):
                a = vertex_index(i, j, segments)
                b = vertex_index(i, j + 1, segments)
                c = vertex_index(i + 1, j, segments)
                d = vertex_index(i + 1, j + 1, segments)
                f.write(f"f {a} {b} {c}\n")
                f.write(f"f {b} {d} {c}\n")
        # Bottom cap.
        last = rings - 1
        for j in range(segments):
            a = vertex_index(last, j, segments)
            b = vertex_index(last, j + 1, segments)
            f.write(f"f {a} {b} {south}\n")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("output", type=Path)
    parser.add_argument("--preset", choices=sorted(PRESETS), default="t2")
    parser.add_argument("--segments", type=int)
    parser.add_argument("--rings", type=int)
    parser.add_argument("--shape", choices=["sphere", "ellipsoid", "peanut", "wavy", "bumpy"], default="sphere")
    args = parser.parse_args()
    segments, rings = PRESETS[args.preset]
    if args.segments is not None:
        segments = args.segments
    if args.rings is not None:
        rings = args.rings
    args.output.parent.mkdir(parents=True, exist_ok=True)
    write_mesh(args.output, segments, rings, args.shape)
    print(f"wrote {args.output}: V={segments * rings + 2}, F={2 * segments * rings}, shape={args.shape}")


if __name__ == "__main__":
    main()
