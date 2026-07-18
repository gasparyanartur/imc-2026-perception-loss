#!/usr/bin/env python3
"""Generate deterministic medium IMC2026 stress meshes and run solvers.

The suite is diagnostic. It checks runtime, output compression, and structural
validity; it does not reproduce the official 1024 SSIM evaluator.
"""
from __future__ import annotations

import argparse
import csv
import hashlib
import json
import math
import os
import resource
import subprocess
import sys
import time
from dataclasses import asdict
from pathlib import Path
from typing import Callable

sys.path.insert(0, str(Path(__file__).resolve().parent))
from mesh_validate import Mesh, read_mesh, validate  # noqa: E402

Vec3 = tuple[float, float, float]
Face = tuple[int, int, int]


def add(a: Vec3, b: Vec3) -> Vec3:
    return a[0] + b[0], a[1] + b[1], a[2] + b[2]


def sub(a: Vec3, b: Vec3) -> Vec3:
    return a[0] - b[0], a[1] - b[1], a[2] - b[2]


def mul(a: Vec3, s: float) -> Vec3:
    return a[0] * s, a[1] * s, a[2] * s


def dot(a: Vec3, b: Vec3) -> float:
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


def cross(a: Vec3, b: Vec3) -> Vec3:
    return (a[1] * b[2] - a[2] * b[1],
            a[2] * b[0] - a[0] * b[2],
            a[0] * b[1] - a[1] * b[0])


def norm(a: Vec3) -> float:
    return math.sqrt(dot(a, a))


def unit(a: Vec3) -> Vec3:
    length = norm(a)
    if length <= 1e-15:
        raise ValueError("cannot normalize zero vector")
    return mul(a, 1.0 / length)


def normalize_mesh(mesh: Mesh, radius: float = 0.95) -> Mesh:
    maximum = max(norm(vertex) for vertex in mesh.vertices)
    scale = radius / maximum if maximum > 0 else 1.0
    return Mesh([mul(vertex, scale) for vertex in mesh.vertices], mesh.faces)


