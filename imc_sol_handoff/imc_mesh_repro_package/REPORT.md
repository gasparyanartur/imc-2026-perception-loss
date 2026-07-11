# IMC Problem B Mesh Simplification — Complete Development and Reproduction Report

**Project:** Perception-aware simplification of closed triangular meshes  
**Competition context:** Huawei IMC Challenge, Problem B  
**Report updated:** 11 July 2026  
**Current best source:** `nebula_atomic_region_v33_t7.cpp`  
**Current best SHA-256:** `fbd49803b080033deb77aae8fd2df8b13daa8fb4b6e36c8a84dae7153a7b4333`  
**Trusted historical baseline:** `nebula_ceiling_mix_v23.cpp`  
**Current status:** the v33 atomic solver with a dedicated huge-input/T7 path is the best user-confirmed version so far. It has not been proven optimal and all local evaluator results remain proxies for the official judge.

---

## 1. Purpose of this document

This document is the complete engineering handoff for the mesh-simplification work performed so far. It is intended to let another developer or coding agent:

1. understand the official objective and the practical evaluator model;
2. reproduce the local build, topology checks, proxy rendering and synthetic benchmarks;
3. identify the exact current best solver and its dispatch logic;
4. understand every major experimental branch from v23 through v33-T7;
5. avoid repeating approaches that already failed on the judge;
6. continue with the planned vertex-budget redistribution and evaluator-aware search work.

The report distinguishes four evidence classes:

- **Official judge outcome:** feedback explicitly reported by the user after submission.
- **Recorded local result:** result reported during development, but the exact temporary mesh or raw log may not have survived.
- **Preserved local result:** raw CSV/Markdown result is included in the reproduction package.
- **Design hypothesis:** a proposed direction that has not yet been implemented or judged.

This distinction matters. The local proxy is useful for ranking changes, but it is not the official checker and cannot certify acceptance.

---

## 2. Current answer in one page

### 2.1 Current best solver

Use:

```text
nebula_atomic_region_v33_t7.cpp
```

The solver retains the original v33 behavior for normal tiers and introduces a dedicated branch when the original input contains at least one million vertices.

The huge-input branch:

1. uses the robust large-tail continuation settings derived from Pineapple v072;
2. targets a keep ratio of approximately `0.028` rather than the older `0.032` huge ratio;
3. scans more tail edges and accepts larger independent tail batches;
4. compacts the QEM output;
5. searches the compact result for one highly conservative atomic region replacement;
6. commits only if topology, local geometry and a six-view rollback guard pass;
7. otherwise returns the unmodified large-tier baseline.

The atomic search is deliberately performed **after compaction**, on roughly tens of thousands of vertices rather than on the original million-vertex connectivity.

### 2.2 Why this is the best current branch

The user reported that this version is the best one yet. Earlier branches either:

- failed multiple official tests;
- produced no score improvement;
- improved only synthetic spheres while regressing other forms;
- changed timing-sensitive large-tier behavior;
- or introduced a topology/perception risk disproportionate to the vertex reduction.

### 2.3 Main technical conclusion

The solver is no longer limited by a lack of local deletion operators. It already contains:

- QEM collapses;
- screen-aware quadrics;
- occlusion tests;
- star deletion;
- pair-disk replacement;
- valence welds;
- local render/SSIM guards;
- transactional continuation;
- atomic multi-vertex region replacement.

The next large improvement is more likely to come from either:

1. a more faithful optimization of the official aggregate score; or
2. **vertex-budget redistribution**, where vertices are added in high-render-error regions and more vertices are removed elsewhere in the same transaction.

Another ordinary collapse-order or postprocessing tweak is unlikely to move the score substantially.

---

## 3. Competition model and local evaluator assumptions

### 3.1 Objective

For each valid test, the compression score is driven by the output vertex count. Conceptually:

