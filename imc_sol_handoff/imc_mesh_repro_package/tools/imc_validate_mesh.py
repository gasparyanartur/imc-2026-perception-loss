#!/usr/bin/env python3
"""Validate IMC custom-format triangular meshes and optionally estimate Hausdorff distance.

Format:
    <num_vertices> <num_faces>
    v x y z
    ...
    f i j k   # 1-indexed

The topology checks are exact for the parsed mesh. The Hausdorff check is sampled
and therefore diagnostic only; it is not a replacement for the official checker.
"""
from __future__ import annotations

import argparse
import json
import math
from collections import Counter, defaultdict, deque
from pathlib import Path
from typing import Iterable

import numpy as np


def read_mesh(path: Path) -> tuple[np.ndarray, np.ndarray]:
    with path.open("r", encoding="utf-8") as f:
        header = f.readline().split()
        if len(header) != 2:
            raise ValueError("invalid header")
        nv, nf = map(int, header)
        vertices = np.empty((nv, 3), dtype=np.float64)
        faces = np.empty((nf, 3), dtype=np.int64)
        for i in range(nv):
            row = f.readline().split()
            if len(row) != 4 or row[0] != "v":
                raise ValueError(f"invalid vertex line {i + 2}")
            vertices[i] = [float(row[1]), float(row[2]), float(row[3])]
        for i in range(nf):
            row = f.readline().split()
            if len(row) != 4 or row[0] != "f":
                raise ValueError(f"invalid face line {nv + i + 2}")
            faces[i] = [int(row[1]) - 1, int(row[2]) - 1, int(row[3]) - 1]
    return vertices, faces


def validate(vertices: np.ndarray, faces: np.ndarray) -> dict:
    nv = len(vertices)
    nf = len(faces)
    result: dict[str, object] = {
        "vertices": nv,
        "faces": nf,
        "finite_vertices": bool(np.isfinite(vertices).all()),
        "valid_indices": True,
        "positive_area": True,
        "duplicate_faces": 0,
        "edge_incidence_ok": True,
        "edge_orientation_ok": True,
        "closed_oriented_manifold": False,
        "connected_components": 0,
        "unused_vertices": 0,
        "min_double_area": None,
        "max_double_area": None,
        "reason": "ok",
    }
    if nv == 0 or nf == 0:
        result["reason"] = "empty mesh"
        return result
    if not result["finite_vertices"]:
        result["reason"] = "non-finite vertex"
        return result
    if faces.min(initial=0) < 0 or faces.max(initial=-1) >= nv:
        result["valid_indices"] = False
        result["reason"] = "face index out of range"
        return result
    repeated = np.any(
        (faces[:, 0] == faces[:, 1])
        | (faces[:, 1] == faces[:, 2])
        | (faces[:, 2] == faces[:, 0])
    )
    if repeated:
        result["positive_area"] = False
        result["reason"] = "repeated vertex in face"
        return result

    p = vertices[faces]
    double_area = np.linalg.norm(np.cross(p[:, 1] - p[:, 0], p[:, 2] - p[:, 0]), axis=1)
    result["min_double_area"] = float(double_area.min())
    result["max_double_area"] = float(double_area.max())
    scale = float(np.linalg.norm(vertices.max(axis=0) - vertices.min(axis=0)))
    area_eps = max(1e-18, 1e-14 * max(scale * scale, 1.0))
    if np.any(double_area <= area_eps):
        result["positive_area"] = False
        result["reason"] = "zero or near-zero face area"

    canonical = np.sort(faces, axis=1)
    _, counts = np.unique(canonical, axis=0, return_counts=True)
    duplicate_faces = int(np.sum(counts - 1))
    result["duplicate_faces"] = duplicate_faces
    if duplicate_faces and result["reason"] == "ok":
        result["reason"] = "duplicate face"

    edge_count: Counter[tuple[int, int]] = Counter()
    edge_orientation: Counter[tuple[int, int]] = Counter()
    adjacency: dict[int, set[int]] = defaultdict(set)
    used = np.zeros(nv, dtype=bool)
    for a, b, c in faces.tolist():
        used[[a, b, c]] = True
        for x, y in ((a, b), (b, c), (c, a)):
            lo, hi = (x, y) if x < y else (y, x)
            edge_count[(lo, hi)] += 1
            edge_orientation[(lo, hi)] += 1 if x == lo else -1
            adjacency[x].add(y)
            adjacency[y].add(x)

    bad_incidence = sum(v != 2 for v in edge_count.values())
    bad_orientation = sum(edge_orientation[e] != 0 for e in edge_count)
    result["bad_edge_incidence_count"] = int(bad_incidence)
    result["bad_edge_orientation_count"] = int(bad_orientation)
    result["edge_incidence_ok"] = bad_incidence == 0
    result["edge_orientation_ok"] = bad_orientation == 0
    result["unused_vertices"] = int((~used).sum())

    seen: set[int] = set()
    components = 0
    for start in np.flatnonzero(used):
        start = int(start)
        if start in seen:
            continue
        components += 1
        q: deque[int] = deque([start])
        seen.add(start)
        while q:
            v = q.popleft()
            for nb in adjacency[v]:
                if nb not in seen:
                    seen.add(nb)
                    q.append(nb)
    result["connected_components"] = components

    result["closed_oriented_manifold"] = bool(
        result["finite_vertices"]
        and result["valid_indices"]
        and result["positive_area"]
        and duplicate_faces == 0
        and result["edge_incidence_ok"]
        and result["edge_orientation_ok"]
    )
    if not result["edge_incidence_ok"] and result["reason"] == "ok":
        result["reason"] = "edge incidence is not exactly two"
    elif not result["edge_orientation_ok"] and result["reason"] == "ok":
        result["reason"] = "edge orientations are inconsistent"
    return result


