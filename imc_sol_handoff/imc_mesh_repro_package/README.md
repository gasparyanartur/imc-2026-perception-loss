# IMC mesh simplification reproducibility package

This package contains the current best solver, the trusted baselines, all major
experimental branches from v28 through v33, the local six-view evaluator, mesh
generators, topology/Hausdorff diagnostics, recorded benchmark results, and the
complete engineering report.

## Current best

`solvers/current/nebula_atomic_region_v33_t7.cpp`

It keeps v33 behavior for all tiers except the huge-input branch (`nV >= 1,000,000`).
For that branch it uses a more robust Pineapple-style tail continuation, compacts
the result, and attempts one conservative atomic-region replacement with rollback.

## Environment used to verify the package

- Linux
- Python 3.13.5
- g++ 14.2.0
- NumPy 2.3.5
- SciPy 1.17.0
- trimesh 4.11.1

Other recent C++17 compilers and Python 3.11+ installations should work.

## Setup

```bash
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
make all
```

The competition solver itself uses only C++17 and the standard library. Python
packages are needed only for generating synthetic meshes and evaluating them.

## Fast reproduction

```bash
make fast
```

This compiles v23, v33, the current solver, and the proxy evaluator, then runs a
representative smooth/concave/genus-one/high-frequency benchmark at 128x128.
Outputs go to `results/reproduced_fast/`.

## Full diverse-form benchmark

```bash
make full
```

This runs all forms from `tools/imc_shape_benchmark.py` at 256x256 and estimates
sampled Hausdorff distance. It is slower and diagnostic only.

## Tier-isolation regression

```bash
make tier-small
```

This generates T2/T3/T4 closed meshes and checks that current v33-T7 does not
alter the non-target paths.

The huge diagnostic is opt-in because it writes multi-million-line meshes:

```bash
make tier-huge
```

The T7 preset contains 1,024,002 vertices and 2,048,000 faces. It is below the
2.1M-face bound and activates the solver's `nV >= 1,000,000` branch. A closed
triangulated genus-zero mesh with exactly 1.1M vertices would require about 2.2M
faces, so 1,024,002 is the practical face-limit-safe T7 stress mesh.

## Evaluate one result

```bash
build/imc_proxy_eval original.mesh candidate.mesh 512
python3 tools/imc_validate_mesh.py candidate.mesh --original original.mesh --pretty
```

The C++ evaluator reproduces the known six-view flat-normal/depth proxy. The
sampled Hausdorff result is not exact and must never be treated as the official
checker.

## Compare two solver outputs

```bash
python3 tools/imc_compare_outputs.py a.mesh b.mesh \
  --original original.mesh \
  --evaluator build/imc_proxy_eval \
  --resolution 256 \
  --pretty
```

## Rebuild checksums

```bash
make manifest
```

## Package structure

- `REPORT.md`: complete chronological and technical handoff.
- `solvers/current/`: current best submission source.
- `solvers/baselines/`: v23, original v33, and Pineapple v072.
- `solvers/experiments/`: v28-v32 and intermediate hybrid variants.
- `tools/`: evaluator, benchmark, mesh generator, validator, comparison tool.
- `scripts/`: reproducible build and benchmark commands.
- `results/`: recorded raw CSV/Markdown benchmark evidence.
- `design/`: recovery notes and next-algorithm design documents.

## Interpretation rules

1. Official judge outcomes reported in `REPORT.md` are user-reported outcomes,
   not locally reproduced judge logs.
2. Proxy SSIM is suitable for comparing branches, not proving official validity.
3. Sampled Hausdorff is a warning signal, not an exact geometric certificate.
4. A candidate is not considered promoted until untouched tiers are checked,
   topology is valid, and the official judge confirms the change.