\[
\text{compression score} = 100 - 100\frac{|V'|}{|V|}.
\]

The simplified mesh must remain valid and pass the geometric and perceptual thresholds. Once the perceptual threshold is exceeded, additional visual quality does not directly improve the compression score. This makes the real task:

> Find the minimum vertex count that remains valid, geometrically close and just safely above the perceptual threshold.

### 3.2 Topological validity model used locally

The local tools enforce:

- all indices are valid;
- no repeated vertex within a triangle;
- every triangle has positive area;
- no duplicate undirected face;
- every undirected edge has exactly two incident faces;
- the two incident faces use opposite directions along the edge.

The last two conditions define a closed, consistently oriented edge manifold. Connectedness is reported but is not included in the local `closed_oriented_manifold` Boolean, because the exact official connectedness rule was not independently verified.

### 3.3 Geometric constraint

The working understanding is a symmetric surface Hausdorff limit of 5% of the original axis-aligned bounding-box diagonal. The source frequently uses:

```cpp
CParam_HausdorffDiagFraction = 0.055;
```

This is an internal collapse-radius envelope and must not be confused with an exact surface Hausdorff certificate. The provided Python validator performs a sampled bidirectional surface comparison only; it is diagnostic, not exact.

### 3.4 Six-view rendering proxy

The preserved C++ evaluator uses six axis-aligned cameras:

- +X, -X;
- +Y, -Y;
- +Z, -Z.

The known projection configuration is:

- camera distance: `2.5`;
- reference resolution: `1024 × 1024`;
- focal length: `800` at 1024 resolution;
- principal point: image center;
- flat face normals;
- perspective-correct reciprocal-depth interpolation;
- 11×11 SSIM windows;
- foreground-center inclusion rule;
- equal weighting of averaged normal-map SSIM and depth-map SSIM.

The local evaluator supports lower resolutions by scaling the focal length proportionally. Lower resolutions are for ranking candidates and detecting large regressions, not proving official validity.

### 3.5 Important evaluator implication

The final score is an aggregate of normal and depth quality over six views. Many experimental guards were much stricter than the known 0.9 final threshold, often requiring approximately `0.995–0.999` similarity to an already simplified baseline. These guards are useful for safe rollback but may leave considerable compression margin unused.

A future exact-evaluator branch should compare each candidate directly against the original mesh and optimize the aggregate objective rather than enforcing unnecessarily high per-view and per-channel minima.

---

## 4. Tier conventions and the one-million-vertex diagnostic

The project used both official size limits and internal solver boundaries. The core mid-tier dispatch in v23/v33 is approximately:

| Internal branch | Input vertices | Main path |
|---|---:|---|
| Tiny passthrough | `≤ 10` | Emit original mesh |
| Small | `11–5,000` | Base QEM and postpasses |
| T2-like | `5,001–25,000` | Transactional screen-aware path |
| T3-like | `25,001–45,000` | Direct screen-aware core |
| T4-like | `45,001–50,000` | Transactional screen-aware path |
| Large | `50,001–400,000` | Base/large QEM path |
| Huge | `> 400,000` historically | Tail batching and large continuation |
| Dedicated T7 in current best | original input `≥ 1,000,000` | Robust huge-tail path plus atomic transaction |

The reproduction package uses a T7 synthetic mesh with:

```text
1,024,002 vertices
2,048,000 faces
```

This mesh triggers the `nV >= 1,000,000` branch and remains below a 2.1-million-face ceiling.

A useful combinatorial observation is that a closed genus-zero triangular manifold satisfies approximately `F = 2V - 4`. Therefore, an exact 1.1-million-vertex closed triangular sphere would require about 2.2 million faces. A 1,024,002-vertex diagnostic is a more practical way to exercise the million-vertex branch while remaining under 2.1 million faces.

---

## 5. Trusted baseline v23

### 5.1 File identity

```text
File: nebula_ceiling_mix_v23.cpp
Size: 75,253 bytes
SHA-256: cd25a4746687c10f966527101c768306765fc5056a8c51abbda22145f0413a90
```

### 5.2 Core constants

Important v23 parameters include:

```cpp
CParam_HausdorffDiagFraction = 0.055;
HParam_TimeBudgetSeconds = 20.2;
HParam_KeepRatio_UpTo25k = 0.32;
HParam_KeepRatio_UpTo45k = 0.16;
HParam_KeepRatio_UpTo50k = 0.10;
HParam_KeepRatio_UpTo400k = 0.025;
HParam_KeepRatio_Huge = 0.032;
HParam_QemCostCapCoeff = 0.0375;
```

Large-tail settings:

```cpp
HParam_TailBatchElapsedStart = 11.8;
HParam_TailBatchStopElapsed = 19.4;
HParam_TailBatchScanEdges = 65536;
HParam_TailBatchTargetAccepts = 2048;
```

### 5.3 QEM representation

Each vertex stores a symmetric plane quadric. A triangle contributes a normalized plane quadric scaled by the square root of half its area. Edge candidates combine the endpoint quadrics and test:

- the 3×3 QEM minimizer when nonsingular;
- midpoint;
- each endpoint.

Both collapse directions are considered. Candidate validity requires:

- edge still exists;
- candidate versions match or are recomputed;
- exactly two common incident faces;
- exactly two common neighboring vertices;
- accumulated collapse-radius envelope remains below the internal Hausdorff radius;
- cost remains below the tier cost cap.

### 5.4 Connectivity and topology

The solver maintains:

- active/dead flags for vertices and faces;
- per-vertex incident face lists;
- sorted small neighbor sets;
- version counters for stale priority-queue candidates;
- collapse radii;
- accumulated or memoryless quadrics depending on tier.

A collapse rewrites incident faces, deletes degenerate faces, rewires neighbors and pushes refreshed candidates around the kept vertex.

### 5.5 Screen-aware mid-tier path

For inputs between roughly 5k and 50k vertices, v23 uses raster importance from the six cameras. It measures:

- visible face pixels;
- silhouette or depth/normal discontinuity pixels;
- a surrounding importance window.

The face quadrics are reweighted using these measurements. T2, T3 and T4 use different resolutions, weight functions and target sequences.

### 5.6 Transactional continuation

T2 and T4 use a safe checkpoint followed by more aggressive target-ratio trials. Each lower-ratio result is rendered against the checkpoint and retained only if it passes strict normal/depth/view guards. This is a strong safety mechanism but is all-or-nothing: a few harmful collapses can cause an entire lower-count continuation to be discarded.

### 5.7 Additional operators

The v23 family includes:

- occluded-edge collapse;
- star deletion and boundary fan retriangulation;
- valence-weld pass;
- pair-disk two-vertex deletion;
- Vega local patch SSIM screening;
- root nudge toward a removed vertex;
- large-tier camera transaction.

The number of existing operators is important: future work should not assume that “add another local delete pass” is automatically novel.

---

## 6. Current best v33-T7 architecture

### 6.1 File identity

```text
File: nebula_atomic_region_v33_t7.cpp
Size: 93,865 bytes
Lines: 3,014
SHA-256: fbd49803b080033deb77aae8fd2df8b13daa8fb4b6e36c8a84dae7153a7b4333
```

### 6.2 Dispatch logic

The important top-level logic is:

```cpp
readMesh();
hugeInputMode = (nV >= 1000000);

if (nV <= 10) passthrough;
if (5000 < nV && nV <= 50000) {
    // v33 T2 atomic path or original v33 mid-tier paths
}

// Standard QEM initialization and collapse loop.

if (hugeInputMode) {
    compact();
    runHugeAtomicRegionTxPlain();
    writeMesh();
    return;
}

// Remaining v33 postpasses for non-T7 tiers.
```

The intent is to keep the v33 behavior for all other tiers and isolate the huge-input experiment.

### 6.3 Huge-tier tail changes

When `hugeInputMode` is active, the code uses:

```cpp
keep ratio = 0.028;
tail scan cap = 131072;
tail acceptance cap = 4096;
tail stop = 19.8 seconds;
```

For non-huge inputs the original constants remain in effect.

### 6.4 Huge atomic transaction

After the huge QEM path is compacted, `runHugeAtomicRegionTxPlain()`:

1. exits if the result is too small or less than about 0.28 seconds remains;
2. snapshots the compact mesh;
3. builds vertex-face and face adjacency;
4. samples at most 640 seed vertices with a fixed stride;
5. constructs depth-one and depth-two macro-region candidates;
6. rejects candidates with excessive geometric deviation, normal damage, boundary size, interior size or insufficient gain;
7. keeps the lowest-score candidate, preferring larger gain on an exact tie;
8. applies exactly one candidate to a copy;
9. renders base and candidate at 96 resolution;
10. requires extremely high similarity:

```text
final score >= 0.99996
minimum view >= 0.99990
minimum normal >= 0.99982
minimum depth >= 0.99998
```

11. commits only when the candidate has fewer vertices.

This pass is intentionally tiny. Its role is to extract a handful of safe vertices without changing the million-vertex collapse phase.

### 6.5 Recorded local T7 results

The final development run reported:

| Shape | Huge baseline | Atomic T7 | Extra removal |
|---|---:|---:|---:|
| Smooth sphere | 30,800 | 30,794 | 6 |
| Ellipsoid | 30,800 | 30,795 | 5 |
| Concave peanut | 30,800 | 30,794 | 6 |

The outputs were reported as closed, consistently oriented manifolds with effectively unchanged low-resolution six-view proxy quality.

These exact temporary meshes and raw logs were not preserved. The reproduction package provides a deterministic generator for equivalent million-vertex sphere, ellipsoid and peanut diagnostics.

### 6.6 User-reported status

After receiving this branch, the user stated that it was the best one yet. This is the strongest current evidence for keeping it as the frozen baseline.

---

## 7. Chronological experiment history

### 7.1 Pre-v23 work

Before the files preserved in this package, the project explored many QEM simplifier variants with:

- tighter timing windows;
- different keep ratios;
- common-neighbor locking;
- jittered candidate positions;
- camera-aware vertex protection;
- propagation or movement of neighboring vertices;
- more aggressive star deletion;
- hyperparameter adjustments after hidden-test failures.

The main lesson was that timing and small candidate-order changes can alter large greedy trajectories. Blind parameter tightening sometimes passed one hidden case while losing another, and neighbor movement rarely changed the official score meaningfully.

### 7.2 v23 ceiling mix

v23 became the trusted baseline because it combined the best known mid-tier screen weighting, transactional continuation, local topology operations and large-tier handling. It also provided exact tiny-input passthrough:

```cpp
if (nV <= 10) { writeMesh(); return; }
```

This was essential during later recovery work because the official sample should remain byte-for-byte unchanged.

### 7.3 v28 render-fit safe

```text
File: nebula_renderfit_safe_v28.cpp
SHA-256: 7f5a415d65cb4c718ed7461b91dd9f279a48e21126ef18c7b455791508618f33
```

v28 introduced an isolated learned/render-fit mid-tier path. Its components included:

- rendering original and candidate meshes;
- attributing visible error to original faces;
- learned multiplicative face weights;
- rerunning a memoryless screen-weighted simplification;
- fitting candidate vertices to target normal/depth planes;
- projecting movement through local linear systems;
- strict orientation guards;
- fallback to the safe v23 result.

This branch was conceptually valuable because it attempted to optimize image residuals rather than only geometric QEM cost. In practice it did not become the final baseline. The fitting step could improve a proxy without reliably improving official compression, and the learned branch added significant complexity and runtime.

### 7.4 v29.0/v29.1 Flip-Unlock

The intended new primitive was a postprocessing one-ring removal with dynamic-programming retriangulation. It attempted to remove a low-curvature center vertex and triangulate its boundary polygon without requiring a sequence of ordinary edge collapses.

The first delivery mistake was operational rather than algorithmic: a Python patcher was provided instead of the generated C++ submission. Since the official sample has nine vertices and v23 immediately returns for `nV <= 10`, a correctly generated C++ solver could not algorithmically fail that sample. Recovery artifacts were then created:

- `apply_v29_1_flip_unlock.py`;
- `build_and_smoke_test_v29_1.py`;
- `V29_1_RECOVERY.md`;
- checksum files.

The corrected final source:

```text
File: nebula_flip_unlock_v29_1.cpp
SHA-256: c3e9792eab8596f547dc1522fbdf7209ee1d1d9a16e6ef046465e14e69ab916b
```

compiled and preserved the sample locally. It used spare time after the normal mid-tier path and attempted curvature-limited DP retriangulation with six-view comparison to the v23 output.

**Official user feedback:** it failed tests 2, 3 and 4.

**Diagnosis:** the pass allowed non-coplanar one-ring changes while comparing against the already simplified v23 result rather than the original mesh. Local visual similarity to v23 did not guarantee remaining original-mesh Hausdorff or official SSIM margin.

### 7.5 v29.2 Exact-Coplanar Unlock

```text
File: nebula_coplanar_unlock_v29_2.cpp
SHA-256: d39e9e148d4f96b3d1d1ac64d0b80719a9e339cad8ed827fb5175ac36032400a
```

v29.2 restricted postprocessing to essentially exact coplanarity:

- extremely small normal deviation;
- boundary and center near a common plane;
- simple polygon and orientation checks;
- dense local bidirectional surface sampling;
- near-identical six-view rendering;
- fallback to v23.

**Official user feedback:** no improvement.

**Lesson:** an operation can be safe yet too rare or too small to affect the score. Exact coplanar removal is useful as a correctness primitive, not necessarily as the main optimization direction.

### 7.6 v30 Normal Checkpoint

```text
File: nebula_normal_checkpoint_v30.cpp
SHA-256: 98e7e408ff6e229caf296d94a6e5a9fbc2188ddca40753281cbecd99d80d6fbb
```

v30 changed the search trajectory rather than modifying the finished mesh. It branched from a trusted T2 checkpoint and used a face-normal rotation penalty to rank the final collapses.

Recorded local smoke tests claimed a small reduction on a 22k smooth mesh while other tiers fell back to v23.

**Official user feedback:** it did not get better.

**Lesson:** a slightly different ordering near the endpoint still explores nearly the same collapse-only solution family. Synthetic smooth-form gains were insufficient evidence.

### 7.7 v31 Guided Micro-Batch QEM

```text
File: nebula_guided_batch_v31.cpp
SHA-256: e96e05ea8d7c613e7d955dbbfa25cfae27063ae2f502638f9d7427bf5a70c97e
```

v31 addressed the all-or-nothing continuation problem. Instead of trying a lower target and discarding the entire trajectory, it attempted collapses in small batches, rendered after each batch, rolled back failed batches, split failed batches into smaller groups and banned individual harmful edges.

This was algorithmically more interesting than v30 because it could keep harmless collapses from an otherwise failing continuation.

**Official user feedback:** it failed all tests, including the last.

**Lesson:** a complex selective continuation can still be unsafe if its local evaluator, state rollback, timing or tier isolation differs from the official environment. The branch was abandoned rather than incrementally patched.

### 7.8 v32 Six-View Remeshing

```text
File: nebula_sixview_remesh_v32.cpp
SHA-256: a1dc09de7fd016f0f2eaa94a39c49062987578e41a662a52012c843bd0671c7a
```

v32 was a genuine topology replacement:

1. render six depth atlases;
2. reconstruct a new watertight mesh with marching tetrahedra/visual-hull logic;
3. project the new connectivity toward the original surface;
4. simplify the new mesh;
5. tangentially redistribute vertices;
6. validate topology, sampled Hausdorff and six-view proxy;
7. fall back to v23.

Recorded local smooth-form tests claimed roughly 12% fewer vertices, but faceting and normal-map mismatch remained the core risk.

**Official user feedback:** not better.

**Lesson:** whole-mesh remeshing introduces too much normal-map uncertainty. The evaluator uses flat triangle normals, so a geometrically plausible new tessellation can score worse than a less elegant original-derived mesh.

### 7.9 v33 Atomic Region

```text
File: nebula_atomic_region_v33.cpp
SHA-256: 383150d1d1f56ed8be80d4b07918cba3fe08b4813c4f2a766a06df09d15c6ddf
```

v33 introduced atomic multi-vertex region replacement. It searches for a disk-like two-ring patch, fits a replacement surface vertex, removes several interior vertices and triangulates the boundary in one transaction. This can bypass bad intermediate collapses that ordinary QEM cannot traverse.

The operation includes:

- disk topology extraction;
- boundary ordering;
- fitted replacement point;
- normal and area checks;
- local surface-deviation checks;
- six-view ranking of complete transactions;
- multi-region and single-region fallback attempts;
- final transactional guard.

Recorded local smooth-form results showed reductions such as 18–21 vertices on ~10k sphere/ellipsoid meshes, but the preserved diverse-form benchmark showed that these gains often came with a small normal-map regression.

**Official user feedback:** test 7 failed and the overall result did not appear significantly better.

**Important diagnosis:** the v33 atomic region code did not run on the failed large test, but merely adding the code changed compilation layout/timing and perturbed the highly sensitive large QEM trajectory. On a synthetic 408k diagnostic, v23 and v33 produced the same count but different geometry and a tiny proxy difference.

### 7.10 Pineapple v072

```text
File: gasparyanartur-pineapple-v072-20260711004207-001.cpp
SHA-256: cf1f572a207a87bbb1098a36ab46042280e40a7080c2c2b35958dc51d3386726
```

The uploaded Pineapple source contained a different and apparently more robust large-tier configuration:

- huge keep ratio around `0.028`;
- tail scan cap `131072`;
- tail target accepts `4096`;
- tail stop around `19.8` seconds;
- no fragile v23 large-camera continuation in the same form.

This became the reference for fixing the T7 path without discarding the v33 atomic work.

### 7.11 Pineapple/v33 hybrid and 100KB minification

```text
File: pineapple_atomic_v33_passfix.cpp
SHA-256: 2c94407a3693d80337003718b6b1a667f48be8c4fece7beba07e799ab99d5637

Minified file: pineapple_atomic_v33_passfix_under100kb.cpp
SHA-256: 500e4c40a2db1c0b2e59a179c91d4a75d2466db8a1dcfc115a85f00bafa0b81f
```

The first hybrid kept the v33 atomic transaction for T2 and used Pineapple behavior for other tiers. It was later minified below 100KB by stripping comments, blank lines and indentation only. The minified source compiled and produced byte-identical local outputs to the unminified hybrid on representative tests.

The user then clarified that the desired final direction was not the broad Pineapple hybrid: keep v33 elsewhere and make the atomic idea work specifically on the million-vertex tier.

### 7.12 v33-T7 final branch

The final current best therefore combines:

- v33 as the conceptual base;
- Pineapple-derived huge-tail constants only when the original input has at least one million vertices;
- one conservative post-compaction atomic region transaction;
- original v33 behavior elsewhere.

**User feedback:** best one yet.

---

## 8. Official judge outcome ledger

No official score numbers or checker logs were preserved. The following table records only explicit user feedback.

| Version | User-reported official outcome | Decision |
|---|---|---|
| v29.1 Flip-Unlock | Failed tests 2, 3 and 4 | Reject |
| v29.2 Coplanar Unlock | Did not improve | Keep only as safety concept |
| v30 Normal Checkpoint | Did not get better | Reject |
| v31 Guided Batch | Failed all, including last | Reject completely |
| v32 Six-View Remesh | Not better | Reject |
| v33 Atomic Region | Failed test 7; not significantly better | Reject as full replacement, retain atomic primitive |
| Pineapple/v33 broad hybrid | Not selected as final direction | Superseded |
| v33-T7 atomic | “Best one yet” | Current frozen baseline |

Future agents must not convert these qualitative outcomes into invented numeric scores.

---

## 9. Preserved multi-shape benchmark

### 9.1 Why it was introduced

Early local tests relied too heavily on spheres and toruses. These forms are useful for smooth curvature but fail to expose:

- sharp normal discontinuities;
- thin opposing surfaces;
- planar region behavior;
- concavity and self-occlusion;
- frequency-sensitive normal maps;
- mechanical/CAD-like topology.

The preserved benchmark compares v23 and original v33 across diverse forms at 128 resolution. It is diagnostic and not official.

### 9.2 Preserved summary

| Form | Input V | v23 V | v33 V | ΔV | v23 final | v33 final | Δ final |
|---|---:|---:|---:|---:|---:|---:|---:|
| Smooth sphere | 10,242 | 2,918 | 2,900 | -18 | 0.988054 | 0.987996 | -0.000058 |
| Ellipsoid | 10,242 | 2,918 | 2,897 | -21 | 0.995797 | 0.995656 | -0.000142 |
| Peanut | 10,242 | 2,970 | 2,962 | -8 | 0.992099 | 0.992056 | -0.000042 |
| Dimpled sphere | 10,242 | 2,970 | 2,952 | -18 | 0.989857 | 0.989810 | -0.000047 |
| Torus | 8,192 | 2,457 | 2,457 | 0 | 0.986915 | 0.986915 | approximately 0 |
| Rounded cube | 10,242 | 2,918 | 2,900 | -18 | 0.996988 | 0.996973 | -0.000015 |
| High-frequency sphere | 10,242 | 3,072 | 3,072 | 0 | 0.953639 | 0.953491 | -0.000148 |
| Sharp cube | 6,146 | 1,813 | 1,813 | 0 | 0.877678 | 0.875538 | -0.002140 |
| Thin box | 6,146 | 1,843 | 1,641 | -202 | 0.805472 | 0.803544 | -0.001927 |
| Sharp cylinder | 24,578 | 7,373 | 7,250 | -123 | 0.870161 | 0.868924 | -0.001236 |
| Sharp cone | 12,290 | 3,502 | 3,502 | 0 | 0.855326 | 0.858825 | +0.003499 |
| Capsule | 8,066 | 2,379 | 2,373 | -6 | 0.996682 | 0.996675 | -0.000007 |
| 408k symmetric sphere diagnostic | 408,578 | 10,623 | 10,623 | 0 | 0.999249 | 0.999249 | approximately 0 |

### 9.3 Interpretation

The data established several important facts:

1. v33's sphere/ellipsoid gains were real but tiny.
2. The gains usually consumed a small amount of normal-map quality.
3. Large reductions on thin/mechanical forms could be misleading because quality was already below the known official target in the low-resolution proxy.
4. Atomic replacement was shape-dependent and not a universal win.
5. Smooth-form-only testing was overfitting the development process.
6. The large-tier failure could arise from timing/candidate-order drift even when the new operator was not executed.

### 9.4 Preserved files

The package contains:

- `results/imc_multishape_results.csv`;
- `results/imc_multishape_report.md`;
- `results/bench_sharp1/`;
- `results/bench_mech1/`;
- `results/bench_mech2/`.

---

## 10. Local evaluation tools

### 10.1 `imc_proxy_eval.cpp`

This standalone C++ program:

- parses the competition text mesh format;
- validates positive areas, duplicate faces and edge incidence/orientation;
- renders original and candidate from six views;
- computes flat-normal and reciprocal-depth images;
- computes foreground-aware 11×11 SSIM;
- reports JSON with:
  - candidate vertex and face count;
  - manifold Boolean and failure reason;
  - average final score;
  - minimum view score;
  - minimum normal score;
  - minimum depth score.

Compile:

```bash
g++ -std=c++17 -O3 -DNDEBUG -march=native tools/imc_proxy_eval.cpp -o build/imc_proxy_eval
```

Use:

```bash
build/imc_proxy_eval original.mesh candidate.mesh 256
```

### 10.2 `imc_shape_benchmark.py`

This Python benchmark generates:

- sphere;
- ellipsoid;
- peanut;
- dimpled sphere;
- rounded cube;
- organic and bumpy spheres;
- high-frequency wavy sphere;
- normal, thin, gear and wavy toruses;
- subdivided cube and thin box;
- cylinder and cone;
- capsule.

For every solver it records:

- runtime;
- status or timeout;
- candidate vertex count;
- compression ratio;
- six-view proxy fields;
- sampled Hausdorff fraction unless disabled.

Use the package scripts rather than calling it manually unless custom solver lists are needed.

### 10.3 `imc_validate_mesh.py`

This new handoff utility provides a more explicit topology report:

- finite coordinates;
- valid indices;
- repeated vertices in faces;
- minimum/maximum doubled face area;
- duplicate faces;
- bad edge-incidence count;
- bad edge-orientation count;
- connected component count;
- unused vertices;
- closed oriented manifold Boolean;
- optional sampled Hausdorff to an original mesh.

Example:

```bash
python3 tools/imc_validate_mesh.py candidate.mesh \
  --original original.mesh \
  --samples 18000 \
  --pretty
```

### 10.4 `imc_generate_tier_mesh.py`

This streaming generator creates face-limit-safe closed UV meshes without storing all faces in Python memory. Presets:

| Preset | Vertices | Faces |
|---|---:|---:|
| `t2` | 10,242 | 20,480 |
| `t3` | 40,002 | 80,000 |
| `t4` | 48,002 | 96,000 |
| `t6` | 400,002 | 800,000 |
| `t7` | 1,024,002 | 2,048,000 |

Shapes:

- sphere;
- ellipsoid;
- peanut;
- wavy;
- bumpy.

Example:

```bash
python3 tools/imc_generate_tier_mesh.py generated/t7_ellipsoid.mesh \
  --preset t7 --shape ellipsoid
```

### 10.5 `imc_compare_outputs.py`

Compares two output meshes by:

- byte identity;
- SHA-256;
- vertex/face deltas;
- detailed topology validation;
- optional proxy score against the original.

This is the recommended tool for untouched-tier isolation tests.

---

## 11. Exact reproduction procedure

### 11.1 Environment used for package verification

```text
Linux
Python 3.13.5
g++ 14.2.0
NumPy 2.3.5
SciPy 1.17.0
trimesh 4.11.1
```

The solver itself needs only a C++17 compiler and the standard library. Python dependencies are only for generation and evaluation.

### 11.2 Setup

From the package root:

```bash
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
make all
```

This builds:

```text
build/v33_t7
build/v23
build/v33
build/pineapple_v072
build/imc_proxy_eval
```

### 11.3 Fast diverse-form reproduction

```bash
make fast
```

This runs v23, original v33 and current v33-T7 over a representative subset at 128 resolution. Results are written to:

```text
results/reproduced_fast/
```

### 11.4 Full diverse-form reproduction

```bash
make full
```

This runs all preserved shape families at 256 resolution and estimates sampled Hausdorff distance. It is slower.

### 11.5 Tier isolation

```bash
make tier-small
```

This generates T2, T3 and T4 closed diagnostic meshes and compares original v33 against current v33-T7. The intended invariant is that non-huge paths remain unchanged in output behavior.

### 11.6 Huge/T7 diagnostics

```bash
make tier-huge
```

This additionally generates and runs one-million-vertex sphere, ellipsoid and peanut tests. The operation is opt-in because the generated files contain millions of text lines and consume significant disk and runtime.

Expected qualitative outcome:

- current v33-T7 follows the robust huge path;
- the atomic transaction may remove a small number of additional vertices;
- if no candidate passes or time is insufficient, the atomic pass must leave the huge baseline unchanged;
- topology must remain a closed oriented manifold.

### 11.7 Build flags

The development build used:

```bash
g++ -std=c++17 -O3 -DNDEBUG -march=native source.cpp -o solver
```

For portability testing, also compile without `-march=native`:

```bash
g++ -std=c++17 -O3 -DNDEBUG source.cpp -o solver_portable
```

Do not assume two different compiler versions will generate byte-identical floating-point outputs. Compare counts, topology and proxy quality as well as bytes.

### 11.8 Generate checksums

```bash
make manifest
```

The package includes a SHA-256 manifest so another agent can verify source identity before interpreting result differences.

---

## 12. Known local-evaluation limitations

### 12.1 Proxy versus official evaluator

The C++ evaluator is based on the known renderer and SSIM logic but has not been validated pixel-for-pixel against an official executable. Differences may exist in:

- clipping;
- exact raster boundary rules;
- floating-point precision;
- foreground-window inclusion;
- face orientation/backface behavior;
- depth normalization;
- exact validity ordering.

### 12.2 Sampled Hausdorff

The Python Hausdorff metric samples vertices, a subset of face centroids and random area-weighted points. It can miss a narrow worst-case deviation. Treat it as a rejection signal, never as proof of passing 5%.

### 12.3 Synthetic geometry bias

Synthetic spheres, toruses and UV meshes are unusually regular. They often create many equal-cost or near-equal-cost QEM candidates and can exaggerate timing/tie sensitivity. They are useful for regression testing but cannot represent the hidden test distribution.

### 12.4 Text I/O cost

The generated huge meshes are text files. Reading/writing them can consume a meaningful portion of observed wall time outside the solver's internally measured `startTime`, depending on where timing begins. Judge runtime and local shell timeout are not identical concepts.

### 12.5 Missing exact early smoke meshes

Several early recorded results used temporary 5,204-, 6,322-, 22,352- or 408k-vertex meshes that were not retained. Equivalent generators are included, but exact byte-level reproduction of every historical local claim is impossible.

---

## 13. Why previous directions failed

### 13.1 Comparing to the simplified baseline instead of the original

v29.1 accepted local changes that were nearly identical to v23 but could exceed the remaining original-mesh error budget. Every serious future transaction must ultimately be scored against the original mesh.

### 13.2 Too-conservative operations with no activation

v29.2 proved that near-exact coplanar surgery can be safe but too rare to change the score. A fallback-only branch can pass every local test and still be strategically useless.

### 13.3 Reordering the same collapse family

v30 changed late collapse ranking but remained in the same greedy collapse-only solution space. Small smooth-form gains did not transfer to the judge.

### 13.4 Complex selective rollback without faithful state/evaluator equivalence

v31 introduced a sophisticated batch search but failed broadly. The complexity increased the number of ways local state, timing and proxy decisions could diverge from the official environment.

### 13.5 Whole-mesh remeshing and flat normals

v32 generated new connectivity that looked geometrically plausible but created different flat-face normal maps. In this problem, tessellation orientation is part of the rendered signal.

### 13.6 Small shape-specific gains

v33 removed extra vertices on smooth and thin forms, but the preserved benchmark showed that the normal-map margin often decreased. A count reduction without sufficient perceptual margin is not a real improvement.

### 13.7 Timing and code-layout contamination

Large-tier QEM is sensitive to:

- elapsed-time gates;
- priority-queue ties;
- compiler layout;
- cache behavior;
- the number of stale candidates processed before a time boundary.

Even a branch that is never called can alter compilation and timing enough to change the result. This is why current T7 work uses a dedicated path and why untouched-tier regression is mandatory.

---

## 14. Non-negotiable development rules

1. **Freeze the current best source and hash before every experiment.**
2. **Never replace the fallback until the new branch passes the official judge.**
3. **Score final transactions against the original mesh.**
4. **Keep experimental changes tier-specific and regression-test adjacent tiers.**
5. **Do not infer success from one sphere or torus.**
6. **Always include sharp, thin, concave, high-frequency and genus-one forms.**
7. **Validate edge incidence and edge orientation after every connectivity operation.**
8. **Use sampled Hausdorff only as a warning metric.**
9. **Do not claim official validity from the proxy.**
10. **Do not loosen multiple independent guards in one submission.**
11. **Record source hash, compiler, flags, runtime, output count and proxy metrics.**
12. **Treat a no-op fallback as safe but not as an improvement.**
13. **Avoid broad large-tier code changes unless the expected gain is substantial.**
14. **Prefer one reversible transaction over several irreversible postpasses.**
15. **Preserve all raw result CSV/JSON files, not only summary prose.**

---

## 15. Planned next core algorithm: vertex-budget redistribution

### 15.1 Motivation

Every implemented branch so far primarily deletes or moves vertices. Once the greedy simplifier allocates too few triangles to a silhouette, visible ridge or high-normal-error area, later delete-only passes cannot repair that allocation.

The next major branch should allow a transaction to:

- add a small number of vertices where image error is high;
- remove a larger number of vertices from low-impact regions;
- produce a net reduction in vertex count;
- validate the complete result against the original six-view score.

### 15.2 Minimal transaction

The smallest prototype is:

```text
split one high-error edge: +1 vertex
remove two low-impact vertices: -2 vertices
net change: -1 vertex
```

The split can recover normal/depth margin that permits removals elsewhere.

### 15.3 Full transaction

1. Start from the frozen current result.
2. Render against the original at 256 or 384 resolution.
3. Attribute normal/depth residuals to candidate faces.
4. Rank high-error edges for splitting.
5. Project split vertices onto the original surface or a local fitted patch.
6. Rank low-impact QEM, star, pair-disk or atomic-region removals.
7. Test paired split/removal transactions.
8. Require a net vertex reduction.
9. Validate topology and sampled surface deviation.
10. Run the high-resolution proxy against the original.
11. Commit only if the target safety margin is retained.

### 15.4 Staged implementation

#### V34A: zero-count edge flips

Before splitting, implement evaluator-aware edge flips. They keep vertex count fixed but can improve the flat-normal map and alter future collapse opportunities.

#### V34B: split-one/remove-two

Test a small set of combinations:

- top 8–16 split candidates;
- top 16–32 removal candidates;
- at most a few hundred paired transactions.

#### V34C: split-K plus atomic removal

Use the v33 atomic operator as the removal side:

```text
split 4 high-error edges: +4
remove one 10-vertex atomic interior: -10
net: -6
```

This is the most promising form because it combines detail recovery with a multi-vertex local-minimum escape.

### 15.5 Promotion criterion

A redistribution branch should not be submitted merely because it activates. It should:

- reduce count across at least four different shape categories;
- preserve or improve proxy normal margin;
- leave non-target tiers stable;
- remain within time with rollback;
- pass the official judge on an isolated tier experiment.

The separate design document `design/V34_VERTEX_REDISTRIBUTION_DESIGN.md` is included in the package.

---

## 16. Other future tracks

### 16.1 Exact evaluator and residual attribution

This is the highest-leverage infrastructure task. Required outputs:

- per-view normal and depth score;
- foreground mismatch;
- per-pixel residual maps;
- candidate-face attribution;
- original-face attribution;
- incremental tile rerendering for local transactions.

### 16.2 Aggregate-objective tuning

Experiment with candidate guards based on the actual aggregate score rather than requiring every view and channel to remain near 1.0. Use safety targets such as 0.92 or 0.91 locally, then validate on the judge.

### 16.3 Evaluator-aware edge flips

Edge flips are low-risk because they preserve vertex count. Their value is recovering visual margin and creating new collapse opportunities.

### 16.4 Residual-aware QEM

At major checkpoints, update face weights using actual current residuals rather than only static projected area and silhouette counts.

### 16.5 Small endpoint beam search

Near the final 1–2% of removals, branch on a few top candidate operations and keep the branch with the best low-resolution original-mesh score. Do not rerun full multi-trajectory simplification.

### 16.6 Legal evaluator exploitation

Potential rule-aligned experiments include:

- six-view-only visibility classification;
- concentrating error in a small low-area region rather than distributing it;
- allowing one weak view when the six-view aggregate remains valid;
- trading normal score against depth score according to the actual average;
- probing whether connectedness, self-intersection or global orientation are explicitly checked.

Any checker probe must be isolated from the best solver and should never rely on crashes, malformed output or undefined behavior.

---

## 17. File inventory

### 17.1 Current and baselines

| File | Role | SHA-256 |
|---|---|---|
| `solvers/current/nebula_atomic_region_v33_t7.cpp` | Current best | `fbd49803b080033deb77aae8fd2df8b13daa8fb4b6e36c8a84dae7153a7b4333` |
| `solvers/baselines/nebula_ceiling_mix_v23.cpp` | Trusted historical baseline | `cd25a4746687c10f966527101c768306765fc5056a8c51abbda22145f0413a90` |
| `solvers/baselines/nebula_atomic_region_v33.cpp` | Original atomic region | `383150d1d1f56ed8be80d4b07918cba3fe08b4813c4f2a766a06df09d15c6ddf` |
| `solvers/baselines/gasparyanartur-pineapple-v072-20260711004207-001.cpp` | Robust large-tier reference | `cf1f572a207a87bbb1098a36ab46042280e40a7080c2c2b35958dc51d3386726` |

### 17.2 Major experimental sources

| File | Experiment | SHA-256 |
|---|---|---|
| `nebula_renderfit_safe_v28.cpp` | Learned render fitting | `7f5a415d65cb4c718ed7461b91dd9f279a48e21126ef18c7b455791508618f33` |
| `nebula_flip_unlock_v29_1.cpp` | DP one-ring Flip-Unlock | `c3e9792eab8596f547dc1522fbdf7209ee1d1d9a16e6ef046465e14e69ab916b` |
| `nebula_coplanar_unlock_v29_2.cpp` | Exact-coplanar retriangulation | `d39e9e148d4f96b3d1d1ac64d0b80719a9e339cad8ed827fb5175ac36032400a` |
| `nebula_normal_checkpoint_v30.cpp` | Normal-aware endpoint branch | `98e7e408ff6e229caf296d94a6e5a9fbc2188ddca40753281cbecd99d80d6fbb` |
| `nebula_guided_batch_v31.cpp` | Selective micro-batch continuation | `e96e05ea8d7c613e7d955dbbfa25cfae27063ae2f502638f9d7427bf5a70c97e` |
| `nebula_sixview_remesh_v32.cpp` | Six-view remeshing | `a1dc09de7fd016f0f2eaa94a39c49062987578e41a662a52012c843bd0671c7a` |
| `pineapple_atomic_v33_passfix.cpp` | Broad Pineapple/v33 hybrid | `2c94407a3693d80337003718b6b1a667f48be8c4fece7beba07e799ab99d5637` |
| `pineapple_atomic_v33_passfix_under100kb.cpp` | Minified hybrid | `500e4c40a2db1c0b2e59a179c91d4a75d2466db8a1dcfc115a85f00bafa0b81f` |

### 17.3 Tools

| File | Purpose |
|---|---|
| `tools/imc_proxy_eval.cpp` | Six-view normal/depth proxy and basic topology validation |
| `tools/imc_shape_benchmark.py` | Diverse synthetic benchmark and CSV/JSON/Markdown generation |
| `tools/imc_validate_mesh.py` | Detailed topology and sampled Hausdorff diagnostics |
| `tools/imc_generate_tier_mesh.py` | Streaming T2–T7 UV-mesh generator |
| `tools/imc_compare_outputs.py` | Byte/hash/count/topology/proxy comparison |

### 17.4 Scripts

| File | Purpose |
|---|---|
| `scripts/build_all.sh` | Compile current, baselines and evaluator |
| `scripts/run_fast_suite.sh` | Quick diverse-form regression |
| `scripts/run_full_suite.sh` | Full 256-resolution benchmark |
| `scripts/run_tier_regression.sh` | T2/T3/T4 isolation and optional T7 diagnostics |
| `scripts/make_manifest.sh` | Generate SHA-256 manifest |

---

## 18. Experiment logging template

Every new branch should create a record containing:

```markdown
# Experiment ID

## Hypothesis
What specific bottleneck is being addressed?

## Source identity
- Parent source and SHA-256
- New source and SHA-256
- Compiler/version
- Flags

## Scope
Which input tiers and which functions can change?

## Implementation
Exact algorithm, guards, budgets and fallback behavior.

## Local tests
For each mesh:
- input V/F
- output V/F
- runtime
- topology
- sampled Hausdorff
- final/min-view/min-normal/min-depth proxy
- fallback or activation

## Untouched-tier regression
Byte identity, count identity and proxy delta.

## Official judge
Exact user-reported pass/fail or score. Do not infer missing numbers.

## Decision
Promote, retain as primitive, retune or reject.

## Lesson
What should the next agent avoid or preserve?
```

---

## 19. Handoff checklist for another agent

Before editing code:

1. Verify `MANIFEST.sha256`.
2. Build with `make all`.
3. Run `make fast`.
4. Confirm current source hash.
5. Read the current top-level `run()` dispatch.
6. Identify exactly which tier the experiment can affect.
7. Create an experiment-specific copy; do not edit the frozen source.

Before claiming improvement:

1. Show activation on more than one geometry family.
2. Show output count reduction.
3. Show topology validity.
4. Show proxy normal/depth effects.
5. Run adjacent-tier isolation.
6. Record runtime and timeout margin.
7. Submit one controlled change to the official judge.
8. Preserve the raw output and user feedback.

---

## 20. Final current-state declaration

### Stable/current

- `nebula_atomic_region_v33_t7.cpp`
- v33-style mid-tier atomic behavior
- robust million-vertex tail settings
- one conservative post-compaction huge atomic transaction
- exact tiny-input passthrough
- fallback-first development philosophy

### Rejected as full replacements

- v29.1 Flip-Unlock
- v29.2 exact coplanar unlock as a score-improvement strategy
- v30 normal-aware checkpoint ordering
- v31 guided micro-batch continuation
- v32 six-view whole-mesh remeshing
- original v33 across all tiers
- broad Pineapple/v33 hybrid as the final architecture

### Retained primitives

- original-mesh transactional evaluation
- exact-coplanar retriangulation
- atomic multi-vertex region replacement
- robust Pineapple huge-tail settings
- six-view residual attribution
- evaluator-aware local topology changes

### Next recommended implementation

1. improve evaluator fidelity and per-face residual attribution;
2. implement zero-count evaluator-aware edge flips;
3. implement split-one/remove-two vertex-budget redistribution;
4. extend to split-K plus atomic-region removal;
5. evaluate across the full diverse-form suite before any judge submission.

The most important strategic rule is:

> Do not optimize a proxy branch merely until it activates. Optimize the complete candidate against the original mesh, prove that untouched tiers remain stable, and demand cross-form evidence before using an official submission.

---

## 21. Reproduction-package verification performed during this handoff

The packaged sources and tools were verified on the environment listed above.

### 21.1 Compilation

The following targets compiled successfully with `g++ 14.2.0`, C++17, `-O3`, `-DNDEBUG` and `-march=native`:

- current `v33_t7`;
- v23;
- original v33;
- Pineapple v072;
- the local proxy evaluator.

### 21.2 Generator and topology-validator smoke test

A 290-vertex closed peanut mesh generated by `imc_generate_tier_mesh.py` was simplified by the current solver to 125 vertices and 246 faces. The detailed validator reported:

- closed oriented manifold: true;
- one connected component;
- zero bad edge-incidence edges;
- zero bad edge-orientation edges;
- zero duplicate faces;
- sampled Hausdorff fraction approximately `0.0309` of the original diagonal.

The low-resolution 128 proxy was only `0.8197` on this very coarse small-mesh stress test; this confirms that topology/Hausdorff success alone does not imply perceptual success and is exactly why the proxy must be retained in the workflow.

### 21.3 Preserved benchmark smoke test

At 64 resolution on the preserved 10,242-vertex smooth sphere:

| Solver | Output V | Output F | Final proxy | Min normal | Min depth | Manifold |
|---|---:|---:|---:|---:|---:|---|
| v23 | 2,918 | 5,832 | 0.996359880 | 0.992673036 | 0.999999996 | true |
| current v33-T7 | 2,900 | 5,796 | 0.996346318 | 0.992642497 | 0.999999996 | true |

This reproduces the known T2 atomic behavior: 18 fewer vertices with a very small normal-proxy decrease. Raw JSON/CSV results are included under `results/package_smoke/`.