def surface_samples(vertices: np.ndarray, faces: np.ndarray, n: int, seed: int) -> np.ndarray:
    rng = np.random.default_rng(seed)
    tri = vertices[faces]
    areas = np.linalg.norm(np.cross(tri[:, 1] - tri[:, 0], tri[:, 2] - tri[:, 0]), axis=1) * 0.5
    total = float(areas.sum())
    if total <= 0:
        return vertices.copy()
    ids = rng.choice(len(tri), size=n, p=areas / total)
    r1 = np.sqrt(rng.random(n))
    r2 = rng.random(n)
    pts = (
        (1.0 - r1)[:, None] * tri[ids, 0]
        + (r1 * (1.0 - r2))[:, None] * tri[ids, 1]
        + (r1 * r2)[:, None] * tri[ids, 2]
    )
    centroids = tri.mean(axis=1)
    stride = max(1, len(centroids) // 4000)
    return np.vstack([vertices, centroids[::stride], pts])


def sampled_hausdorff(
    a_v: np.ndarray,
    a_f: np.ndarray,
    b_v: np.ndarray,
    b_f: np.ndarray,
    samples: int,
) -> dict:
    try:
        from scipy.spatial import cKDTree
    except ImportError as exc:
        raise RuntimeError("scipy is required for sampled Hausdorff") from exc
    pa = surface_samples(a_v, a_f, samples, 111)
    pb = surface_samples(b_v, b_f, samples, 222)
    d_ab = float(cKDTree(pb).query(pa, k=1, workers=-1)[0].max())
    d_ba = float(cKDTree(pa).query(pb, k=1, workers=-1)[0].max())
    diagonal = float(np.linalg.norm(a_v.max(axis=0) - a_v.min(axis=0)))
    worst = max(d_ab, d_ba)
    return {
        "sampled_hausdorff": worst,
        "sampled_hausdorff_fraction_of_original_diagonal": worst / diagonal if diagonal else 0.0,
        "sampled_original_to_candidate": d_ab,
        "sampled_candidate_to_original": d_ba,
        "samples_per_surface": samples,
    }


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("mesh", type=Path)
    parser.add_argument("--original", type=Path, help="estimate symmetric sampled Hausdorff to this mesh")
    parser.add_argument("--samples", type=int, default=18000)
    parser.add_argument("--pretty", action="store_true")
    args = parser.parse_args()

    vertices, faces = read_mesh(args.mesh)
    output = validate(vertices, faces)
    if args.original:
        ov, of = read_mesh(args.original)
        output.update(sampled_hausdorff(ov, of, vertices, faces, args.samples))
    print(json.dumps(output, indent=2 if args.pretty else None, sort_keys=True))


if __name__ == "__main__":
    main()
