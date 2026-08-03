#!/usr/bin/env python3
"""Generate deterministic, connected, closed hard meshes near the IMC2026 T3 range.

These are diagnostics, not hidden-test replicas. The cases stress broad curvature,
local spikes, ridges, dimples, narrow corrugation, and globally folded tubes.
"""
from __future__ import annotations

import argparse
import math
from pathlib import Path
from typing import Callable

from mesh_validate import Mesh, validate

Vec3 = tuple[float, float, float]
Face = tuple[int, int, int]


def dot(a: Vec3, b: Vec3) -> float:
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


def cross(a: Vec3, b: Vec3) -> Vec3:
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])


def norm(a: Vec3) -> float:
    return math.sqrt(dot(a, a))


def add(a: Vec3, b: Vec3) -> Vec3:
    return a[0] + b[0], a[1] + b[1], a[2] + b[2]


def mul(a: Vec3, s: float) -> Vec3:
    return a[0] * s, a[1] * s, a[2] * s


def unit(a: Vec3) -> Vec3:
    n = norm(a)
    if n <= 1e-15:
        raise ValueError("zero vector")
    return mul(a, 1.0 / n)


def normalize(mesh: Mesh, radius: float = 0.95) -> Mesh:
    scale = radius / max(norm(v) for v in mesh.vertices)
    return Mesh([mul(v, scale) for v in mesh.vertices], mesh.faces)


