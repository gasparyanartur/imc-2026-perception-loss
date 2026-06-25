#!/usr/bin/env python3
"""Build a small representative challenge dataset from the ppsurf meshes.

The IMC 2026 challenge evaluates a mesh simplifier on closed watertight
triangular 2-manifolds that are centered at the origin and scaled into the unit
sphere (see ``docs/report.md`` section 2.1). This script turns ground-truth
surface meshes from the **ppsurf** dataset
(https://huggingface.co/datasets/perler/ppsurf, mirrored in the
``cg-tuwien/ppsurf`` GitHub repository) into that format and selects a small,
size-diverse, **representative** subset for offline evaluation.

For each source mesh it:

  1. loads the mesh (via ``trimesh``);
  2. recenters it on its bounding-box center and scales it so the farthest
     vertex lies on the unit sphere (``max||v|| = 1``, hence within
     ``[-1, 1]``);
  3. validates that the result is a closed watertight triangular 2-manifold
     with positive-area faces (the challenge's input guarantee);
  4. writes it in the challenge mesh format (``V F`` header, ``v x y z`` and
     1-indexed ``f i j k`` lines).

Finally it picks ``--num`` meshes spanning the vertex-count distribution so the
representative set exercises both small and large inputs, and records their
provenance in ``MANIFEST.md``.

Usage::

    # Point at a local ppsurf checkout (e.g. its 03_meshes folders):
    python3 datasets/prepare_ppsurf.py --source /path/to/ppsurf/datasets

    # Or several explicit source directories / files:
    python3 datasets/prepare_ppsurf.py --source a/03_meshes b/03_meshes \
        --out data/ppsurf --num 20

Obtaining the ppsurf meshes:

  * A 10-mesh minimal set ships inside the ppsurf GitHub repo under
    ``datasets/abc_minimal/03_meshes`` (clone ``cg-tuwien/ppsurf``).
  * The full ABC / Famous / Thingi10k test sets are fetched by ppsurf's own
    ``datasets/download_testsets.py`` (HuggingFace / TU Wien hosted); run that
    first, then point ``--source`` at the extracted ``*/03_meshes`` folders to
    grow the representative set toward ``--num``.

``trimesh`` is required for this *preparation* step only (``pip install
trimesh``); the evaluator itself needs only NumPy.
"""

from __future__ import annotations

import argparse
import os
import sys

MESH_EXTENSIONS = (".ply", ".obj", ".stl", ".off")
AREA_EPS = 1e-12


def find_mesh_files(sources):
    """Return a sorted list of mesh files under the given files/directories."""
    files = []
    for src in sources:
        if os.path.isfile(src):
            if src.lower().endswith(MESH_EXTENSIONS):
                files.append(src)
        elif os.path.isdir(src):
            for root, _dirs, names in os.walk(src):
                for name in names:
                    if name.lower().endswith(MESH_EXTENSIONS):
                        files.append(os.path.join(root, name))
        else:
            print("warning: source not found: %s" % src, file=sys.stderr)
    # Deduplicate while keeping a deterministic order.
    return sorted(set(files))


def normalize(vertices):
    """Center on the bounding-box center and scale into the unit sphere."""
    import numpy as np

    v = np.asarray(vertices, dtype=np.float64)
    center = (v.max(axis=0) + v.min(axis=0)) / 2.0
    v = v - center
    radius = float(np.linalg.norm(v, axis=1).max())
    if radius <= 0.0:
        raise ValueError("degenerate mesh (zero radius)")
    return v / radius


def is_valid_manifold(vertices, faces):
    """Check the challenge input guarantee: closed watertight 2-manifold."""
    import numpy as np

    v = np.asarray(vertices, dtype=np.float64)
    f = np.asarray(faces, dtype=np.int64)
    if len(f) == 0:
        return False, "no faces"
    if f.min() < 0 or f.max() >= len(v):
        return False, "face index out of range"

    a, b, c = f[:, 0], f[:, 1], f[:, 2]
    if np.any((a == b) | (b == c) | (a == c)):
        return False, "face with repeated vertex"

    cross = np.cross(v[b] - v[a], v[c] - v[a])
    areas = 0.5 * np.linalg.norm(cross, axis=1)
    if np.any(areas <= AREA_EPS):
        return False, "degenerate (zero-area) face"

    directed = np.concatenate([f[:, [0, 1]], f[:, [1, 2]], f[:, [2, 0]]], axis=0)
    undirected = np.sort(directed, axis=1)
    _, und_counts = np.unique(undirected, axis=0, return_counts=True)
    if np.any(und_counts != 2):
        return False, "not closed (edge not shared by exactly two faces)"

    _, dir_counts = np.unique(directed, axis=0, return_counts=True)
    if np.any(dir_counts != 1):
        return False, "inconsistent orientation"

    return True, "ok"


def write_challenge_mesh(path, vertices, faces):
    """Write a mesh in the challenge format (faces are 1-indexed on disk)."""
    lines = ["%d %d" % (len(vertices), len(faces))]
    lines += ["v %.10g %.10g %.10g" % (x, y, z) for (x, y, z) in vertices]
    lines += ["f %d %d %d" % (a + 1, b + 1, c + 1) for (a, b, c) in faces]
    with open(path, "w", encoding="utf-8") as handle:
        handle.write("\n".join(lines))
        handle.write("\n")


