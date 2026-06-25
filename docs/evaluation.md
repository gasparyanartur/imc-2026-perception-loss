# Evaluation

This document describes the offline evaluator (`evaluate.py`) and the
iteration harness (`evaluate.sh`) used to score candidate solutions for the
IMC 2026 mesh-simplification challenge.

It is a **preliminary** reimplementation of the official grading pipeline as
described in [`docs/report.md`](report.md) (sections 2.1–2.7). It is meant for
fast local iteration, not as a bit-exact clone of the competition grader.

---

## 1. What is being evaluated

The challenge asks for a simplified mesh `M'` derived from an original mesh `M`
that:

1. is a **closed watertight triangular 2-manifold** with non-degenerate
   (positive-area) faces and `1 ≤ |V'| ≤ |V|`;
2. stays within a **symmetric Hausdorff bound**, `d_H(M, M') ≤ 0.05 · Diagonal`;
3. reaches a **foreground multi-view SSIM** of `FinalSSIM ≥ 0.9`.

Valid submissions are then ranked purely by **vertex reduction**:

```
CompressionRate = 100 · (1 − |V'| / |V|)        (higher is better)
```

A submission that fails any of the three gates is **invalid** and is not ranked,
regardless of its compression rate.

---

## 2. The evaluator (`evaluate.py`)

### Mesh format

Both meshes use the challenge format: a `V F` header line, then `v x y z`
vertex lines, then `f i j k` face lines (faces are **1-indexed** on disk and
converted to 0-indexed internally).

### Validity gate

`check_validity()` enforces:

- vertex-count bound `1 ≤ |V'| ≤ |V|`;
- all face indices in range and three distinct vertices per face;
- positive triangle area (no degenerate faces);
- every **undirected** edge shared by exactly two faces (closed 2-manifold);
- every **directed** edge used exactly once (consistent orientation).

### Rendering

Each mesh is rendered from **six axial cameras** at distance `D = 2.5`:
`(±2.5, 0, 0)`, `(0, ±2.5, 0)`, `(0, 0, ±2.5)`, all looking at the origin. The
camera model is a 1024×1024 pinhole with `fx = fy = 800` and principal point
`(512, 512)`. Pixels are sampled once at their center and resolved with a
z-buffer (nearest triangle wins).

Two feature images are produced per view:

- **Normal map** — flat per-face unit normals mapped to RGB via
  `(n + 1) · 127.5`; background is neutral gray `(127.5, 127.5, 127.5)`.
- **Depth map** — perspective-correct depth `z = 1 / Σ(wᵢ / zᵢ)`; background
  depth is `255`.

> **Note on resolution.** The focal length is calibrated for the native 1024
> resolution. Lower `--resolution` values are useful for fast previews but crop
> the projected object and therefore change the score; use 1024 for scores that
> approximate the real grader.

### SSIM

SSIM uses an 11×11 sliding window with `k1 = 0.01`, `k2 = 0.03`, `L = 255`. Only
**foreground windows** are averaged — a window counts if its center pixel is
non-background in the original and/or the simplified render. Common background
is excluded. For RGB normal maps, SSIM is computed per channel and averaged.

The combined score per view is `0.5 · SSIM(normal) + 0.5 · SSIM(depth)`, and:

```
FinalSSIM = mean over the 6 views
```

### Hausdorff distance

`symmetric_hausdorff()` computes the **vertex-based** symmetric Hausdorff
distance (using SciPy's `cKDTree` when available, otherwise a chunked NumPy
fallback). This follows the practical, vertex-based interpretation in
`docs/report.md` §3.8 and is a conservative proxy, not a full surface guarantee.
The bound is `0.05 · Diagonal`, where `Diagonal` is the AABB diagonal of `M`.

### Output

`evaluate.py` produces one of three outputs:

- default: a human-readable report;
- `--quiet`: a single `VALID` / `INVALID` line;
- `--summary`: the report (unless combined with `--quiet`) followed by a stable,
  machine-readable `KEY=VALUE` block:

  ```
  RESULT=VALID|INVALID
  MANIFOLD_OK=0|1
  FINAL_SSIM=<float>
  SSIM_THRESHOLD=<float>
  HAUSDORFF=<float>
  HAUSDORFF_BOUND=<float>
  COMPRESSION_RATE=<float>
  ORIGINAL_VERTICES=<int>
  SIMPLIFIED_VERTICES=<int>
  ```

The process exits `0` for a valid submission and `1` otherwise.

### Usage

```sh
python3 evaluate.py ORIGINAL.txt SIMPLIFIED.txt              # full report
python3 evaluate.py ORIGINAL.txt SIMPLIFIED.txt --quiet      # VALID / INVALID
python3 evaluate.py ORIGINAL.txt SIMPLIFIED.txt --summary    # report + KEY=VALUE
python3 evaluate.py ORIGINAL.txt SIMPLIFIED.txt --resolution 256   # fast preview
```

Only NumPy is required; SciPy is optional and only accelerates the Hausdorff
step.

---

## 3. The iteration harness (`evaluate.sh`)

`evaluate.sh` is the end-to-end loop used while iterating on the Python solver.
It:

1. runs the solver (`solution.py` by default) on the input mesh to produce a
   simplified mesh;
2. scores it with `evaluate.py --summary`;
3. writes a timestamped log to `outputs/<date>-<result>.txt`, where `<result>`
   is the compression metric on success (e.g. `compr-11.1111`) or `invalid` /
   `error` on failure;
4. compares the new compression rate against the **best previous valid run**
   recorded in `outputs/` and reports whether the model improved.

### Configuration

| Variable      | Default                 | Meaning                          |
| ------------- | ----------------------- | -------------------------------- |
| `SCRIPT_FILE` | `solution.py`           | solver script to run             |
| `INPUT_PATH`  | `data/sample-input.txt` | original mesh fed to the solver  |
| `OUTPUTS_DIR` | `outputs`               | directory for logs               |
| `EVAL_SCRIPT` | `evaluate.py`           | evaluator script                 |
| `RESOLUTION`  | evaluator default       | render resolution                |
| `PYTHON`      | `python3`               | python interpreter               |

### Exit codes

- `0` — valid submission **and** strictly better than the previous best (or no
  previous valid run to compare against);
- `1` — invalid submission, solver/evaluator error, or no improvement vs. the
  best previous valid run.

### Usage

```sh
./evaluate.sh                          # default solver, input, resolution
RESOLUTION=1024 ./evaluate.sh          # native-resolution score
SCRIPT_FILE=solution.py INPUT_PATH=data/sample-input.txt ./evaluate.sh
```

The `outputs/` directory is git-ignored; logs accumulate there so that score
history (and the "did it improve?" check) persists across runs.
