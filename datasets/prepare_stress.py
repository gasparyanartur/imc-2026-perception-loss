#!/usr/bin/env python3
"""Generate large, clean closed-manifold *stress* meshes for runtime testing.

The representative `data/ppsurf` set maxes out near ~10k vertices, far below the
grader's ~1.1M-vertex / 2.1M-face ceiling and its 21-second budget. A solver can
therefore look perfect locally while timing out on the real grader. These stress
meshes exist to exercise the solver's **runtime and memory** at grader scale.

The mesh is a **torus grid**: a `nu` x `nv` lattice wrapped around a torus.

    x = (R + r cos phi) cos theta
    y = (R + r cos phi) sin theta
    z =  r sin phi
    theta = 2*pi*i/nu,  phi = 2*pi*j/nv

Each grid cell becomes two triangles with a single consistent winding. This is a
closed watertight triangular 2-manifold (it wraps in both directions, so every
edge is shared by exactly two faces) with **no poles and no sliver triangles**,
so - unlike midpoint subdivision of an arbitrary seed - it never produces
degenerate (zero-area) faces. The vertices are scaled into the unit sphere to
match the challenge's input convention (centered, radius <= 1).

Vertex count is exactly ``nu * nv``; pass a target with ``--target-vertices`` and
the tool picks a near-square grid.

Usage::

    python3 datasets/prepare_stress.py --target-vertices 150000 \\
        --out data/stress/stress_150k.txt

Sizes worth generating: 150k (mid), 600k (large), 1050k (~grader ceiling).
"""

from __future__ import annotations

import argparse
import math
import os
import sys


def torus_mesh(nu, nv, major=1.0, minor=0.4):
    """Build a torus grid with ``nu`` x ``nv`` vertices.

    Returns ``(vertices, faces)`` with 0-indexed faces, normalized into the unit
    sphere. ``major > minor > 0`` keeps the tube from self-intersecting.
    """
    verts = [None] * (nu * nv)
    inv_scale = 1.0 / (major + minor)  # fit the outer radius into the unit ball
    two_pi = 2.0 * math.pi
    for i in range(nu):
        theta = two_pi * i / nu
        ct, st = math.cos(theta), math.sin(theta)
        for j in range(nv):
            phi = two_pi * j / nv
            ring = major + minor * math.cos(phi)
            x = ring * ct
            y = ring * st
            z = minor * math.sin(phi)
            verts[i * nv + j] = (x * inv_scale, y * inv_scale, z * inv_scale)

    faces = []
    for i in range(nu):
        i1 = (i + 1) % nu
        for j in range(nv):
            j1 = (j + 1) % nv
            a = i * nv + j
            b = i1 * nv + j
            c = i1 * nv + j1
            d = i * nv + j1
            # Two triangles per quad, consistent CCW winding.
            faces.append((a, b, c))
            faces.append((a, c, d))
    return verts, faces


def grid_for_target(target):
    """Pick a near-square (nu, nv) whose product is closest to ``target``."""
    nu = max(3, int(round(math.sqrt(target))))
    nv = max(3, int(round(target / nu)))
    return nu, nv


def save_mesh(path, verts, faces):
    os.makedirs(os.path.dirname(os.path.abspath(path)), exist_ok=True)
    out = ["%d %d" % (len(verts), len(faces))]
    out += ["v %.10g %.10g %.10g" % v for v in verts]
    out += ["f %d %d %d" % (a + 1, b + 1, c + 1) for (a, b, c) in faces]
    with open(path, "w") as handle:
        handle.write("\n".join(out))
        handle.write("\n")


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--target-vertices", type=int, default=150000,
                        help="approximate vertex count (a near-square grid)")
    parser.add_argument("--rows", type=int, default=0,
                        help="explicit nu (overrides --target-vertices)")
    parser.add_argument("--cols", type=int, default=0,
                        help="explicit nv (overrides --target-vertices)")
    parser.add_argument("--major", type=float, default=1.0,
                        help="torus major radius")
    parser.add_argument("--minor", type=float, default=0.4,
                        help="torus minor radius (must be < major)")
    parser.add_argument("--out", required=True, help="output mesh path")
    args = parser.parse_args(argv)

    if args.rows and args.cols:
        nu, nv = args.rows, args.cols
    else:
        nu, nv = grid_for_target(args.target_vertices)

    verts, faces = torus_mesh(nu, nv, major=args.major, minor=args.minor)
    save_mesh(args.out, verts, faces)
    print("wrote %s: %d vertices, %d faces (grid %d x %d)"
          % (args.out, len(verts), len(faces), nu, nv))
    return 0


if __name__ == "__main__":
    sys.exit(main())
