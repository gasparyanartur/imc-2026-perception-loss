#!/usr/bin/env python3
"""Preliminary offline evaluator for the IMC 2026 mesh-simplification challenge.

This is a self-contained Python reimplementation of the evaluation pipeline
described in ``docs/report.md`` (sections 2.1--2.7). It is intended as a
*preliminary* reference evaluator for local iteration, not a bit-exact clone of
the official grader. The pipeline reproduces:

  * the input/output mesh format (``V F`` header, ``v x y z`` and ``f i j k``);
  * the six axial cameras at distance ``D = 2.5`` with a 1024x1024 pinhole
    model (``fx = fy = 800``, ``cu = cv = 512``);
  * flat-shaded face-normal maps and perspective-correct depth maps;
  * foreground-only 11x11 windowed SSIM, averaged per channel for normal maps;
  * the combined ``FinalSSIM`` score and the ``>= 0.9`` validity threshold;
  * the symmetric (vertex-based) Hausdorff distance against ``0.05 * Diagonal``;
  * the closed watertight triangular 2-manifold / positive-area validity gate;
  * the ``CompressionRate = 100 * (1 - |V'| / |V|)`` ranking objective.

Usage:
    python3 evaluate.py ORIGINAL.txt SIMPLIFIED.txt [--resolution N] [--quiet]

Only NumPy is required. SciPy's cKDTree is used for the Hausdorff distance when
available, otherwise a chunked NumPy fallback is used.
"""

from __future__ import annotations

import argparse
import sys
from dataclasses import dataclass

import numpy as np

# --- Evaluator constants (see docs/report.md, section 2.2-2.6) --------------

CAMERA_DISTANCE = 2.5          # D
RESOLUTION = 1024             # image is RESOLUTION x RESOLUTION
PRINCIPAL_POINT = RESOLUTION / 2.0  # (cu, cv) = (512, 512)
FOCAL_LENGTH = 800.0          # fx = fy
WINDOW = 11                   # SSIM sliding window
K1, K2, L = 0.01, 0.03, 255.0
C1 = (K1 * L) ** 2
C2 = (K2 * L) ** 2
BACKGROUND_DEPTH = 255.0
BACKGROUND_NORMAL = 127.5     # neutral gray, from n_b = (0, 0, 0)
SSIM_THRESHOLD = 0.9
HAUSDORFF_FRACTION = 0.05

# Six axial eye positions: (+-D, 0, 0), (0, +-D, 0), (0, 0, +-D).
EYES = np.array(
    [
        [CAMERA_DISTANCE, 0.0, 0.0],
        [-CAMERA_DISTANCE, 0.0, 0.0],
        [0.0, CAMERA_DISTANCE, 0.0],
        [0.0, -CAMERA_DISTANCE, 0.0],
        [0.0, 0.0, CAMERA_DISTANCE],
        [0.0, 0.0, -CAMERA_DISTANCE],
    ],
    dtype=np.float64,
)


# --- Mesh I/O ---------------------------------------------------------------

@dataclass
class Mesh:
    vertices: np.ndarray  # (n, 3) float64
    faces: np.ndarray     # (m, 3) int64, zero-indexed


def load_mesh(path: str) -> Mesh:
    """Parse the challenge mesh format (faces are 1-indexed on disk)."""
    with open(path, "rb") as handle:
        tok = handle.read().split()
    nv = int(tok[0])
    nf = int(tok[1])

    verts = np.empty((nv, 3), dtype=np.float64)
    p = 2
    for i in range(nv):
        # tokens: "v" x y z
        verts[i, 0] = float(tok[p + 1])
        verts[i, 1] = float(tok[p + 2])
        verts[i, 2] = float(tok[p + 3])
        p += 4

    faces = np.empty((nf, 3), dtype=np.int64)
    for i in range(nf):
        # tokens: "f" a b c (1-indexed)
        faces[i, 0] = int(tok[p + 1]) - 1
        faces[i, 1] = int(tok[p + 2]) - 1
        faces[i, 2] = int(tok[p + 3]) - 1
        p += 4

    return Mesh(verts, faces)


# --- Validity (closed watertight triangular 2-manifold) ---------------------