def write(mesh: Mesh, path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w") as f:
        f.write(f"{len(mesh.vertices)} {len(mesh.faces)}\n")
        for x, y, z in mesh.vertices:
            f.write(f"v {x:.12g} {y:.12g} {z:.12g}\n")
        for a, b, c in mesh.faces:
            f.write(f"f {a + 1} {b + 1} {c + 1}\n")


def sphere_mesh(rings: int, segments: int, deform: Callable[[float, float], Vec3]) -> Mesh:
    vertices: list[Vec3] = [(0.0, 0.0, 1.0)]
    for i in range(1, rings + 1):
        theta = math.pi * i / (rings + 1)
        for j in range(segments):
            phi = 2 * math.pi * j / segments
            vertices.append(deform(theta, phi))
    south = len(vertices)
    vertices.append((0.0, 0.0, -1.0))
    faces: list[Face] = []
    for j in range(segments):
        faces.append((0, 1 + (j + 1) % segments, 1 + j))
    for i in range(rings - 1):
        a0 = 1 + i * segments
        b0 = a0 + segments
        for j in range(segments):
            k = (j + 1) % segments
            a, b, c, d = a0 + j, a0 + k, b0 + k, b0 + j
            faces.extend(((a, b, c), (a, c, d)))
    last = 1 + (rings - 1) * segments
    for j in range(segments):
        faces.append((last + j, last + (j + 1) % segments, south))
    return normalize(Mesh(vertices, faces))


def periodic_faces(nu: int, nv: int) -> list[Face]:
    faces: list[Face] = []
    for i in range(nu):
        ii = (i + 1) % nu
        for j in range(nv):
            jj = (j + 1) % nv
            a, b, c, d = i * nv + j, ii * nv + j, ii * nv + jj, i * nv + jj
            faces.extend(((a, b, c), (a, c, d)))
    return faces


def knot(nu: int, nv: int) -> Mesh:
    vertices: list[Vec3] = []
    p, q = 2.0, 3.0
    for i in range(nu):
        t = 2 * math.pi * i / nu
        radial = 0.62 + 0.18 * math.cos(q * t)
        center = (radial * math.cos(p * t), radial * math.sin(p * t), 0.18 * math.sin(q * t))
        tangent = unit((
            -p * radial * math.sin(p * t) - q * 0.18 * math.sin(q * t) * math.cos(p * t),
            p * radial * math.cos(p * t) - q * 0.18 * math.sin(q * t) * math.sin(p * t),
            q * 0.18 * math.cos(q * t),
        ))
        ref = (0.0, 0.0, 1.0) if abs(tangent[2]) < 0.95 else (0.0, 1.0, 0.0)
        n = unit(cross(ref, tangent))
        b = unit(cross(tangent, n))
        spike = max(0.0, math.cos(11 * t)) ** 8
        tube = 0.072 * (1 + 0.60 * spike)
        for j in range(nv):
            a = 2 * math.pi * j / nv
            off = add(mul(n, math.cos(a)), mul(b, math.sin(a)))
            vertices.append(add(center, mul(off, tube)))
    return normalize(Mesh(vertices, periodic_faces(nu, nv)))


def corrugated_folded_torus(nu: int, nv: int) -> Mesh:
    vertices: list[Vec3] = []
    for i in range(nu):
        u = 2 * math.pi * i / nu
        fold = 0.13 * math.sin(3 * u) + 0.05 * math.sin(7 * u)
        major = 0.62 + 0.08 * math.cos(2 * u)
        for j in range(nv):
            v = 2 * math.pi * j / nv
            r = 0.19 * (1 + 0.10 * math.sin(13 * u + 5 * v) + 0.05 * math.sin(21 * u - 4 * v))
            x = (major + r * math.cos(v)) * math.cos(u)
            y = (major + r * math.cos(v)) * math.sin(u)
            z = r * math.sin(v) + fold
            vertices.append((x, y, z))
    return normalize(Mesh(vertices, periodic_faces(nu, nv)))


def sphere_case(rings: int, segments: int, kind: str) -> Mesh:
    def deform(theta: float, phi: float) -> Vec3:
        st, ct = math.sin(theta), math.cos(theta)
        base = 1.0
        if kind == "mixed_superquadric":
            base += 0.10 * math.cos(4 * phi) * st**4 + 0.06 * math.cos(6 * theta)
        elif kind == "multi_component_mixed":
            base += 0.13 * math.cos(3 * phi) * st**3 + 0.08 * math.cos(5 * theta) + 0.035 * math.sin(11 * phi) * st**5
        elif kind == "needle_spike_sphere":
            spike = max(0.0, math.cos(9 * phi) * math.sin(5 * theta)) ** 10
            base += 0.04 * math.sin(13 * phi) * st**5 + 0.35 * spike
        elif kind == "saddle_peanut":
            base *= 0.72 + 0.28 * abs(ct) ** 0.55
            base += 0.09 * math.cos(2 * phi) * st**2 - 0.05 * math.cos(4 * theta)
        elif kind == "dimpled_ridged_shell":
            ridge = 0.08 * abs(math.sin(8 * phi + 3 * theta)) ** 8 * st**3
            dimple = -0.12 * max(0.0, math.cos(5 * phi - 2 * theta)) ** 10 * st**4
            base += 0.06 * math.cos(3 * theta) + ridge + dimple
        return base * st * math.cos(phi), base * st * math.sin(phi), base * ct
    return sphere_mesh(rings, segments, deform)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--out-dir", type=Path, default=Path("hard_mesh_suite"))
    ap.add_argument("--target-vertices", type=int, default=30000)
    args = ap.parse_args()
    segments = max(72, int(round(math.sqrt(args.target_vertices * 0.75))))
    rings = max(32, (args.target_vertices - 2) // segments)
    nv = max(48, int(round(math.sqrt(args.target_vertices / 5))))
    nu = max(120, args.target_vertices // nv)
    cases = {
        "mixed_superquadric": sphere_case(rings, segments, "mixed_superquadric"),
        "multi_component_mixed": sphere_case(rings, segments, "multi_component_mixed"),
        "needle_spike_sphere": sphere_case(rings, segments, "needle_spike_sphere"),
        "saddle_peanut": sphere_case(rings, segments, "saddle_peanut"),
        "dimpled_ridged_shell": sphere_case(rings, segments, "dimpled_ridged_shell"),
        "twisted_spiky_knot": knot(nu, nv),
        "corrugated_folded_torus": corrugated_folded_torus(nu, nv),
    }
    args.out_dir.mkdir(parents=True, exist_ok=True)
    for name, mesh in cases.items():
        result = validate(mesh)
        if not result.valid:
            raise RuntimeError(f"invalid generated case {name}: {result}")
        path = args.out_dir / f"{name}.in"
        write(mesh, path)
        print(f"{name:28s} {len(mesh.vertices):7d} V {len(mesh.faces):7d} F -> {path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