def write_mesh(mesh: Mesh, path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8") as handle:
        handle.write(f"{len(mesh.vertices)} {len(mesh.faces)}\n")
        for x, y, z in mesh.vertices:
            handle.write(f"v {x:.12g} {y:.12g} {z:.12g}\n")
        for a, b, c in mesh.faces:
            handle.write(f"f {a + 1} {b + 1} {c + 1}\n")


def periodic_grid_faces(nu: int, nv: int) -> list[Face]:
    faces: list[Face] = []
    for i in range(nu):
        ni = (i + 1) % nu
        for j in range(nv):
            nj = (j + 1) % nv
            a, b = i * nv + j, ni * nv + j
            c, d = ni * nv + nj, i * nv + nj
            faces.extend(((a, b, c), (a, c, d)))
    return faces


def torus_knot(nu: int, nv: int, *, spike: float = 0.0) -> Mesh:
    vertices: list[Vec3] = []
    p, q = 2.0, 3.0
    major, knot_minor, tube = 0.62, 0.18, 0.075
    for i in range(nu):
        t = 2.0 * math.pi * i / nu
        ct, st = math.cos(p * t), math.sin(p * t)
        cq, sq = math.cos(q * t), math.sin(q * t)
        radial = major + knot_minor * cq
        center = (radial * ct, radial * st, knot_minor * sq)
        tangent = unit((
            -p * radial * st - q * knot_minor * sq * ct,
            p * radial * ct - q * knot_minor * sq * st,
            q * knot_minor * cq,
        ))
        reference = (0.0, 0.0, 1.0)
        if abs(dot(tangent, reference)) > 0.95:
            reference = (0.0, 1.0, 0.0)
        normal = unit(cross(reference, tangent))
        binormal = unit(cross(tangent, normal))
        pulse = max(0.0, math.cos(11.0 * t)) ** 6
        local_tube = tube * (1.0 + spike * pulse)
        for j in range(nv):
            angle = 2.0 * math.pi * j / nv
            offset = add(mul(normal, math.cos(angle)), mul(binormal, math.sin(angle)))
            vertices.append(add(center, mul(offset, local_tube)))
    return normalize_mesh(Mesh(vertices, periodic_grid_faces(nu, nv)))


def corrugated_torus(nu: int, nv: int) -> Mesh:
    vertices: list[Vec3] = []
    major, minor = 0.68, 0.22
    for i in range(nu):
        u = 2.0 * math.pi * i / nu
        for j in range(nv):
            v = 2.0 * math.pi * j / nv
            corrugation = 1.0 + 0.10 * math.sin(13 * u + 5 * v)
            r = minor * corrugation
            vertices.append(((major + r * math.cos(v)) * math.cos(u),
                             (major + r * math.cos(v)) * math.sin(u),
                             r * math.sin(v)))
    return normalize_mesh(Mesh(vertices, periodic_grid_faces(nu, nv)))


def sphere_mesh(rings: int, segments: int,
                deformation: Callable[[float, float], tuple[float, float, float]]) -> Mesh:
    vertices: list[Vec3] = [(0.0, 0.0, 1.0)]
    for ring in range(1, rings + 1):
        theta = math.pi * ring / (rings + 1)
        for segment in range(segments):
            phi = 2.0 * math.pi * segment / segments
            vertices.append(deformation(theta, phi))
    south = len(vertices)
    vertices.append((0.0, 0.0, -1.0))

    faces: list[Face] = []
    first = 1
    for segment in range(segments):
        nxt = (segment + 1) % segments
        faces.append((0, first + nxt, first + segment))

    for ring in range(rings - 1):
        base = 1 + ring * segments
        below = base + segments
        for segment in range(segments):
            nxt = (segment + 1) % segments
            a, b = base + segment, base + nxt
            c, d = below + nxt, below + segment
            faces.extend(((a, b, c), (a, c, d)))

    last = 1 + (rings - 1) * segments
    for segment in range(segments):
        nxt = (segment + 1) % segments
        faces.append((last + segment, last + nxt, south))

    return normalize_mesh(Mesh(vertices, faces))


def bumpy_sphere(rings: int, segments: int) -> Mesh:
    def deform(theta: float, phi: float) -> Vec3:
        radius = 1.0 + 0.06 * math.sin(7 * theta) * math.cos(9 * phi) + 0.025 * math.sin(17 * phi)
        return (radius * math.sin(theta) * math.cos(phi),
                radius * math.sin(theta) * math.sin(phi),
                radius * math.cos(theta))
    return sphere_mesh(rings, segments, deform)


def pinched_sphere(rings: int, segments: int) -> Mesh:
    def deform(theta: float, phi: float) -> Vec3:
        z = math.cos(theta)
        pinch = 0.50 + 0.50 * abs(z) ** 0.65
        ripple = 1.0 + 0.035 * math.cos(8 * phi) * math.sin(theta) ** 4
        return (pinch * ripple * math.sin(theta) * math.cos(phi),
                pinch * ripple * math.sin(theta) * math.sin(phi), z)
    return sphere_mesh(rings, segments, deform)


def signed_power(value: float, exponent: float) -> float:
    return math.copysign(abs(value) ** exponent, value)


def rounded_cube(rings: int, segments: int) -> Mesh:
    def deform(theta: float, phi: float) -> Vec3:
        eta = math.pi / 2.0 - theta
        exponent = 0.34
        ce = signed_power(math.cos(eta), exponent)
        return (ce * signed_power(math.cos(phi), exponent),
                ce * signed_power(math.sin(phi), exponent),
                signed_power(math.sin(eta), exponent))
    return sphere_mesh(rings, segments, deform)


def subdivided_cube(subdivisions: int, ripple: float = 0.0) -> Mesh:
    """Closed cube whose six planar faces contain dense interior vertices."""
    n = max(2, subdivisions)
    vertices: list[Vec3] = []
    index: dict[tuple[int, int, int], int] = {}

    def vertex(i: int, j: int, k: int) -> int:
        key = (i, j, k)
        if key not in index:
            index[key] = len(vertices)
            xyz = [2.0 * i / n - 1.0,
                   2.0 * j / n - 1.0,
                   2.0 * k / n - 1.0]
            lattice = (i, j, k)
            boundary_axes = [axis for axis, value in enumerate(lattice)
                             if value == 0 or value == n]
            if ripple and len(boundary_axes) == 1:
                axis = boundary_axes[0]
                other = [value for a, value in enumerate(lattice) if a != axis]
                wave = math.sin(math.pi * other[0] / n) * math.sin(math.pi * other[1] / n)
                xyz[axis] += ripple * wave * (-1.0 if lattice[axis] == 0 else 1.0)
            vertices.append((xyz[0], xyz[1], xyz[2]))
        return index[key]

    faces: list[Face] = []

    def quad(a: tuple[int, int, int], b: tuple[int, int, int],
             c: tuple[int, int, int], d: tuple[int, int, int]) -> None:
        ia, ib, ic, id_ = vertex(*a), vertex(*b), vertex(*c), vertex(*d)
        faces.extend(((ia, ib, ic), (ia, ic, id_)))

    for i in range(n):
        for j in range(n):
            quad((0, i, j), (0, i, j + 1), (0, i + 1, j + 1), (0, i + 1, j))
            quad((n, i, j), (n, i + 1, j), (n, i + 1, j + 1), (n, i, j + 1))
            quad((i, 0, j), (i + 1, 0, j), (i + 1, 0, j + 1), (i, 0, j + 1))
            quad((i, n, j), (i, n, j + 1), (i + 1, n, j + 1), (i + 1, n, j))
            quad((i, j, 0), (i, j + 1, 0), (i + 1, j + 1, 0), (i + 1, j, 0))
            quad((i, j, n), (i + 1, j, n), (i + 1, j + 1, n), (i, j + 1, n))
    return normalize_mesh(Mesh(vertices, faces))


def dimensions(target_vertices: int) -> tuple[int, int, int, int]:
    nv = max(24, int(round(math.sqrt(target_vertices / 4))))
    nu = max(64, target_vertices // nv)
    segments = max(48, int(round(math.sqrt(target_vertices * 0.60))))
    rings = max(16, (target_vertices - 2) // segments)
    return nu, nv, rings, segments


def generate_suite(out_dir: Path, target_vertices: int) -> list[Path]:
    nu, nv, rings, segments = dimensions(target_vertices)
    cube_subdivisions = max(2, int(round(math.sqrt(max(1, target_vertices - 2) / 6.0))))
    cases = {
        "smooth_torus_knot": torus_knot(nu, nv, spike=0.0),
        "spiky_torus_knot": torus_knot(nu, nv, spike=0.55),
        "corrugated_torus": corrugated_torus(nu, nv),
        "bumpy_sphere": bumpy_sphere(rings, segments),
        "rounded_cube": rounded_cube(rings, segments),
        "subdivided_cube": subdivided_cube(cube_subdivisions),
        "near_planar_cube": subdivided_cube(cube_subdivisions, ripple=0.001),
        "pinched_sphere": pinched_sphere(rings, segments),
        "tetrahedron": Mesh([(1, 1, 1), (-1, -1, 1), (-1, 1, -1), (1, -1, -1)],
                            [(0, 2, 1), (0, 1, 3), (0, 3, 2), (1, 2, 3)]),
        "octahedron": Mesh([(1, 0, 0), (-1, 0, 0), (0, 1, 0), (0, -1, 0),
                            (0, 0, 1), (0, 0, -1)],
                           [(0, 2, 4), (2, 1, 4), (1, 3, 4), (3, 0, 4),
                            (2, 0, 5), (1, 2, 5), (3, 1, 5), (0, 3, 5)]),
    }
    paths: list[Path] = []
    for name, mesh in cases.items():
        path = out_dir / f"{name}.in"
        write_mesh(normalize_mesh(mesh), path)
        result = validate(mesh if name not in {"tetrahedron", "octahedron"} else normalize_mesh(mesh))
        if not result.valid:
            raise RuntimeError(f"generated invalid case {name}: {result}")
        paths.append(path)
    return paths


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1 << 20), b""):
            digest.update(chunk)
    return digest.hexdigest()


def compile_source(source: Path, output: Path) -> None:
    subprocess.run(["g++", "-std=c++17", "-O2", "-pipe", str(source), "-o", str(output)], check=True)


def run_solver(solver: Path, input_path: Path, output_path: Path,
               timeout: float) -> dict[str, object]:
    before = resource.getrusage(resource.RUSAGE_CHILDREN).ru_maxrss
    start = time.perf_counter()
    with input_path.open("rb") as source, output_path.open("wb") as destination:
        process = subprocess.run([str(solver)], stdin=source, stdout=destination,
                                 stderr=subprocess.PIPE, timeout=timeout)
    elapsed = time.perf_counter() - start
    after = resource.getrusage(resource.RUSAGE_CHILDREN).ru_maxrss
    record: dict[str, object] = {
        "returncode": process.returncode,
        "seconds": elapsed,
        "peak_rss_kb_delta": max(0, after - before),
        "stderr": process.stderr.decode("utf-8", "replace")[-4000:],
    }
    if process.returncode == 0:
        try:
            mesh = read_mesh(output_path)
            result = validate(mesh)
            record.update(asdict(result))
        except Exception as exc:  # diagnostic runner: retain parse failures
            record.update({"valid": False, "parse_error": str(exc)})
    else:
        record["valid"] = False
    return record


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out-dir", type=Path, default=Path("medium_mesh_suite"))
    parser.add_argument("--target-vertices", type=int, default=24000)
    parser.add_argument("--generate", action="store_true")
    parser.add_argument("--source", type=Path, help="compile and run this C++ source")
    parser.add_argument("--solver", type=Path, help="run this existing executable")
    parser.add_argument("--baseline", type=Path, help="optional baseline executable")
    parser.add_argument("--timeout", type=float, default=30.0)
    args = parser.parse_args()

    args.out_dir.mkdir(parents=True, exist_ok=True)
    inputs = sorted(args.out_dir.glob("*.in"))
    if args.generate or not inputs:
        inputs = generate_suite(args.out_dir, args.target_vertices)

    solver = args.solver
    if args.source:
        solver = args.out_dir / "candidate_solver"
        compile_source(args.source, solver)
    if solver is None:
        print("Generated cases:")
        for path in inputs:
            mesh = read_mesh(path)
            print(f"  {path.name}: {len(mesh.vertices)} V / {len(mesh.faces)} F")
        return 0

    solver = solver.resolve()
    baseline = args.baseline.resolve() if args.baseline else None
    records: list[dict[str, object]] = []

    for input_path in inputs:
        input_mesh = read_mesh(input_path)
        output = args.out_dir / f"{input_path.stem}.candidate.out"
        record = {
            "case": input_path.stem,
            "input_vertices": len(input_mesh.vertices),
            "input_faces": len(input_mesh.faces),
            "input_sha256": sha256(input_path),
        }
        record.update(run_solver(solver, input_path, output, args.timeout))
        if record.get("vertices"):
            record["retained_ratio"] = record["vertices"] / len(input_mesh.vertices)
            record["compression_percent"] = 100.0 * (1.0 - record["retained_ratio"])
        if baseline:
            baseline_output = args.out_dir / f"{input_path.stem}.baseline.out"
            baseline_record = run_solver(baseline, input_path, baseline_output, args.timeout)
            record["baseline_vertices"] = baseline_record.get("vertices")
            record["baseline_seconds"] = baseline_record.get("seconds")
            record["baseline_valid"] = baseline_record.get("valid")
            if record.get("vertices") and baseline_record.get("vertices"):
                record["vertex_delta_vs_baseline"] = int(record["vertices"]) - int(baseline_record["vertices"])
        records.append(record)
        status = "PASS" if record.get("valid") else "FAIL"
        print(f"{input_path.stem:24s} {status} V={record.get('vertices')} "
              f"ratio={record.get('retained_ratio', float('nan')):.4f} "
              f"time={record.get('seconds', 0.0):.3f}s")

    json_path = args.out_dir / "results.json"
    json_path.write_text(json.dumps(records, indent=2, sort_keys=True), encoding="utf-8")
    csv_path = args.out_dir / "results.csv"
    keys = sorted({key for record in records for key in record})
    with csv_path.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=keys)
        writer.writeheader()
        writer.writerows(records)
    print(f"Wrote {json_path} and {csv_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