@dataclass
class Validity:
    ok: bool
    reasons: list


def check_validity(simplified: Mesh, original: Mesh) -> Validity:
    reasons = []
    nv = len(simplified.vertices)
    nv_orig = len(original.vertices)

    if not (1 <= nv <= nv_orig):
        reasons.append(
            "vertex count %d out of range [1, %d]" % (nv, nv_orig)
        )

    faces = simplified.faces
    if faces.size == 0:
        reasons.append("mesh has no faces")
        return Validity(False, reasons)

    if faces.min() < 0 or faces.max() >= nv:
        reasons.append("face references an out-of-range vertex index")
        return Validity(False, reasons)

    # Degenerate connectivity: a face must reference three distinct vertices.
    a, b, c = faces[:, 0], faces[:, 1], faces[:, 2]
    if np.any((a == b) | (b == c) | (a == c)):
        reasons.append("found a face with repeated vertex indices")

    # Positive area (non-degenerate geometry).
    v = simplified.vertices
    cross = np.cross(v[b] - v[a], v[c] - v[a])
    areas = 0.5 * np.linalg.norm(cross, axis=1)
    n_zero = int(np.count_nonzero(areas <= 1e-12))
    if n_zero:
        reasons.append("found %d zero-area (degenerate) face(s)" % n_zero)

    # Manifold / watertight check on directed and undirected edges.
    directed = np.concatenate(
        [faces[:, [0, 1]], faces[:, [1, 2]], faces[:, [2, 0]]], axis=0
    )
    undirected = np.sort(directed, axis=1)
    _, und_counts = np.unique(undirected, axis=0, return_counts=True)
    if np.any(und_counts != 2):
        reasons.append(
            "not a closed 2-manifold: %d edge(s) are not shared by exactly "
            "two faces" % int(np.count_nonzero(und_counts != 2))
        )

    _, dir_counts = np.unique(directed, axis=0, return_counts=True)
    if np.any(dir_counts != 1):
        reasons.append("inconsistent orientation: a directed edge is repeated")

    return Validity(len(reasons) == 0, reasons)


# --- Rendering --------------------------------------------------------------

def _camera_basis(eye: np.ndarray):
    """Right-handed look-at basis for a camera at ``eye`` facing the origin.

    Returns (right, up, forward) where ``forward`` points into the scene and a
    world point ``p`` has camera-space coordinates
    ``(dot(p-eye, right), dot(p-eye, up), dot(p-eye, forward))``.
    """
    forward = -eye / np.linalg.norm(eye)
    # Pick an up reference that is not parallel to the view direction.
    up_ref = np.array([0.0, 1.0, 0.0])
    if abs(float(np.dot(forward, up_ref))) > 0.9:
        up_ref = np.array([0.0, 0.0, 1.0])
    right = np.cross(up_ref, forward)
    right /= np.linalg.norm(right)
    up = np.cross(forward, right)
    return right, up, forward


