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

The script exits with status `0` for a valid submission and `1` otherwise.