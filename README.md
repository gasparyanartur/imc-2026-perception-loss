# imc-2026-perception-loss

## Preliminary evaluator

`evaluate.py` is a self-contained Python reimplementation of the evaluation
pipeline described in `docs/report.md` (sections 2.1–2.7). It renders the
original and simplified meshes from the six axial cameras, builds flat-shaded
normal maps and perspective-correct depth maps, computes foreground-only 11×11
windowed SSIM, the symmetric (vertex-based) Hausdorff distance, the
closed-2-manifold validity gate, and the compression rate.

It is a *preliminary* reference for local iteration, not a bit-exact clone of
the official grader. Only NumPy is required (SciPy speeds up the Hausdorff step
when available).

```sh
python3 evaluate.py data/sample-input.txt data/sample-output.txt
# faster, lower-fidelity preview:
python3 evaluate.py data/sample-input.txt data/sample-output.txt --resolution 256
```

The script exits with status `0` for a valid submission and `1` otherwise. See
[`docs/evaluation.md`](docs/evaluation.md) for full details of the evaluator and
its options.

## Iterating on a solution

`evaluate.sh` runs the Python solver (`solution.py`) across a **representative
dataset** of meshes (`data/ppsurf/`, derived from the
[ppsurf dataset](https://huggingface.co/datasets/perler/ppsurf)), scores each
with `evaluate.py`, logs the aggregate to `outputs/<date>-<result>.txt`, and
reports whether the new model beats the best previous valid run. The submission
is only valid when **every** mesh in the dataset passes — evaluating on a single
mesh would hide solvers that fail on real geometry.

```sh
./evaluate.sh                 # default solver, dataset, resolution
RESOLUTION=1024 ./evaluate.sh # native-resolution score
```

Regenerate or grow the dataset with `datasets/prepare_ppsurf.py` (requires
`trimesh`); see [`docs/evaluation.md`](docs/evaluation.md) for details.

Coding agents should follow [`AGENTS.md`](AGENTS.md) and use the
[`skills/evaluate.md`](skills/evaluate.md) skill to drive this loop.

## Tests

The evaluator is covered by a pytest suite under `tests/` that validates mesh
I/O, the validity gate, rendering, SSIM, Hausdorff, the end-to-end pipeline, and
the multi-sample dataset harness.

```sh
pip install numpy pytest   # scipy is optional (speeds up Hausdorff)
python3 -m pytest
```