def render_view(mesh: Mesh, eye: np.ndarray, resolution: int):
    """Rasterize a mesh from one camera.

    Returns ``(normal_map, depth_map, coverage)``:
      * ``normal_map`` : (H, W, 3) float64 in [0, 255], background = 127.5
      * ``depth_map``  : (H, W) float64, background = 255
      * ``coverage``   : (H, W) bool, True where a face was rasterized
    """
    res = resolution
    principal = res / 2.0
    right, up, forward = _camera_basis(eye)

    verts = mesh.vertices
    faces = mesh.faces

    rel = verts - eye
    cam_x = rel @ right
    cam_y = rel @ up
    cam_z = rel @ forward  # depth; > 0 in front of the camera

    # Perspective projection to pixel coordinates.
    with np.errstate(divide="ignore", invalid="ignore"):
        proj_u = FOCAL_LENGTH * cam_x / cam_z + principal
        proj_v = FOCAL_LENGTH * cam_y / cam_z + principal

    # Flat face normals (docs/report.md 2.3).
    a, b, c = faces[:, 0], faces[:, 1], faces[:, 2]
    face_normals = np.cross(verts[b] - verts[a], verts[c] - verts[a])
    norms = np.linalg.norm(face_normals, axis=1, keepdims=True)
    norms[norms == 0.0] = 1.0
    face_normals = face_normals / norms

    normal_map = np.full((res, res, 3), BACKGROUND_NORMAL, dtype=np.float64)
    depth_map = np.full((res, res), BACKGROUND_DEPTH, dtype=np.float64)
    zbuffer = np.full((res, res), np.inf, dtype=np.float64)
    coverage = np.zeros((res, res), dtype=bool)

    for fi in range(len(faces)):
        i0, i1, i2 = a[fi], b[fi], c[fi]
        z0, z1, z2 = cam_z[i0], cam_z[i1], cam_z[i2]
        # Skip faces with any vertex at or behind the camera plane.
        if z0 <= 1e-9 or z1 <= 1e-9 or z2 <= 1e-9:
            continue

        u0, u1, u2 = proj_u[i0], proj_u[i1], proj_u[i2]
        v0, v1, v2 = proj_v[i0], proj_v[i1], proj_v[i2]

        umin = int(np.floor(min(u0, u1, u2)))
        umax = int(np.ceil(max(u0, u1, u2)))
        vmin = int(np.floor(min(v0, v1, v2)))
        vmax = int(np.ceil(max(v0, v1, v2)))
        umin = max(umin, 0)
        vmin = max(vmin, 0)
        umax = min(umax, res - 1)
        vmax = min(vmax, res - 1)
        if umin > umax or vmin > vmax:
            continue

        # Edge-function (barycentric) setup in screen space.
        denom = (v1 - v2) * (u0 - u2) + (u2 - u1) * (v0 - v2)
        if abs(denom) < 1e-12:
            continue
        inv_denom = 1.0 / denom

        us = np.arange(umin, umax + 1) + 0.5  # pixel centers
        vs = np.arange(vmin, vmax + 1) + 0.5
        grid_u, grid_v = np.meshgrid(us, vs)

        w0 = ((v1 - v2) * (grid_u - u2) + (u2 - u1) * (grid_v - v2)) * inv_denom
        w1 = ((v2 - v0) * (grid_u - u2) + (u0 - u2) * (grid_v - v2)) * inv_denom
        w2 = 1.0 - w0 - w1

        inside = (w0 >= 0.0) & (w1 >= 0.0) & (w2 >= 0.0)
        if not inside.any():
            continue

        # Perspective-correct depth: z = 1 / sum(w_i / z_i) (docs 2.4).
        inv_z = w0 / z0 + w1 / z1 + w2 / z2
        with np.errstate(divide="ignore", invalid="ignore"):
            depth = 1.0 / inv_z

        rows = (np.arange(vmin, vmax + 1)[:, None]
                * np.ones((1, umax - umin + 1), dtype=int))
        cols = (np.ones((vmax - vmin + 1, 1), dtype=int)
                * np.arange(umin, umax + 1)[None, :])

        sel = inside & (depth < zbuffer[rows, cols])
        if not sel.any():
            continue

        r = rows[sel]
        col = cols[sel]
        zbuffer[r, col] = depth[sel]
        depth_map[r, col] = depth[sel]
        normal_map[r, col, :] = (face_normals[fi] + 1.0) * 127.5
        coverage[r, col] = True

    return normal_map, depth_map, coverage


# --- SSIM -------------------------------------------------------------------

def _box_sum(img: np.ndarray, win: int) -> np.ndarray:
    """Sum over every ``win`` x ``win`` window (valid mode) via integral image."""
    integral = np.zeros((img.shape[0] + 1, img.shape[1] + 1), dtype=np.float64)
    integral[1:, 1:] = np.cumsum(np.cumsum(img, axis=0), axis=1)
    return (
        integral[win:, win:]
        - integral[:-win, win:]
        - integral[win:, :-win]
        + integral[:-win, :-win]
    )


def _channel_ssim_map(x: np.ndarray, y: np.ndarray, win: int):
    """Per-window SSIM map for a single channel (valid windows only)."""
    n = win * win
    mu_x = _box_sum(x, win) / n
    mu_y = _box_sum(y, win) / n
    mu_xx = _box_sum(x * x, win) / n
    mu_yy = _box_sum(y * y, win) / n
    mu_xy = _box_sum(x * y, win) / n

    var_x = mu_xx - mu_x * mu_x
    var_y = mu_yy - mu_y * mu_y
    cov_xy = mu_xy - mu_x * mu_y

    numerator = (2 * mu_x * mu_y + C1) * (2 * cov_xy + C2)
    denominator = (mu_x * mu_x + mu_y * mu_y + C1) * (var_x + var_y + C2)
    return numerator / denominator


