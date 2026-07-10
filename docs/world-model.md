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
- 2026-07-10 workflow smoke-run evidence: increasing Banana T1/T2 retention
  beyond v16 preserves a large local perceptual margin but reduces compression
  (v17: 26.061328% versus v16: 30.447274%). Until official results disagree,
  the local model predicts that this direction is dominated.
- Native renderer diagnostics are resource-intensive: four concurrent
  synthetic evaluations exceeded their 120-second per-mesh timeout. Sequential
  local evaluation is required for reliable diagnostic evidence.
- The official suite contains a perceptually sensitive case absent from the
  local ppsurf suite: T1 Vega candidate v21 is locally valid but fails one of
  seven official cases. A stricter T1 Vega threshold makes the otherwise
  equivalent v22 valid at 88.364866%, so local T1 SSIM margins need a larger
  safety buffer.
- The local T1 retention frontier is sharp: reducing Banana's T1 keep ratio
  from 0.060 to 0.055 makes `abc_00010098` fail native SSIM, while stricter
  Vega post-pass controls at 0.060 do not alter the rendered result. This
  indicates that the base collapse phase, rather than the guarded Vega
  post-pass, determines this local failure boundary.