#!/usr/bin/env python3
"""Validate IMC2026 modified-OBJ triangular meshes.

Checks indices, positive areas, duplicates, unused vertices, closed two-manifold
edge incidence, and opposite orientation across every shared edge.
"""
from __future__ import annotations

import argparse
import json
import math
from collections import Counter, defaultdict
from dataclasses import asdict, dataclass
from pathlib import Path
from typing import Iterable


@dataclass
class Mesh:
    vertices: list[tuple[float, float, float]]
    faces: list[tuple[int, int, int]]


@dataclass
class Validation:
    vertices: int
    faces: int
    invalid_indices: int
    repeated_vertex_faces: int
    zero_or_nonfinite_area_faces: int
    duplicate_faces: int
    unused_vertices: int
    edges: int
    non_two_manifold_edges: int
    orientation_errors: int
    euler_characteristic: int
    valid: bool


def _tokens(path: Path) -> Iterable[str]:
    with path.open("r", encoding="utf-8", errors="strict") as handle:
        for line in handle:
            yield from line.split()


def read_mesh(path: Path) -> Mesh:
    it = iter(_tokens(path))
    try:
        n_vertices = int(next(it))
        n_faces = int(next(it))
    except (StopIteration, ValueError) as exc:
        raise ValueError(f"invalid header in {path}") from exc

    if n_vertices < 0 or n_faces < 0:
        raise ValueError("negative mesh size")

    vertices: list[tuple[float, float, float]] = []
    for index in range(n_vertices):
        try:
            marker = next(it)
            if marker.lower() != "v":
                raise ValueError(f"expected vertex marker at vertex {index}, got {marker!r}")
            vertices.append((float(next(it)), float(next(it)), float(next(it))))
        except (StopIteration, ValueError) as exc:
            raise ValueError(f"invalid vertex {index}") from exc

    faces: list[tuple[int, int, int]] = []
    for index in range(n_faces):
        try:
            marker = next(it)
            if marker.lower() != "f":
                raise ValueError(f"expected face marker at face {index}, got {marker!r}")
            faces.append((int(next(it)) - 1, int(next(it)) - 1, int(next(it)) - 1))
        except (StopIteration, ValueError) as exc:
            raise ValueError(f"invalid face {index}") from exc

    return Mesh(vertices, faces)


def _area2(a: tuple[float, float, float], b: tuple[float, float, float],
           c: tuple[float, float, float]) -> float:
    ux, uy, uz = b[0] - a[0], b[1] - a[1], b[2] - a[2]
    vx, vy, vz = c[0] - a[0], c[1] - a[1], c[2] - a[2]
    nx = uy * vz - uz * vy
    ny = uz * vx - ux * vz
    nz = ux * vy - uy * vx
    return nx * nx + ny * ny + nz * nz


def validate(mesh: Mesh, *, area_epsilon: float = 1e-24) -> Validation:
    n = len(mesh.vertices)
    invalid_indices = 0
    repeated = 0
    bad_area = 0
    duplicate_faces = 0
    used = [False] * n
    face_keys: set[tuple[int, int, int]] = set()
    edge_incidence: Counter[tuple[int, int]] = Counter()
    directed: Counter[tuple[int, int]] = Counter()

    for face in mesh.faces:
        if any(vertex < 0 or vertex >= n for vertex in face):
            invalid_indices += 1
            continue
        if len(set(face)) != 3:
            repeated += 1
            continue

        for vertex in face:
            used[vertex] = True

        key = tuple(sorted(face))
        if key in face_keys:
            duplicate_faces += 1
        face_keys.add(key)

        area2 = _area2(*(mesh.vertices[vertex] for vertex in face))
        if not math.isfinite(area2) or area2 <= area_epsilon:
            bad_area += 1

        a, b, c = face
        for u, v in ((a, b), (b, c), (c, a)):
            edge_incidence[(min(u, v), max(u, v))] += 1
            directed[(u, v)] += 1

    non_two = sum(count != 2 for count in edge_incidence.values())
    orientation_errors = 0
    for u, v in edge_incidence:
        if directed[(u, v)] != 1 or directed[(v, u)] != 1:
            orientation_errors += 1

    unused = sum(not flag for flag in used)
    euler = n - len(edge_incidence) + len(mesh.faces)
    valid = not any((invalid_indices, repeated, bad_area, duplicate_faces,
                     non_two, orientation_errors))

    return Validation(
        vertices=n,
        faces=len(mesh.faces),
        invalid_indices=invalid_indices,
        repeated_vertex_faces=repeated,
        zero_or_nonfinite_area_faces=bad_area,
        duplicate_faces=duplicate_faces,
        unused_vertices=unused,
        edges=len(edge_incidence),
        non_two_manifold_edges=non_two,
        orientation_errors=orientation_errors,
        euler_characteristic=euler,
        valid=valid,
    )


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("mesh", type=Path)
    parser.add_argument("--area-epsilon", type=float, default=1e-24)
    parser.add_argument("--json", action="store_true")
    args = parser.parse_args()

    try:
        result = validate(read_mesh(args.mesh), area_epsilon=args.area_epsilon)
    except (OSError, ValueError) as exc:
        print(json.dumps({"valid": False, "parse_error": str(exc)}, indent=2))
        return 2

    if args.json:
        print(json.dumps(asdict(result), indent=2, sort_keys=True))
    else:
        for key, value in asdict(result).items():
            print(f"{key}: {value}")

    return 0 if result.valid else 1


if __name__ == "__main__":
    raise SystemExit(main())