def foreground_ssim(orig: np.ndarray, simp: np.ndarray,
                    foreground: np.ndarray, win: int = WINDOW) -> float:
    """Foreground-only windowed SSIM.

    A window is counted if the center pixel is foreground in the original
    and/or simplified render. For multi-channel images, SSIM is computed per
    channel and averaged.
    """
    if orig.ndim == 2:
        orig = orig[:, :, None]
        simp = simp[:, :, None]

    # Foreground mask at window centers (valid-window coordinates).
    half = win // 2
    center_mask = foreground[half:half + orig.shape[0] - win + 1,
                             half:half + orig.shape[1] - win + 1]
    if not center_mask.any():
        return 1.0

    channel_scores = []
    for ch in range(orig.shape[2]):
        ssim_map = _channel_ssim_map(orig[:, :, ch], simp[:, :, ch], win)
        channel_scores.append(float(ssim_map[center_mask].mean()))
    return float(np.mean(channel_scores))


# --- Hausdorff --------------------------------------------------------------

def _nearest_distances(query: np.ndarray, reference: np.ndarray) -> np.ndarray:
    """Min Euclidean distance from each ``query`` point to ``reference`` set."""
    try:
        from scipy.spatial import cKDTree  # type: ignore

        tree = cKDTree(reference)
        dist, _ = tree.query(query, k=1)
        return dist
    except Exception:
        # Chunked brute-force fallback (NumPy only).
        out = np.empty(len(query), dtype=np.float64)
        chunk = max(1, 2_000_000 // max(1, len(reference)))
        for start in range(0, len(query), chunk):
            block = query[start:start + chunk]
            d = np.linalg.norm(block[:, None, :] - reference[None, :, :], axis=2)
            out[start:start + chunk] = d.min(axis=1)
        return out


def symmetric_hausdorff(a: np.ndarray, b: np.ndarray) -> float:
    """Vertex-based symmetric Hausdorff distance (preliminary proxy).

    This uses mesh vertices as the point sets, matching the practical,
    vertex-based interpretation in docs/report.md (section 3.8). It is not a
    full surface Hausdorff guarantee.
    """
    forward = _nearest_distances(a, b).max()
    backward = _nearest_distances(b, a).max()
    return float(max(forward, backward))


def aabb_diagonal(vertices: np.ndarray) -> float:
    extent = vertices.max(axis=0) - vertices.min(axis=0)
    return float(np.linalg.norm(extent))


# --- Top-level evaluation ---------------------------------------------------

@dataclass
class Result:
    valid: bool
    final_ssim: float
    per_view: list
    hausdorff: float
    hausdorff_bound: float
    compression_rate: float
    validity: Validity


def evaluate(original: Mesh, simplified: Mesh, resolution: int = RESOLUTION):
    validity = check_validity(simplified, original)

    per_view = []
    ssim_accum = 0.0
    for eye in EYES:
        n_orig, d_orig, cov_orig = render_view(original, eye, resolution)
        n_simp, d_simp, cov_simp = render_view(simplified, eye, resolution)
        foreground = cov_orig | cov_simp

        ssim_normal = foreground_ssim(n_orig, n_simp, foreground)
        ssim_depth = foreground_ssim(d_orig, d_simp, foreground)
        view_score = 0.5 * ssim_normal + 0.5 * ssim_depth
        ssim_accum += view_score
        per_view.append(
            {
                "eye": eye.tolist(),
                "ssim_normal": ssim_normal,
                "ssim_depth": ssim_depth,
                "view_score": view_score,
            }
        )

    final_ssim = ssim_accum / len(EYES)

    hausdorff = symmetric_hausdorff(original.vertices, simplified.vertices)
    bound = HAUSDORFF_FRACTION * aabb_diagonal(original.vertices)

    compression = 100.0 * (1.0 - len(simplified.vertices)
                           / max(1, len(original.vertices)))

    valid = (
        validity.ok
        and final_ssim >= SSIM_THRESHOLD
        and hausdorff <= bound
    )

    return Result(
        valid=valid,
        final_ssim=final_ssim,
        per_view=per_view,
        hausdorff=hausdorff,
        hausdorff_bound=bound,
        compression_rate=compression,
        validity=validity,
    )


def _print_report(result: Result, original: Mesh, simplified: Mesh) -> None:
    print("=" * 60)
    print("IMC 2026 preliminary evaluation")
    print("=" * 60)
    print("Original  : %d vertices, %d faces"
          % (len(original.vertices), len(original.faces)))
    print("Simplified: %d vertices, %d faces"
          % (len(simplified.vertices), len(simplified.faces)))
    print("-" * 60)

    print("Manifold/validity gate: %s"
          % ("PASS" if result.validity.ok else "FAIL"))
    for reason in result.validity.reasons:
        print("  - %s" % reason)

    print("-" * 60)
    print("Per-view SSIM (normal / depth / combined):")
    for view in result.per_view:
        ex, ey, ez = view["eye"]
        print("  eye (%+.1f, %+.1f, %+.1f): %.4f / %.4f / %.4f"
              % (ex, ey, ez, view["ssim_normal"],
                 view["ssim_depth"], view["view_score"]))

    print("-" * 60)
    print("FinalSSIM        : %.6f (threshold >= %.2f -> %s)"
          % (result.final_ssim, SSIM_THRESHOLD,
             "PASS" if result.final_ssim >= SSIM_THRESHOLD else "FAIL"))
    print("Hausdorff (sym.) : %.6f (bound <= %.6f -> %s)"
          % (result.hausdorff, result.hausdorff_bound,
             "PASS" if result.hausdorff <= result.hausdorff_bound else "FAIL"))
    print("CompressionRate  : %.4f %%" % result.compression_rate)
    print("-" * 60)
    print("VALID SUBMISSION : %s" % ("YES" if result.valid else "NO"))
    print("=" * 60)


def _print_summary(result: Result, original: Mesh, simplified: Mesh) -> None:
    """Print a machine-readable ``KEY=VALUE`` block (one pair per line).

    This is the contract relied on by ``evaluate.sh``; keep the keys stable.
    """
    print("RESULT=%s" % ("VALID" if result.valid else "INVALID"))
    print("MANIFOLD_OK=%s" % ("1" if result.validity.ok else "0"))
    print("FINAL_SSIM=%.6f" % result.final_ssim)
    print("SSIM_THRESHOLD=%.6f" % SSIM_THRESHOLD)
    print("HAUSDORFF=%.6f" % result.hausdorff)
    print("HAUSDORFF_BOUND=%.6f" % result.hausdorff_bound)
    print("COMPRESSION_RATE=%.6f" % result.compression_rate)
    print("ORIGINAL_VERTICES=%d" % len(original.vertices))
    print("SIMPLIFIED_VERTICES=%d" % len(simplified.vertices))


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(
        description="Preliminary offline evaluator for the IMC 2026 "
                    "mesh-simplification challenge."
    )
    parser.add_argument("original", help="path to the original mesh")
    parser.add_argument("simplified", help="path to the simplified mesh")
    parser.add_argument("--resolution", type=int, default=RESOLUTION,
                        help="render resolution (default: %d)" % RESOLUTION)
    parser.add_argument("--quiet", action="store_true",
                        help="print only the final valid/invalid verdict")
    parser.add_argument("--summary", action="store_true",
                        help="print a machine-readable KEY=VALUE summary block")
    args = parser.parse_args(argv)

    original = load_mesh(args.original)
    simplified = load_mesh(args.simplified)
    result = evaluate(original, simplified, resolution=args.resolution)

    if args.summary:
        if not args.quiet:
            _print_report(result, original, simplified)
        _print_summary(result, original, simplified)
    elif args.quiet:
        print("VALID" if result.valid else "INVALID")
    else:
        _print_report(result, original, simplified)

    return 0 if result.valid else 1


if __name__ == "__main__":
    sys.exit(main())