def representative_indices(sizes, num):
    """Indices of ``num`` meshes spanning the sorted vertex-count range.

    Sorting by size and sampling evenly keeps the subset diverse (small to
    large) and deterministic.
    """
    order = sorted(range(len(sizes)), key=lambda i: (sizes[i], i))
    if num >= len(order):
        return order
    picked = []
    for k in range(num):
        pos = round(k * (len(order) - 1) / (num - 1)) if num > 1 else 0
        picked.append(order[pos])
    # Deduplicate (rounding may collide) while preserving order.
    seen = set()
    unique = []
    for i in picked:
        if i not in seen:
            seen.add(i)
            unique.append(i)
    # If collisions shrank the set, top up with the next unused sizes.
    if len(unique) < num:
        for i in order:
            if i not in seen:
                unique.append(i)
                seen.add(i)
                if len(unique) >= num:
                    break
    return unique


def derive_name(source_path):
    """Short, traceable output name from a ppsurf source filename."""
    base = os.path.basename(source_path)
    for ext in MESH_EXTENSIONS:
        if base.lower().endswith(ext):
            base = base[: -len(ext)]
            break
    # ABC meshes look like "00010009_<hash>_trimesh_000"; keep the leading id.
    token = base.split("_", 1)[0]
    if token.isdigit():
        return "abc_%s" % token
    # Fall back to a sanitized full stem.
    safe = "".join(ch if ch.isalnum() else "_" for ch in base)
    return safe[:48]


def main(argv=None):
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument("--source", nargs="+", required=True,
                        help="ppsurf mesh files or directories to scan")
    parser.add_argument("--out", default="data/ppsurf",
                        help="output directory (default: data/ppsurf)")
    parser.add_argument("--num", type=int, default=20,
                        help="number of representative meshes (default: 20)")
    args = parser.parse_args(argv)

    try:
        import trimesh  # noqa: F401
    except ImportError:
        print("error: trimesh is required for dataset preparation "
              "(pip install trimesh)", file=sys.stderr)
        return 2

    import numpy as np  # noqa: F401
    import trimesh

    files = find_mesh_files(args.source)
    if not files:
        print("error: no mesh files found under the given sources",
              file=sys.stderr)
        return 1

    # Load, normalize and validate every candidate.
    candidates = []  # (name, vertices, faces, source)
    used_names = {}
    for path in files:
        try:
            mesh = trimesh.load(path, process=False, force="mesh")
            verts = normalize(mesh.vertices)
            faces = np.asarray(mesh.faces, dtype=np.int64)
        except Exception as exc:  # noqa: BLE001 - report and skip bad meshes
            print("skip %s (%s)" % (path, exc), file=sys.stderr)
            continue

        ok, reason = is_valid_manifold(verts, faces)
        if not ok:
            print("skip %s (not a valid challenge mesh: %s)" % (path, reason),
                  file=sys.stderr)
            continue

        name = derive_name(path)
        # Ensure unique output names.
        if name in used_names:
            used_names[name] += 1
            name = "%s_%d" % (name, used_names[name])
        else:
            used_names[name] = 0
        candidates.append((name, verts, faces, path))

    if not candidates:
        print("error: no valid challenge meshes after normalization",
              file=sys.stderr)
        return 1

    sizes = [len(c[1]) for c in candidates]
    chosen = representative_indices(sizes, args.num)
    chosen_sorted = sorted(chosen, key=lambda i: (len(candidates[i][1]), i))

    os.makedirs(args.out, exist_ok=True)
    manifest = [
        "# ppsurf representative dataset",
        "",
        "Generated by `datasets/prepare_ppsurf.py` from the ppsurf dataset",
        "(https://huggingface.co/datasets/perler/ppsurf). Each mesh is recentered",
        "on its bounding-box center and scaled into the unit sphere, then written",
        "in the challenge format (`V F` header, `v x y z`, 1-indexed `f i j k`).",
        "",
        "| file | vertices | faces | source |",
        "| ---- | -------: | ----: | ------ |",
    ]
    written = 0
    for i in chosen_sorted:
        name, verts, faces, source = candidates[i]
        out_path = os.path.join(args.out, name + ".txt")
        write_challenge_mesh(out_path, verts, faces)
        manifest.append("| %s.txt | %d | %d | %s |"
                        % (name, len(verts), len(faces),
                           os.path.basename(source)))
        written += 1

    with open(os.path.join(args.out, "MANIFEST.md"), "w", encoding="utf-8") as fh:
        fh.write("\n".join(manifest) + "\n")

    print("wrote %d representative meshes to %s (from %d candidates)"
          % (written, args.out, len(candidates)))
    if written < args.num:
        print("note: only %d valid ppsurf meshes were available; download the "
              "full ppsurf test sets and re-run with more --source folders to "
              "reach %d." % (written, args.num), file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
