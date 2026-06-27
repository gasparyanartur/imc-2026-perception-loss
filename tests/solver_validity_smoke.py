#!/usr/bin/env python3
"""Smoke-test a C++ solver for output validity and over-collapse regressions.

This is intentionally independent of pytest so it can run in the lightweight
competition workspace:

    python3 tests/solver_validity_smoke.py simplifygeometry_v2_aggressive.cpp

The generated meshes are closed tori with vertex counts matching official size
tiers. The checks are not a substitute for the full SSIM evaluator, but they
catch two classes of failures that the small local dataset can miss:

* malformed or non-manifold solver output;
* accidental removal of the calibrated target-vertex floor.
"""

from __future__ import annotations

import argparse
import math
import os
import subprocess
import sys
import tempfile
from dataclasses import dataclass
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(REPO_ROOT))

import evaluate  # noqa: E402


@dataclass(frozen=True)
class Case:
    name: str
    u_segments: int
    v_segments: int


DEFAULT_CASES = [
    Case("torus_5k", 50, 100),        # 5,000 vertices
    Case("torus_25k", 125, 200),      # 25,000 vertices
]

LARGE_CASES = [
    Case("torus_40k", 200, 200),      # 40,000 vertices
    Case("torus_50k", 200, 250),      # 50,000 vertices
]


def target_vertex_count(nv: int) -> int:
    if nv <= 10:
        return nv
    if nv <= 5000:
        return max(10, int(nv * 0.40))
    if nv <= 25000:
        return max(10, int(nv * 0.70))
    if nv <= 45000:
        return max(10, int(nv * 0.35))
    if nv <= 50000:
        return max(10, int(nv * 0.30))
    if nv <= 400000:
        return max(10, int(nv * 0.18))
    return max(10, int(nv * 0.11))


def torus(u_segments: int, v_segments: int) -> evaluate.Mesh:
    """Build a consistently oriented triangular torus inside the unit sphere."""
    major_radius = 0.60
    minor_radius = 0.28
    vertices: list[list[float]] = []
    for i in range(u_segments):
        u = 2.0 * math.pi * i / u_segments
        cu = math.cos(u)
        su = math.sin(u)
        for j in range(v_segments):
            v = 2.0 * math.pi * j / v_segments
            cv = math.cos(v)
            sv = math.sin(v)
            ring_radius = major_radius + minor_radius * cv
            vertices.append([
                ring_radius * cu,
                ring_radius * su,
                minor_radius * sv,
            ])

    def vid(i: int, j: int) -> int:
        return (i % u_segments) * v_segments + (j % v_segments)

    faces: list[list[int]] = []
    for i in range(u_segments):
        for j in range(v_segments):
            a = vid(i, j)
            b = vid(i, j + 1)
            c = vid(i + 1, j)
            d = vid(i + 1, j + 1)
            faces.append([a, c, d])
            faces.append([a, d, b])

    import numpy as np

    return evaluate.Mesh(
        np.asarray(vertices, dtype=float),
        np.asarray(faces, dtype=int),
    )


def write_mesh(mesh: evaluate.Mesh, path: Path) -> None:
    with path.open("w", encoding="utf-8") as handle:
        handle.write(f"{len(mesh.vertices)} {len(mesh.faces)}\n")
        for v in mesh.vertices:
            handle.write("v %.10g %.10g %.10g\n" % (v[0], v[1], v[2]))
        for f in mesh.faces:
            handle.write("f %d %d %d\n" % (f[0] + 1, f[1] + 1, f[2] + 1))


def compile_solver(source: Path, out_bin: Path, cxx: str) -> None:
    subprocess.run(
        [cxx, "-O2", "-std=c++17", str(source), "-o", str(out_bin)],
        cwd=REPO_ROOT,
        check=True,
    )


def run_solver(binary: Path, input_path: Path, output_path: Path,
               timeout: float) -> None:
    with input_path.open("rb") as fin, output_path.open("wb") as fout:
        subprocess.run(
            [str(binary)],
            stdin=fin,
            stdout=fout,
            stderr=subprocess.PIPE,
            timeout=timeout,
            check=True,
        )


def check_case(case: Case, binary: Path, tmp: Path, timeout: float) -> bool:
    original = torus(case.u_segments, case.v_segments)
    original_validity = evaluate.check_validity(original, original)
    if not original_validity.ok:
        print(f"{case.name}: generated invalid fixture: {original_validity.reasons}")
        return False

    input_path = tmp / f"{case.name}.in.txt"
    output_path = tmp / f"{case.name}.out.txt"
    write_mesh(original, input_path)
    run_solver(binary, input_path, output_path, timeout)

    simplified = evaluate.load_mesh(str(output_path))
    validity = evaluate.check_validity(simplified, original)
    min_vertices = min(len(original.vertices) - 1,
                       target_vertex_count(len(original.vertices)))
    enough_vertices = len(simplified.vertices) >= min_vertices
    keep = len(simplified.vertices) / len(original.vertices)

    status = "PASS" if validity.ok and enough_vertices else "FAIL"
    print(
        "%-12s %-4s original=%6d simplified=%6d keep=%6.2f%% min=%6d %s"
        % (
            case.name,
            status,
            len(original.vertices),
            len(simplified.vertices),
            keep * 100.0,
            min_vertices,
            "; ".join(validity.reasons),
        )
    )
    return validity.ok and enough_vertices


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "source",
        nargs="?",
        default="simplifygeometry_v2_aggressive.cpp",
        help="C++ solver source to compile",
    )
    parser.add_argument("--cxx", default=os.environ.get("CXX", "g++"))
    parser.add_argument("--timeout", type=float, default=120.0)
    parser.add_argument(
        "--large",
        action="store_true",
        help="also run 40k/50k tier fixtures",
    )
    args = parser.parse_args(argv)

    source = (REPO_ROOT / args.source).resolve()
    cases = list(DEFAULT_CASES)
    if args.large:
        cases.extend(LARGE_CASES)

    with tempfile.TemporaryDirectory(prefix="solver-smoke-") as tmp_name:
        tmp = Path(tmp_name)
        binary = tmp / "solver"
        compile_solver(source, binary, args.cxx)
        ok = True
        for case in cases:
            ok = check_case(case, binary, tmp, args.timeout) and ok
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main())
