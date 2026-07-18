# IMC2026 Local Evaluation Suite

Self-contained local tooling for the mesh-simplification problem.

## Contents

- `mesh_validate.py` — structural validity checks: indices, areas, duplicate faces, manifold incidence, orientation, unused vertices, Euler characteristic.
- `medium_mesh_stress_suite.py` — deterministic medium mesh generator and solver/baseline runner.
- `hard_mesh_suite.py` — harder connected T3-scale diagnostics: spikes, dimples, ridges, folded tubes, and knots.
- `official_like_renderer.cpp` — six axial cameras, flat face normals, perspective-correct depth, foreground-only 11×11 SSIM.
- `make_case_sheet.py` — renders one original/candidate pair and creates a 2×2×6 normal/depth contact sheet.
- `evaluate_solver.py` — runs a solver over a directory of `.in` cases, validates outputs, computes SSIM, and writes CSV/JSON results.
- `IMC2026_EVALUATION_PROTOCOL.md` and `experiment_log.csv` — experiment discipline and ledger template.
- `reference/` — evaluator/problem specification extracts.
- `legacy/evaluate_patch_visuals_v10.py` — historical wrapper retained for reference; it contains old absolute `/mnt/data` paths.

## Setup

```bash
python3 -m pip install -r requirements.txt
./build_renderer.sh
```

Requires Python 3.10+, `g++`, NumPy, and Pillow.

## Quick run

Generate hard cases and evaluate a solver executable or C++ source:

```bash
./run_example.sh /path/to/solution.cpp
# or
./run_example.sh /path/to/solution_binary
```

Results are placed under `example_cases/`, including:

```text
visual_results_candidate.csv
visual_results_candidate.json
renders_candidate/<case>/render_metrics.json
renders_candidate/<case>/<case>_2x2x6_contact_sheet.png
```

## Individual commands

```bash
python3 mesh_validate.py mesh.out --json

python3 medium_mesh_stress_suite.py \
  --out-dir medium_cases --target-vertices 24000 --generate

python3 hard_mesh_suite.py \
  --out-dir hard_cases --target-vertices 30000

python3 evaluate_solver.py \
  --solver ./solution --root hard_cases --variant test --resolution 1024

python3 make_case_sheet.py original.in simplified.out \
  --dest render_pair --resolution 1024
```

## Scope

The renderer follows the published evaluator constants and conventions: six axial cameras at distance 2.5, focal length 800 at 1024 resolution, flat normals, perspective-correct reciprocal-depth interpolation, background values, foreground-only SSIM, 11×11 windows, and equal normal/depth weighting.

It is a local approximation, not an official oracle. Synthetic cases diagnose mechanisms; only the competition evaluator establishes official correctness.
