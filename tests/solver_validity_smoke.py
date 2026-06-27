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
import time
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
    kind: str = "torus"


DEFAULT_CASES = [
    Case("torus_5k", 50, 100),        # 5,000 vertices
    Case("torus_25k", 125, 200),      # 25,000 vertices
]

LARGE_CASES = [
    Case("torus_40k", 200, 200),      # 40,000 vertices
    Case("torus_50k", 200, 250),      # 50,000 vertices
]

EXTREME_CASES = [
    Case("torus_400k", 500, 800),     # 400,000 vertices, 800,000 faces
    Case("torus_1m", 1000, 1000),     # 1,000,000 vertices, 2,000,000 faces
]

BUMPY_CASES = [
    Case("bumpy_5k", 50, 100, "bumpy_torus"),
    Case("bumpy_25k", 125, 200, "bumpy_torus"),
    Case("bumpy_40k", 200, 200, "bumpy_torus"),
    Case("bumpy_50k", 200, 250, "bumpy_torus"),
]


def target_vertex_count(nv: int) -> int:
    if nv <= 10:
        return nv
    if nv <= 5000:
        return max(10, int(nv * 0.30))
    if nv <= 25000:
        return max(10, int(nv * 0.57))
    if nv <= 45000:
        return max(10, int(nv * 0.35))
    if nv <= 50000:
        return max(10, int(nv * 0.27))
    if nv <= 400000:
        return max(10, int(nv * 0.11))
    return max(10, int(nv * 0.10))


def torus(u_segments: int, v_segments: int,
          kind: str = "torus") -> evaluate.Mesh:
    """Build a consistently oriented triangular torus inside the unit sphere."""
    if kind == "bumpy_torus":
        major_radius = 0.56
        minor_radius = 0.23
    else:
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
            local_minor = minor_radius
            if kind == "bumpy_torus":
                local_minor *= (
                    1.0
                    + 0.16 * math.sin(9.0 * u + 0.4) * math.sin(7.0 * v)
                    + 0.06 * math.cos(17.0 * u - 3.0 * v)
                )
            ring_radius = major_radius + local_minor * cv
            vertices.append([
                ring_radius * cu,
                ring_radius * su,
                local_minor * sv,
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


def compile_solver(source: Path, out_bin: Path, cxx: str,
                   cxxflags: list[str]) -> None:
    subprocess.run(
        [cxx, "-O2", "-std=c++17", *cxxflags, str(source), "-o", str(out_bin)],
        cwd=REPO_ROOT,
        check=True,
    )


def run_solver(binary: Path, input_path: Path, output_path: Path,
               timeout: float) -> float:
    start = time.monotonic()
    with input_path.open("rb") as fin, output_path.open("wb") as fout:
        subprocess.run(
            [str(binary)],
            stdin=fin,
            stdout=fout,
            stderr=subprocess.PIPE,
            timeout=timeout,
            check=True,
        )
    return time.monotonic() - start


def check_case(case: Case, binary: Path, tmp: Path, timeout: float,
               score: bool, resolution: int, score_max_vertices: int) -> bool:
    original = torus(case.u_segments, case.v_segments, case.kind)
    original_validity = evaluate.check_validity(original, original)
    if not original_validity.ok:
        print(f"{case.name}: generated invalid fixture: {original_validity.reasons}")
        return False

    input_path = tmp / f"{case.name}.in.txt"
    output_path = tmp / f"{case.name}.out.txt"
    write_mesh(original, input_path)
    elapsed = run_solver(binary, input_path, output_path, timeout)

    simplified = evaluate.load_mesh(str(output_path))
    validity = evaluate.check_validity(simplified, original)
    min_vertices = min(len(original.vertices) - 1,
                       target_vertex_count(len(original.vertices)))
    enough_vertices = len(simplified.vertices) >= min_vertices
    keep = len(simplified.vertices) / len(original.vertices)

    status = "PASS" if validity.ok and enough_vertices else "FAIL"
    details = [
        "%-12s %-4s original=%7d simplified=%7d keep=%6.2f%% min=%7d time=%7.2fs"
        % (
            case.name, status, len(original.vertices), len(simplified.vertices),
            keep * 100.0, min_vertices, elapsed,
        )
    ]

    ok = validity.ok and enough_vertices
    if score and len(original.vertices) <= score_max_vertices:
        result = evaluate.evaluate(original, simplified, resolution=resolution)
        ok = ok and result.valid
        details.append(
            "score=%s compr=%7.3f%% haus=%8.4f/%8.4f ssim=%7.4f"
            % (
                "VALID" if result.valid else "INVALID",
                result.compression_rate,
                result.hausdorff,
                result.hausdorff_bound,
                result.final_ssim,
            )
        )
    elif score:
        details.append("score=SKIP(vertices>%d)" % score_max_vertices)

    reasons = list(validity.reasons)
    if not enough_vertices:
        reasons.append("simplified below target floor")
    if reasons:
        details.append("; ".join(reasons))

    print(" ".join(details), flush=True)
    return ok


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "source",
        nargs="?",
        default="simplifygeometry_v2_aggressive.cpp",
        help="C++ solver source to compile",
    )
    parser.add_argument("--cxx", default=os.environ.get("CXX", "g++"))
    parser.add_argument(
        "--cxxflags",
        default=os.environ.get("CXXFLAGS", ""),
        help="extra compiler flags, e.g. '-I /usr/include/eigen3'",
    )
    parser.add_argument("--timeout", type=float, default=120.0)
    parser.add_argument(
        "--large",
        action="store_true",
        help="also run 40k/50k tier fixtures",
    )
    parser.add_argument(
        "--extreme",
        action="store_true",
        help="also run 400k/1m fixtures; this is intended for runtime/memory smoke",
    )
    parser.add_argument(
        "--bumpy",
        action="store_true",
        help="also run higher-frequency bumpy torus fixtures through 50k",
    )
    parser.add_argument(
        "--score",
        action="store_true",
        help="run the full local evaluator on generated cases up to --score-max-vertices",
    )
    parser.add_argument("--resolution", type=int, default=1024)
    parser.add_argument("--score-max-vertices", type=int, default=50000)
    args = parser.parse_args(argv)

    source = (REPO_ROOT / args.source).resolve()
    cases = list(DEFAULT_CASES)
    if args.large:
        cases.extend(LARGE_CASES)
    if args.bumpy:
        cases.extend(BUMPY_CASES)
    if args.extreme:
        cases.extend(EXTREME_CASES)

    with tempfile.TemporaryDirectory(prefix="solver-smoke-") as tmp_name:
        tmp = Path(tmp_name)
        binary = tmp / "solver"
        compile_solver(source, binary, args.cxx, args.cxxflags.split())
        ok = True
        for case in cases:
            ok = check_case(
                case, binary, tmp, args.timeout, args.score, args.resolution,
                args.score_max_vertices,
            ) and ok
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main())
