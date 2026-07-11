# Nebula v29: Flip-Unlock Postprocessor

## Status

- **Parent:** `nebula_ceiling_mix_v23.cpp`
- **v28 result:** Passed all tests but did not improve the competition score.
- **Purpose of v29:** Remove additional vertices after the v23 core by changing local connectivity, rather than only moving surviving vertices.
- **Affected inputs:** 5,001–50,000 original vertices, corresponding to the current T2–T4 mid-tier dispatch.
- **Protected paths:** Inputs above 50,000 vertices retain the v23 source path unchanged.

## Main idea

The v23 core can become trapped because its final one-ring topology makes every remaining edge collapse individually unattractive. A different triangulation of the same local surface may make the center vertex removable.

For a candidate center vertex, v29:

1. Extracts its oriented one-ring boundary.
2. Projects the ring to the dominant plane of the patch normal.
3. Rejects non-simple or highly curved rings.
4. Uses dynamic programming to enumerate the valid triangulations of the boundary polygon.
5. Chooses a triangulation using normal deviation, center-to-patch distance, triangle quality, and area-change terms.
6. Removes the center and replaces its incident fan atomically.
7. Applies only candidates whose one-rings do not overlap.
8. Validates the complete candidate as an oriented closed edge-manifold.
9. Renders the candidate against the exact v23 output from all six views.
10. Commits only when the candidate is nearly indistinguishable from v23; otherwise it keeps v23 unchanged.

This is “flip-equivalent” because all triangulations of a simple polygon are connected by diagonal edge flips. Instead of executing fragile flip sequences one by one, the postprocessor directly selects the best resulting triangulation and commits it atomically.

## Why this differs from earlier retriangulation branches

Earlier star and DP branches were embedded in older simplification trajectories and mostly used local geometric or local patch acceptance. v29 is intentionally narrower:

- It starts from the final, trusted v23 result.
- It searches connectivity configurations that v23's fan/root star deletion does not represent.
- It batches only disjoint patches.
- It validates the whole output topology after the batch.
- It uses a global six-view delta check against v23 before committing.
- It never needs to outperform v23 in proxy SSIM; it only needs to remain visually equivalent while using fewer vertices.

## Generated changes

The patcher modifies four parts of v23:

1. Adds `fuOriginalN` and `fuCoreStop` state.
2. Changes only the existing 5k–50k dispatch to call `postProcessFlipUnlock()`.
3. Reserves 1.85 seconds on the T3 core path by using `fuCoreStop` in the collapse-loop and T3 stage checks.
4. Inserts the postprocessor immediately before `initScale()`.

T2 and T4 keep the exact v23 transaction and run v29 only when spare time remains. T3 receives a deterministic postprocessing window. T5–T7 do not enter the new dispatch.

## Usage

```bash
python apply_v29_flip_unlock.py nebula_ceiling_mix_v23.cpp \
  -o nebula_flip_unlock_v29.cpp

g++ -std=c++17 -O3 -DNDEBUG -march=native \
  nebula_flip_unlock_v29.cpp -o nebula_flip_unlock_v29
```

The patcher intentionally fails rather than silently patching the wrong version when expected v23 markers are missing or duplicated.

## Conservative first-submission parameters

### Candidate geometry

| Tier | Maximum old normal deviation | Maximum new normal deviation | Center-distance fraction of internal Hausdorff radius | Maximum area change |
|---|---:|---:|---:|---:|
| T2 | 0.018 | 0.034 | 0.28 | 8.5% |
| T3 | 0.026 | 0.044 | 0.34 | 11% |
| T4 | 0.032 | 0.052 | 0.40 | 14% |

### Batch caps

At 384 rendering resolution:

- T2: 24 centers
- T3: 36 centers
- T4: 40 centers

At 256 resolution, caps are reduced to 10, 14, and 16 and the visual guard is stricter.

### Delta-render acceptance

The reference is the exact v23 output, not the original mesh. At 384 resolution the current guard is approximately:

- T2: final ≥ 0.9972, minimum view ≥ 0.9960, minimum normal ≥ 0.9940, minimum depth ≥ 0.9990.
- T3/T4: final ≥ 0.9965, minimum view ≥ 0.9952, minimum normal ≥ 0.9925, minimum depth ≥ 0.9987.

At 256 resolution the common guard is tightened to final ≥ 0.9990.

## Local validation performed

The delivered patcher was checked in three ways:

1. Python syntax compilation with `python -m py_compile`.
2. Source transformation and C++17 compilation against a small v23-shaped mock class.
3. A synthetic closed, oriented bipyramid test whose almost-coplanar valence-four apex was removed. The test changed the mesh from 6 vertices / 8 faces to 5 vertices / 6 faces and passed the postprocessor's oriented edge-manifold validator.

These checks validate source generation and the core connectivity operation. They do **not** replace an official judge submission.

## What to inspect after the first submission

There are three possible outcomes:

### Passes and score increases

The direction works. The next branch should increase only one of:

- candidate cap;
- valence limit from 10 to 12;
- T3 reserved time;
- second batch attempt.

Do not relax the render guard at the same time.

### Passes but score is unchanged

The postprocessor probably committed zero removals. The likely causes are:

1. no remaining one-rings satisfy the conservative geometry filter;
2. no candidates fit the remaining runtime;
3. the global delta guard rejects every batch.

The next diagnostic version should print, to `stderr` only:

- candidates generated;
- independent candidates selected;
- batch sizes tested;
- render resolution;
- accepted removals;
- elapsed time at entry and exit.

### Fails a test

First tighten or disable only the failing tier. The safest order is:

1. lower the batch cap;
2. tighten the center-distance fraction;
3. tighten normal deviation;
4. increase the delta-render thresholds;
5. disable that tier while retaining successful tiers.

Do not modify T5–T7 in response to a mid-tier failure.

## Likely next version after v29

If v29 establishes that connectivity-changing postprocessing can safely remove vertices, v30 should generalize from one interior vertex to small disks containing two or three interior vertices. It should retain the same architecture:

- atomic replacement;
- non-overlapping batches;
- full topology validation;
- v23 delta rendering;
- rollback.

If v29 produces no removals despite sufficient time, the better next move is not to loosen everything. Instead, integrate the postprocessor inside `runTransactionalScreenMid()` so it can reuse the already-rendered safe reference and avoid spending time rendering v23 again.
