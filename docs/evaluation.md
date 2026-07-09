# Evaluation

This document specifies the local evaluation pipeline for the IMC 2026 mesh
simplification challenge. The native evaluator is implemented in C++ and the
only supported candidate format is a C++ source file.

The local evaluator is intended to provide repeatable engineering feedback.
The official grader remains authoritative for final scores.

## 1. Evaluation gates

For each input mesh, a candidate must satisfy every gate:

1. The candidate finishes within the configured per-mesh time budget.
2. The output uses the challenge mesh format and has a valid vertex count.
3. The output is a closed, watertight, consistently oriented triangular
   2-manifold with positive-area faces.
4. The sampled symmetric surface Hausdorff distance is within `0.05 · D`, where
   `D` is the original mesh's axis-aligned bounding-box diagonal.
5. The native six-view `FinalSSIM` is at least `0.9`.

The reported objective is:

$$
\mathrm{CompressionRate} = 100\left(1 - \frac{|V'|}{|V|}\right),
$$

where $V$ and $V'$ are the original and simplified vertex sets.

A submission is locally valid only when every evaluated scenario passes.

## 3. Native rendering and SSIM

The native diagnostic renders both meshes from six axial cameras at distance
$2.5$:

$$
(\pm 2.5,0,0),\quad (0,\pm 2.5,0),\quad (0,0,\pm 2.5).
$$

The default render is 1024 × 1024 with focal length 800 and principal point
(512, 512). Rendering uses flat per-face normal maps, perspective-correct
depth, a nearest-triangle z-buffer, and supersampling.

SSIM uses an 11 × 11 Gaussian window with $K_1=0.01$, $K_2=0.03$, and
$L=255$. Foreground windows are averaged; normal-map channels are averaged
before combining normal and depth scores. The six view scores are averaged to
produce `FinalSSIM`.

The native diagnostic also reports:

- aggregate normal SSIM;
- aggregate depth SSIM;
- per-view normal, depth, and combined SSIM;
- sampled surface Hausdorff distance and its limit;
- compression and solve time.

The 256-pixel diagnostic is available for quick experiments only. It is not a
promotion or submission criterion.

## 4. Evaluator components

The source files under `evaluators/` are compiled into `build/evaluators/`:

- `diagnostic_v3.cpp` — native 1024-pixel SSIM diagnostic;
- `diag_small.cpp` — faster low-resolution diagnostic;
- `hausdorff_validator.cpp` — sampled bidirectional surface distance;
- `mesh_validity.cpp` — native topology, indexing, degeneracy, and vertex-count
  validation.

Build them with:

```sh
scripts/build-evaluators.sh
```

The orchestration command runs the candidate once per input mesh, writes a
temporary output, invokes the native diagnostics, and aggregates the results.
It does not execute candidate scripts or interpret non-C++ source files.

## 5. Datasets

The default suite is `data/ppsurf/`, a representative collection of closed
meshes spanning multiple sizes and geometric characteristics. A candidate must
pass every mesh in the suite.

The synthetic suite in `data/synth_bench/` contains targeted shapes for tier,
feature, silhouette, and renderer diagnostics. It is enabled explicitly and
should be used in addition to, not instead of, the representative suite.

Large stress meshes can be generated under the ignored `data/stress/` path.
Stress evaluation is useful for memory, runtime, topology, and Hausdorff
behavior at grader scale. Full native rendering should be reserved for meshes
that fit within the local resource budget.

## 6. Commands

Evaluate a C++ candidate on the representative suite:

```sh
scripts/evaluate.sh --candidate solutions/lemon/v115.cpp
```

Include synthetic diagnostics and save machine-readable records:

```sh
scripts/evaluate.sh --candidate solutions/lemon/v115.cpp \
    --include-synthetic --json outputs/latest.json
```

Evaluate custom files or directories by repeating `--dataset`:

```sh
scripts/evaluate.sh --candidate path/to/solution.cpp \
    --dataset path/to/mesh-directory
```

Useful options:

- `--time-budget SECONDS` — reject a mesh whose candidate exceeds the budget;
- `--solver-timeout SECONDS` — hard subprocess timeout;
- `--surface-samples N` — samples per direction for the Hausdorff diagnostic;
- `--json PATH` — save per-scenario native metrics.

The root `evaluate.sh` is a compatibility entry point for the same C++
pipeline. The canonical command is `scripts/evaluate.sh`.

## 7. Interpreting results

A valid result means every scenario passed all local gates. It does not imply
that the official grader will produce the same score, because implementation
details, resource limits, and the official test set may differ.

When a run fails, inspect the per-scenario note and the saved report. Typical
next actions are:

- reduce an aggressive tier's compression target when SSIM fails;
- preserve feature and silhouette edges when normal SSIM falls;
- reduce collapse distance when Hausdorff fails;
- reduce scan work or simplify data structures when the time budget fails;
- fix link-condition, orientation, or retriangulation logic when topology fails.
