# Perception-Aware Mesh Simplification — IMC 2026

A C++ mesh-simplification system developed for the **Huawei IMC 2026 mesh simplification challenge**. The goal is to reduce a triangle mesh to as few vertices as possible while preserving its topology, geometry, and rendered appearance under the official evaluator.

The repository contains the final submission, the full project report, native evaluation tools, representative datasets, and the experimental history behind the solver.

- **Final submission:** [`FINAL_SUBMISSION.cpp`](FINAL_SUBMISSION.cpp)
- **Project report:** [`IMC report CVmaxxing.pdf`](IMC%20report%20CVmaxxing.pdf)
- **Challenge specification:** [`IMC.pdf`](IMC.pdf)

## Overview

The challenge can be formulated as a constrained mesh-optimization problem. Given an input triangle mesh \(M=(V,F)\), we seek a simplified mesh \(M'=(V',F')\) with as few vertices as possible:

\[
\min_{M'} |V'|
\]

subject to three main constraints:

\[
\mathrm{FinalSSIM}(M,M') \ge 0.9,
\qquad
 d_H(M,M') \le 0.05\,D_{\mathrm{AABB}},
\qquad
 M' \in \mathcal{M}_{\mathrm{closed}}.
\]

In other words, the simplified result must remain a **closed, watertight triangular 2-manifold**, stay within the allowed symmetric Hausdorff distance, and preserve appearance under the evaluator's six fixed camera views. Visual similarity is measured from rendered **normal and depth maps** using foreground-only SSIM.

This makes the problem different from ordinary mesh compression: the objective is not to minimize geometric error in isolation, but to remove geometry that is perceptually unnecessary while protecting the small subset of vertices and faces that strongly affect silhouettes, normals, depth, topology, or the Hausdorff bound.

Our solution is built around a fast **edge-collapse / Quadric Error Metric (QEM)** pipeline, augmented with competition-specific perceptual and structural safeguards. The final solver combines geometric collapse costs with topology checks, planar-region simplification, screen-space reasoning, six-view rasterization, and tightly budgeted refinement passes. The implementation is written as a single self-contained C++17 submission to fit the competition's runtime and memory constraints.

## Method

The solver evolved from a conventional QEM decimator into a perception-aware simplification pipeline. Its main components are:

- **Quadric-error edge collapse** for efficient geometry reduction.
- **Strict manifold and degeneracy checks** before topology-changing operations.
- **Planar / near-coplanar simplification** to aggressively remove redundant tessellation on flat surfaces.
- **Six-view screen-space analysis** matching the evaluator's axial camera setup.
- **Normal- and depth-aware perceptual checks** to protect visually important geometry.
- **Silhouette and projected-coverage reasoning** for vertices that have disproportionate image-space impact.
- **Strategic endpoint-weld and local refinement passes** near the perceptual frontier.
- **Tier-dependent behavior and anytime scheduling** so the algorithm remains practical from small meshes to million-vertex inputs.

The central design principle is to spend the vertex budget where the evaluator can actually see it. Flat interior triangulation can often be simplified heavily, while small changes near silhouettes, sharp features, or high-impact projected regions can consume a large fraction of the SSIM margin.

For the mathematical formulation and the full reasoning behind the approach, see the [project report](IMC%20report%20CVmaxxing.pdf), [`docs/math-formalism.md`](docs/math-formalism.md), and [`docs/mesh-simplification-overview.md`](docs/mesh-simplification-overview.md).

## Evaluation

The repository includes a local native evaluator designed to reproduce the important parts of the official acceptance pipeline. A candidate is compiled and run on the mesh suite, after which the output is checked for topology/geometry validity and compared perceptually through six rendered views.

The **official grader remains authoritative**; the local evaluator is intended for repeatable development, regression testing, and candidate ranking.

### Build the evaluators

```bash
scripts/build-evaluators.sh
```

### Evaluate the final submission

```bash
scripts/evaluate.sh --candidate FINAL_SUBMISSION.cpp
```

Evaluate with the synthetic benchmark suite and save a JSON report:

```bash
scripts/evaluate.sh --candidate FINAL_SUBMISSION.cpp \
    --include-synthetic \
    --json outputs/latest.json
```

To compile an arbitrary candidate directly:

```bash
scripts/build.sh path/to/solution.cpp
```

See [`docs/evaluation.md`](docs/evaluation.md) for the mesh format, metrics, validation gates, datasets, and evaluator options.

## Repository structure

```text
.
├── FINAL_SUBMISSION.cpp       # final self-contained C++ solution
├── IMC report CVmaxxing.pdf   # full project report
├── IMC.pdf                    # challenge specification
├── solutions/                 # solver families and iteration history
├── evaluators/                # native validity/perceptual evaluators
├── scripts/                   # build, evaluation and submission tools
├── data/                      # evaluation data and experiment records
├── datasets/                  # additional mesh datasets
├── docs/                      # formulation, methods and experiment notes
└── tests/                     # repository/evaluator checks
```

The most useful technical documentation is:

- [`docs/evaluation.md`](docs/evaluation.md) — local evaluation pipeline and metrics.
- [`docs/math-formalism.md`](docs/math-formalism.md) — mathematical formulation.
- [`docs/mesh-simplification-overview.md`](docs/mesh-simplification-overview.md) — background and algorithmic overview.
- [`docs/solutions.md`](docs/solutions.md) — detailed experimental history and solver iterations.
- [`docs/world-model.md`](docs/world-model.md) — accumulated observations about the hidden evaluation behavior.

## Final submission

[`FINAL_SUBMISSION.cpp`](FINAL_SUBMISSION.cpp) is the submission-ready solver. It is deliberately self-contained and uses no external runtime dependencies beyond a standard C++17 environment.

The development history in `solutions/` contains many intermediate solver families and experiments. These are retained for reproducibility and for understanding how the final approach evolved, but new readers should start with the report and `FINAL_SUBMISSION.cpp` rather than the historical variants.

## Reproducing development experiments

For experimentation, evaluate candidates locally before comparing them on the official service. Example:

```bash
scripts/evaluate.sh --candidate solutions/lemon/v115.cpp
```

The project contains extensive iteration records because many apparently safe simplifications behave differently at the hidden perceptual boundary. Local tests are therefore best treated as a regression and diagnostic tool rather than a perfect substitute for the official evaluator.
