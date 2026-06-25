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

## 3. The representative dataset (`data/ppsurf`)

A solver must generalize across many meshes, so the harness scores it on a
**representative dataset** rather than a single mesh. Evaluating on one trivial
mesh (e.g. a cube) hides solvers that fail on real geometry — the
"passes N/M scenarios" situation.

`data/ppsurf/` holds meshes derived from the **ppsurf** dataset
(<https://huggingface.co/datasets/perler/ppsurf>, also mirrored in the
`cg-tuwien/ppsurf` GitHub repo). Each mesh is recentered on its bounding-box
center and scaled into the unit sphere, then written in the challenge format
(`V F` header, `v x y z`, 1-indexed `f i j k`). `data/ppsurf/MANIFEST.md`
records each file's vertex/face counts and its source mesh.

### Regenerating / growing the dataset

`datasets/prepare_ppsurf.py` builds the dataset from ppsurf source meshes:

```sh
pip install trimesh   # required for preparation only
python3 datasets/prepare_ppsurf.py --source /path/to/ppsurf/datasets \
    --out data/ppsurf --num 20
```

It loads every `.ply`/`.obj`/`.stl`/`.off` under `--source`, normalizes and
validates it (closed watertight 2-manifold, positive-area faces), and selects
`--num` meshes spanning the vertex-count range so the set stays diverse.

Obtaining ppsurf meshes:

- A 10-mesh minimal set ships inside the ppsurf GitHub repo under
  `datasets/abc_minimal/03_meshes` (clone `cg-tuwien/ppsurf`).
- The full ABC / Famous / Thingi10k test sets are fetched by ppsurf's own
  `datasets/download_testsets.py`; run that, then pass the extracted
  `*/03_meshes` folders to `--source` to grow the set toward `--num`.

---

## 4. The multi-sample evaluator (`evaluate_dataset.py`)

`evaluate_dataset.py` runs a solver across a whole dataset and aggregates the
results. For each input mesh it:

1. runs `SOLVER < input > simplified` (a subprocess, like the real grader);
2. scores the pair with `evaluate.py`'s `evaluate()`;
3. records the per-scenario verdict, compression, Hausdorff and SSIM.

It prints a per-scenario table and a machine-readable `KEY=VALUE` block. The
overall `RESULT` is `VALID` only when **every** scenario is valid; the aggregate
`COMPRESSION_RATE` is the **mean** over all scenarios:

```
RESULT=VALID|INVALID
SCENARIOS_TOTAL=<int>
SCENARIOS_PASSED=<int>
MEAN_COMPRESSION_RATE=<float>
MIN_COMPRESSION_RATE=<float>
COMPRESSION_RATE=<float>        # alias for the mean (kept stable for tooling)
```

The process exits `0` only if every scenario passed.

### Usage

```sh
python3 evaluate_dataset.py --dataset data/ppsurf --summary
python3 evaluate_dataset.py --solver solution.py --dataset data/ppsurf \
    --resolution 1024 --summary
```

The default render resolution for the multi-mesh harness is `256` (fast
iteration over many meshes); pass `--resolution 1024` for real-grader-like
scores. Only NumPy is required (SciPy speeds up the Hausdorff step).

---

## 5. The iteration harness (`evaluate.sh`)

`evaluate.sh` is the end-to-end loop used while iterating on the Python solver.
It:

1. runs the solver (`solution.py` by default) on **every** mesh in the dataset
   directory and scores each with `evaluate.py` (via `evaluate_dataset.py`);
2. aggregates the per-scenario verdicts — the submission is `VALID` only when
   all scenarios pass; the reported `CompressionRate` is the mean over all
   scenarios;
3. writes a timestamped log to `outputs/<date>-<result>.txt`, where `<result>`
   is the mean compression metric on success (e.g. `compr-78.2938`) or
   `invalid` / `error` on failure;
4. compares the new mean compression rate against the **best previous valid
   run** recorded in `outputs/` and reports whether the model improved.

### Configuration

| Variable      | Default               | Meaning                          |
| ------------- | --------------------- | -------------------------------- |
| `SCRIPT_FILE` | `solution.py`         | solver script to run             |
| `DATASET_DIR` | `data/ppsurf`         | directory of input meshes        |
| `OUTPUTS_DIR` | `outputs`             | directory for logs               |
| `EVAL_SCRIPT` | `evaluate_dataset.py` | dataset evaluator script         |
| `RESOLUTION`  | evaluator default     | render resolution                |
| `PYTHON`      | `python3`             | python interpreter               |

### Exit codes

- `0` — valid submission (all scenarios pass) **and** strictly better mean
  compression than the previous best (or no previous valid run to compare
  against);
- `1` — invalid submission (one or more scenarios failed), solver/evaluator
  error, or no improvement vs. the best previous valid run.

### Usage

```sh
./evaluate.sh                          # default solver, dataset, resolution
RESOLUTION=1024 ./evaluate.sh          # native-resolution score
SCRIPT_FILE=solution.py DATASET_DIR=data/ppsurf ./evaluate.sh
```

The `outputs/` directory is git-ignored; logs accumulate there so that score
history (and the "did it improve?" check) persists across runs.
