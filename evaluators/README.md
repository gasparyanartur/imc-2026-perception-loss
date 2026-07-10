# Native C++ evaluator suite

A single binary (`evaluator.cpp`) reads both meshes once and reports every
metric the orchestrator needs:

- topology (manifold, orientation, repeats, euler/genus, boundary loops)
- geometric reduction stats and per-mesh surface area
- bidirectional Hausdorff distance (uniform-grid accelerated)
- six axial-view normal/depth SSIM (rendered in parallel)
- per-view silhouette IoU and foreground-pixel counts

Build with:

```sh
scripts/build-evaluators.sh
```

The binary prints structured `KEY=VALUE` lines on stdout and never reports
PASS/FAIL — the Python orchestrator decides based on its own thresholds.
