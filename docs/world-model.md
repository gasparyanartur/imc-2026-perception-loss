# World model

This is a working set of hypotheses about the official test environment, not
ground truth.

- The challenge appears to contain several fixed input-size tiers. The local
  suites are useful for modeling case structure, but cannot prove the official
  mapping.
- `data/ppsurf/` is useful for geometric diversity and failure discovery.
  `data/synth_bench/` is useful for targeted tier, shape, and renderer
  diagnostics; neither is a substitute for the official service.
- Native C++ SSIM is the local perceptual signal and should be interpreted as
  an approximation of the official renderer.
- Per-camera normal/depth metrics and sampled surface Hausdorff are more useful
  for diagnosing a regression than a single aggregate score.
- 2026-07-10 smoke-run evidence: the 10 local ppsurf meshes are all below
  10,000 vertices and cross the documented 5,000-vertex tier boundary; the
  lemon baseline's tiny-mesh target is too aggressive for several of them,
  producing SSIM failures despite acceptable Hausdorff values.