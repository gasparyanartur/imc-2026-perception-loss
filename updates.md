# Updates

## 2026-06-27

- Starting point: user reports `simplifygeometry.cpp` is currently the best version.
- Reported v2 official result: `VVFFFFF`; tests 6 and 7 specifically report invalid meshes.
- Local worktree before edits: branch `fredrik-dev` is ahead of origin by 1 commit, `AGENTS.md` is deleted, and `simplifygeometry.cpp` is untracked. I will not revert unrelated changes.
- Docs constraints reviewed: valid output must have `1 <= V' <= V`, all face indices in range, non-degenerate triangular faces, every undirected edge shared by exactly two faces, and consistent directed-edge orientation. It must also pass vertex-based Hausdorff `<= 0.05 * AABB diagonal` and `FinalSSIM >= 0.9`.
- Initial code comparison:
  - `simplifygeometry.cpp` uses free-position QEM edge collapses with target tiers and a cost cap.
  - `simplifygeometry_v2_aggressive.cpp` uses endpoint-only collapses, link-condition checks, normal-change checks, and a cluster-radius Hausdorff proxy.
  - The reported invalid-mesh failures point first at v2 collapse/cleanup topology rather than compression scoring.
- Local environment notes:
  - `python3 -m pytest` does not run because `pytest` is not installed.
  - `./evaluate_cpp.sh simplifygeometry.cpp` fails without an Eigen include path (`Eigen/Dense` missing from repo), but local Eigen exists at `/usr/include/eigen3/Eigen/Dense`; manual compile with `-I /usr/include/eigen3` works.
- Local v2 baseline before changes:
  - `./evaluate_cpp.sh simplifygeometry_v2_aggressive.cpp` passed all 10 local ppsurf meshes at resolution 1024.
  - Mean compression was `76.684753%`, min compression `67.714286%`.
  - This local set only covers 1.4k-9.6k vertices, so it does not exercise official tests 3-7.
- Main suspected v2 problem compared with old:
  - Old solver has calibrated `target_vertices` tiers and `cost_cap` by input size.
  - v2 had no target vertex stop and no cost cap, so it kept collapsing until no valid edge remained.
  - This matches official `VVFFFFF`: small tests can pass, mid-size tests can over-compress below SSIM/Hausdorff tolerance, and large tests can run too long / consume too much memory / produce partial or malformed output.
- Change made to v2:
  - Added the old calibrated target tiers and cost caps to `simplifygeometry_v2_aggressive.cpp`.
  - The collapse loop now stops once `alive_count <= target_vertices` and breaks when the cheapest current endpoint-collapse cost exceeds the size-tier cap.
- New local tests:
  - Added `tests/solver_validity_smoke.py`, a standalone no-pytest smoke test.
  - It compiles a C++ solver, generates exact-size closed torus meshes at official-like tiers, runs the solver, checks `evaluate.check_validity`, and checks the calibrated minimum kept-vertex floor.
  - Default cases: 5k and 25k vertices. `--large` adds 40k and 50k vertices.
- Local results after v2 target/cost change:
  - `python3 tests/solver_validity_smoke.py simplifygeometry_v2_aggressive.cpp` passed 5k and 25k exact-tier tori.
  - `python3 tests/solver_validity_smoke.py simplifygeometry_v2_aggressive.cpp --large` passed 5k, 25k, 40k, and 50k exact-tier tori.
  - `./evaluate_cpp.sh simplifygeometry_v2_aggressive.cpp` passed all 10 local ppsurf meshes at resolution 1024.
  - Mean compression after the guard is `54.013624%`, min compression `30.002076%`.
  - Compression dropped versus unguarded v2, as expected; this is the cost of restoring the old official-safe stopping behavior.
- Local old/current-best comparison:
  - Compiled `simplifygeometry.cpp` manually with `g++ -O2 -std=c++17 -I /usr/include/eigen3`.
  - `python3 evaluate_dataset.py --python /tmp/simplifygeometry_old --solver ignored --dataset data/ppsurf --summary` passed 9/10 local ppsurf meshes.
  - Failure was `abc_00010045`: Hausdorff `0.1640 > 0.1493`.
  - Mean compression was `38.769089%`, but overall local result was invalid due the Hausdorff failure.
- Tooling improvement:
  - Updated `evaluate_cpp.sh` to accept `CXXFLAGS`, so Eigen-based solvers can be run locally as:
    `env "CXXFLAGS=-I /usr/include/eigen3" DATASET_DIR=data/sample-input.txt ./evaluate_cpp.sh simplifygeometry.cpp`.
  - Verified that command on `data/sample-input.txt`: 1/1 valid.
- User retest after the above still reports official `VVVFFFF`; v1 reportedly passes all official tests, so local validation is still missing at least one hidden failure mode.
- Local validation gap found:
  - The original smoke test only checked generated-mesh topology through 50k vertices; it did not full-score generated meshes and did not exercise official-scale 400k/1M runtime.
  - Extended `tests/solver_validity_smoke.py` with solver timing, optional full scoring (`--score`), Eigen compile flags (`--cxxflags` / `CXXFLAGS`), and extreme fixtures (`--extreme`) at 400k and 1M vertices.
- New generated full-score validation for v2:
  - `python3 tests/solver_validity_smoke.py simplifygeometry_v2_aggressive.cpp --large --score`
  - Passed 5k, 25k, 40k, and 50k generated tori with full local `evaluate.evaluate()` at 1024.
  - Lowest generated SSIM was `0.9268` on `torus_5k`, so generated scoring does not reproduce the hidden `F`s through test 5.
- New extreme runtime validation:
  - `python3 tests/solver_validity_smoke.py simplifygeometry_v2_aggressive.cpp --extreme --timeout 180`
  - v2 passed topology at 400k and 1M, but solver runtime was `10.31s` for 400k and `37.73s` for 1M.
  - `python3 tests/solver_validity_smoke.py simplifygeometry.cpp --cxxflags '-I /usr/include/eigen3' --extreme --timeout 180`
  - v1 passed the same fixtures with `3.15s` for 400k and `9.79s` for 1M.
  - Current best explanation for hidden v2 failures: endpoint-only v2 is valid but too slow/memory-heavy at large scale, likely exceeding the 21s official limit and producing failed/invalid large cases. v1's faster adjacency strategy explains why it can pass official while v2 fails.
- New high-detail scoring stress:
  - Added optional `--bumpy` fixtures to `tests/solver_validity_smoke.py`; these are higher-frequency bumpy tori through 50k vertices.
  - `python3 tests/solver_validity_smoke.py simplifygeometry_v2_aggressive.cpp --bumpy` passes topology/runtime.
  - `python3 tests/solver_validity_smoke.py simplifygeometry_v2_aggressive.cpp --bumpy --score` fails SSIM on `bumpy_5k` (`0.7263`), `bumpy_40k` (`0.8615`), and `bumpy_50k` (`0.8467`) despite valid topology and Hausdorff.
  - v1 on the same bumpy scoring stress also fails those three cases, but scores better on `bumpy_40k` (`0.8921` vs v2 `0.8615`), so the fixture is a detail-loss stress test rather than a perfect hidden-set proxy.
  - Conclusion: hidden `F`s may be a mix of scoring failures on detailed geometry (tests 4-5) and runtime failure on large geometry (tests 6-7). Local validation now has knobs to exercise both.
- User updated `simplifygeometry.cpp` again; latest v1 is treated as the current official-best reference and left uncommitted/untouched.
- v2 fast/accurate update:
  - Replaced the slow endpoint-only v2 core with the current fast v1-style free-position QEM core.
  - Added an O(1) scalar cluster-radius guard for free-position collapses:
    `r_new = max(r_a + |p_a - p_new|, r_b + |p_b - p_new|)`.
  - Reject collapse if `r_new > 0.05 * diagonal`. This fixes the local ppsurf Hausdorff failure class without returning to the slow endpoint-only cluster-list approach.
  - Raised the small-tier target from 15% keep to 30% keep in v2 after generated smooth `torus_5k` failed SSIM at the 15% tier (`0.8851`). At 30% keep, `torus_5k` passes with SSIM `0.9242`.
  - Updated `tests/solver_validity_smoke.py` target floors to match the new v2 tiers.
- Latest local v2 results:
  - `python3 tests/solver_validity_smoke.py simplifygeometry_v2_aggressive.cpp --cxxflags '-I /usr/include/eigen3' --large --score`
    passes 5k/25k/40k/50k generated smooth tori. Worst SSIM: `0.9242` on `torus_5k`; `torus_50k` SSIM is `0.9260`.
  - `python3 evaluate_dataset.py --python /tmp/simplifygeometry_v2_fast --solver ignored --dataset data/ppsurf --summary`
    passes 10/10 local ppsurf meshes. Mean compression `42.169256%`.
  - `python3 tests/solver_validity_smoke.py simplifygeometry_v2_aggressive.cpp --cxxflags '-I /usr/include/eigen3' --extreme --timeout 180`
    passes generated 400k/1M topology/runtime. Runtime: `3.94s` for 400k, `13.03s` for 1M.
  - Compared with latest v1 local ppsurf: v1 is faster/aggressive but still fails local `abc_00010045` by Hausdorff (`0.1640 > 0.1493`), while v2 passes that local case at the cost of lower compression.
- Theory / graph-theory ideas to try next:
  - **Planar patch dual-graph contraction:** build a face-dual graph, merge near-coplanar connected components, then contract/retriangulate only component interiors. The planar invariance theorem says this can preserve depth/normal maps almost exactly when boundaries stay fixed.
  - **Six-view silhouette protection as a hitting-set problem:** mark primal edges that are silhouettes in any official camera; treat them as a protected edge set. Collapse candidates get a large penalty if they disconnect or move this protected graph.
  - **Conflict-graph maximal independent set batching:** build a graph where two candidate collapses conflict if their one-rings touch. A maximal independent set of low-cost collapses can be applied in batches, reducing heap churn for large cases while preserving local validity checks.
  - **Curvature-aware epsilon-net:** select survivors as an adaptive surface net with smaller radius near high normal variation / silhouette edges and larger radius on flat regions. Then constrain QEM collapses so every represented cluster stays inside its local epsilon.
  - **Spectral/AMG-style heavy-edge matching:** use mesh graph coarsening ideas from algebraic multigrid to choose well-spaced edge contractions quickly, then run QEM refinement only near high projected-error regions.
  - Most promising near-term novel direction: combine planar dual components plus conflict-graph batching. That directly attacks both known problems: compression on evaluator-invariant regions and runtime at 400k/1M scale.

## Implementation Plan For Next Solver Iteration

Goal: improve beyond the current v2 by keeping v1-style speed, preserving the scalar cluster-radius Hausdorff guard, and adding evaluator-aware simplification that removes vertices where the six rendered normal/depth maps should change least.

### Phase 1: Planar Dual-Graph Contraction

Purpose: get nearly free compression on CAD-like coplanar patches before generic QEM.

Implementation steps:

1. Compute per-face data:
   - unit normal `n_f`;
   - plane offset `d_f = -dot(n_f, p0)`;
   - area;
   - two/three incident edge ids if available.

2. Build an edge-to-face table while loading faces:
   - key undirected edge `(min(a,b), max(a,b))`;
   - value: up to two incident face ids.

3. Build face-dual components with DSU:
   - for every edge with incident faces `f,g`, union if:
     `dot(n_f, n_g) > 0.9998` and `abs(d_f - d_g) < 1e-4 * diagonal`.

4. Mark planar component boundaries:
   - edge is a boundary if incident faces are in different components;
   - vertices touching component-boundary edges become protected for the planar pass.

5. Create a planar-first collapse queue:
   - candidate edge if its two incident faces are in the same planar component;
   - prefer edges whose endpoints are not protected boundary vertices;
   - replacement:
     - endpoint if either endpoint is boundary-protected;
     - midpoint/QEM point projected onto component plane if fully interior.

6. Apply collapses with current validity guards:
   - link condition/common-neighbor count;
   - exactly two shared faces;
   - no degenerate faces after collapse;
   - scalar cluster-radius guard;
   - do not collapse two boundary vertices unless the collapse keeps the same component boundary path.

7. Stop planar pass when no safe planar candidates remain, then run the current generic QEM pass.

Expected result:
larger compression on planar/CAD meshes with little or no SSIM loss, because flat normal and planar perspective-correct depth remain almost invariant when the plane and boundary are preserved.

### Phase 2: Six-View Silhouette Protection

Purpose: avoid damaging depth/foreground boundaries that dominate SSIM.

Implementation steps:

1. For every primal edge with adjacent faces `f,g`, test six official view directions:
   - `+x, -x, +y, -y, +z, -z`.
   - edge is silhouette-risk for a view if `dot(n_f, view) * dot(n_g, view) < 0`.

2. Store:
   - `edge_silhouette_count`;
   - `vertex_silhouette_weight += edge_silhouette_count`.

3. Modify generic QEM cost:
   - `cost *= 1.0 + lambda_sil * max(weight[a], weight[b])`;
   - start with `lambda_sil = 0.25`.

4. For small/mid tiers, add hard protection for strongest silhouette edges:
   - if `edge_silhouette_count >= 3`, only allow collapse if both adjacent faces are nearly coplanar.

Expected result:
fewer hidden scoring failures from contour/depth damage, with limited compression loss.

### Phase 3: Curvature-Aware Local Radius

Purpose: make the Hausdorff budget adaptive instead of globally uniform.

Implementation steps:

1. Compute vertex curvature proxy:
   - for each vertex, gather incident face normals;
   - curvature = max angle or normal variance.

2. Normalize curvature into `[0,1]`, using approximate percentiles or capped max:
   - `curv_score = min(1, curvature / 1.2)`.

3. Define local allowed cluster radius:
   - flat/coplanar: `local_limit = 1.00 * hausdorff_limit`;
   - high curvature/silhouette: `local_limit = 0.35..0.60 * hausdorff_limit`.

4. Collapse guard becomes:
   - `r_new <= min(local_limit[v1], local_limit[v2])`.

5. Keep planar-pass override:
   - if edge is fully interior to a planar component, allow the larger flat-region limit.

Expected result:
less high-frequency detail loss while still compressing flat regions hard.

### Phase 4: Conflict-Graph Batching For Large Runtime

Purpose: reduce priority-queue churn and make 400k/1M cases safer under the 21s limit.

Implementation steps:

1. During a batch, pop low-cost candidates from the heap into a temporary list.

2. Build an implicit conflict graph:
   - two collapses conflict if their endpoints or one-ring neighbors overlap.

3. Greedy maximal independent set:
   - sort by cost;
   - accept candidate if none of its endpoints/one-ring vertices are locked;
   - lock the one-ring of accepted candidate.

4. Apply accepted collapses sequentially with full existing validity checks.

5. Rebuild/update local candidates after each batch.

6. Use batching only for large tiers:
   - enable for `nV > 100000`;
   - batch size target: `2048..8192` candidates.

Expected result:
large-case runtime closer to graph-coarsening algorithms, without sacrificing topology checks.

### Phase 5: Adaptive Tier Selection

Purpose: avoid one hardcoded keep ratio per official size tier.

Implementation steps:

1. Before simplification compute mesh features:
   - `planar_edge_ratio`;
   - `silhouette_edge_ratio`;
   - `mean_curvature`;
   - `curvature_p90`;
   - average valence.

2. Start from current v1/v2 tier target.

3. Adjust:
   - high planar ratio: lower keep target, e.g. `target *= 0.75`;
   - high curvature p90: raise keep target, e.g. `target *= 1.20`;
   - high silhouette ratio: raise keep target, e.g. `target *= 1.10`;
   - low curvature + low silhouette: lower keep target modestly.

4. Clamp per tier to judge-proven ranges:
   - small 5k tier should not drop below 30% until better detail protection exists;
   - large 400k/1M can stay aggressive if runtime remains under budget.

Expected result:
fewer shape-specific hidden failures while retaining aggression on easy models.

### Validation Order

After each phase, run:

1. Compile:
   - `g++ -O2 -std=c++17 -I /usr/include/eigen3 simplifygeometry_v2_aggressive.cpp -o /tmp/simplifygeometry_v2`

2. Local ppsurf:
   - `python3 evaluate_dataset.py --python /tmp/simplifygeometry_v2 --solver ignored --dataset data/ppsurf --summary`

3. Generated smooth score:
   - `python3 tests/solver_validity_smoke.py simplifygeometry_v2_aggressive.cpp --cxxflags '-I /usr/include/eigen3' --large --score`

4. Generated bumpy score:
   - `python3 tests/solver_validity_smoke.py simplifygeometry_v2_aggressive.cpp --cxxflags '-I /usr/include/eigen3' --bumpy --score`

5. Extreme runtime/topology:
   - `python3 tests/solver_validity_smoke.py simplifygeometry_v2_aggressive.cpp --cxxflags '-I /usr/include/eigen3' --extreme --timeout 180`

### Preferred Coding Order

1. Planar dual-graph contraction.
2. Six-view silhouette cost/protection.
3. Curvature-aware local radius.
4. Adaptive tier selection.
5. Conflict-graph batching for large tiers.

Rationale:
planar contraction and silhouette protection are most directly tied to the official renderer. Batching is important, but current v2 is already back under the generated 1M runtime budget, so runtime is no longer the first blocker.

## Implementation Progress: Planar Components And Silhouette Cost

- Implemented the first two planned ideas in `simplifygeometry_v2_aggressive.cpp`:
  - planar face-dual components using DSU;
  - planar-internal edge cost discount;
  - planar-heavy adaptive target reduction;
  - six-view silhouette edge cost penalty.
- Important correction:
  - A first planar threshold (`dot > 0.9998`, offset `1e-4 * diagonal`) overclassified smooth tessellated torus quads as planar.
  - Tightened to near-exact coplanarity (`dot > 0.9999995`, offset `1e-7 * diagonal`) and required planar components to have at least 8 faces before they count as planar patches.
  - This keeps isolated two-triangle quads on curved surfaces from triggering planar-heavy target reduction.
- Current implementation details:
  - build `face_dsu` by unioning near-exact coplanar adjacent faces;
  - compute `planar_comp_size`;
  - mark `planar_edges` only when both faces are in the same component and that component has at least 8 faces;
  - mark component-boundary vertices;
  - lower target only when `planar_edge_ratio` is high:
    - `> 0.70`: target `*= 0.65`;
    - `> 0.45`: target `*= 0.75`;
    - `> 0.25`: target `*= 0.90`;
  - planar edge cost multiplier:
    - interior planar edge: `0.04`;
    - boundary-touching planar edge: `0.35`;
  - silhouette edge cost:
    - for each axis, if adjacent face normals have opposite signs in that component, add 2 to silhouette count;
    - cost multiplier: `1 + 0.20 * silhouette_count`;
    - non-planar strong silhouette edges (`count >= 4`) get an additional `* 2.0`.
- Validation after component-size filtering and silhouette weighting:
  - `python3 tests/solver_validity_smoke.py simplifygeometry_v2_aggressive.cpp --cxxflags '-I /usr/include/eigen3' --large --score`
    passes generated smooth 5k/25k/40k/50k.
    - `torus_5k`: SSIM `0.9239`, compression `70.000%`;
    - `torus_50k`: SSIM `0.9236`, compression `73.000%`.
  - `python3 evaluate_dataset.py --python /tmp/simplifygeometry_v2_planar_sil --solver ignored --dataset data/ppsurf --summary`
    passes 10/10 local ppsurf meshes.
    - mean compression `42.139430%`;
    - lowest local ppsurf SSIM `0.9484`.
  - `python3 tests/solver_validity_smoke.py simplifygeometry_v2_aggressive.cpp --cxxflags '-I /usr/include/eigen3' --bumpy --score`
    still fails the deliberately harsh bumpy stress, but silhouette weighting slightly improves some SSIM values:
    - `bumpy_5k`: `0.7032` (still invalid);
    - `bumpy_25k`: `0.9285` (valid);
    - `bumpy_40k`: `0.8924` (still invalid);
    - `bumpy_50k`: `0.8452` (still invalid).
  - `python3 tests/solver_validity_smoke.py simplifygeometry_v2_aggressive.cpp --cxxflags '-I /usr/include/eigen3' --extreme --timeout 180`
    passes generated 400k/1M topology/runtime.
    - `400k`: `3.34s`;
    - `1M`: `11.47s`.
- Interpretation:
  - Planar/silhouette changes are safe on current local validation and preserve runtime.
  - Local ppsurf does not appear planar-heavy enough to show a compression gain.
  - The new code should help hidden CAD-like planar meshes more than smooth local synthetic meshes.
  - Bumpy stress failures are not solved by silhouette weighting alone; next likely improvement is curvature-aware local radius / adaptive targets.

## Plan: Strawberry V9 Placement Efficiency Push

Goal: restart from `strawberry-v8_no_neighbor_guard.cpp`, not the planar/silhouette v2 branch, and test whether better collapse placement lets us spend less of the scalar Hausdorff/radius budget per accepted collapse.

Version ladder:

1. `simplifygeometry_v9_candidates.cpp`
   - Exact combined plan.
   - Candidate positions: QEM optimum, projected QEM point on the edge segment, min-radius/equalized cluster point, midpoint, endpoint A, endpoint B.
   - Candidate choice: primary raw QEM cost; tie-break smaller merged cluster radius when costs are effectively tied.
   - Neighbor guard remains disabled.
   - Cost cap coefficient raised from `0.010` to `0.012`.
   - Time budget raised from `16.0s` to `17.5s`.

2. `simplifygeometry_v10_projected_only.cpp`
   - Isolates projected QEM point.
   - Candidate positions: QEM optimum, projected QEM point, midpoint, endpoint A, endpoint B.
   - Same QEM-primary/radius-tie-break choice.
   - Baseline cost cap `0.010` and time budget `16.0s`.

3. `simplifygeometry_v11_minradius_only.cpp`
   - Isolates the min-radius/equalized cluster point.
   - Candidate positions: QEM optimum, min-radius/equalized cluster point, midpoint, endpoint A, endpoint B.
   - Same QEM-primary/radius-tie-break choice.
   - Baseline cost cap `0.010` and time budget `16.0s`.

4. `simplifygeometry_v12_candidates_cap014.cpp`
   - Aggressive sweep after v9.
   - Same candidate logic as v9.
   - Cost cap coefficient `0.014`.
   - Time budget `17.5s`.

5. `simplifygeometry_v13_candidates_basecap.cpp`
   - Control for v9.
   - Same candidate logic as v9.
   - Baseline cost cap `0.010` and baseline time budget `16.0s`.
   - Purpose: isolate placement changes from the raised cap/time knobs.

6. `simplifygeometry_v14_cap012_time175.cpp`
   - Control for v9.
   - Same candidate logic as strawberry-v8.
   - Cost cap coefficient `0.012` and time budget `17.5s`.
   - Purpose: isolate the cap/time knobs from placement changes.

7. `simplifygeometry_v15_cap014_time175.cpp`
   - Same candidate logic as strawberry-v8.
   - Cost cap coefficient `0.014` and time budget `17.5s`.
   - Purpose: test the next cost-cap sweep without candidate-placement changes.

8. `simplifygeometry_v16_time175_cap010.cpp`
   - Same candidate logic as strawberry-v8.
   - Baseline cost cap `0.010` and time budget `17.5s`.
   - Purpose: isolate time-budget increase from cost-cap increase.

9. `simplifygeometry_v17_cap012_time16.cpp`
   - Same candidate logic as strawberry-v8.
   - Cost cap coefficient `0.012` and baseline time budget `16.0s`.
   - Purpose: isolate cost-cap increase from time-budget increase.

Implementation details:

- Preserve v8 input/output, topology checks, scalar cluster-radius envelope guard, target ladder, compaction, and disabled neighbor guard.
- Projected QEM point:
  - solve unconstrained QEM;
  - clamp projection parameter `t = dot(qem-a,b-a) / |b-a|^2` into `[0,1]`;
  - use `a + t * (b-a)`.
- Min-radius cluster point:
  - let `L = |b-a|`, `ra = clusterRadius[a]`, `rb = clusterRadius[b]`;
  - if `L` is tiny, use midpoint;
  - otherwise `t = clamp((L + rb - ra) / (2L), 0, 1)`;
  - use `a + t * (b-a)`.
- Candidate comparison:
  - compute `mergedRadius = max(clusterRadius[absorbed] + |p-absorbedPos|, clusterRadius[kept] + |p-keptPos|)`;
  - prefer lower raw QEM cost;
  - if `abs(cost - bestCost) <= 1e-10 * max(1.0, abs(bestCost))`, prefer smaller `mergedRadius`.
- Queue entries estimate merged radius without rejecting on the envelope; the pop-time valid candidate selection still enforces the envelope guard.

Validation order:

1. Establish current-best baseline on `strawberry-v8_no_neighbor_guard.cpp`.
2. Run generated smooth scoring for each candidate:
   - `python3 tests/solver_validity_smoke.py <file> --cxxflags '-I /usr/include/eigen3' --large --score`
3. Run ppsurf dataset summary for each candidate:
   - compile to `/tmp/<name>`;
   - `python3 evaluate_dataset.py --python /tmp/<name> --solver ignored --dataset data/ppsurf --summary`
4. Run extreme generated runtime/topology for promising candidates:
   - `python3 tests/solver_validity_smoke.py <file> --cxxflags '-I /usr/include/eigen3' --extreme --timeout 180`
5. Optional stress:
   - `python3 tests/solver_validity_smoke.py <file> --cxxflags '-I /usr/include/eigen3' --bumpy --score`

Acceptance rule:

- Generated smooth 5k/25k/40k/50k must remain valid.
- Generated 400k/1M must finish comfortably below the official time envelope.
- ppsurf invalid count must not be worse than v8.
- Prefer the version that improves compression or official-like aggressiveness without opening new validity failures.

## Implementation Progress: V9-V17 Placement And Cap/Time Ladder

Created new source files from the clean `strawberry-v8_no_neighbor_guard.cpp` base:

- `simplifygeometry_v9_candidates.cpp`
- `simplifygeometry_v10_projected_only.cpp`
- `simplifygeometry_v11_minradius_only.cpp`
- `simplifygeometry_v12_candidates_cap014.cpp`
- `simplifygeometry_v13_candidates_basecap.cpp`
- `simplifygeometry_v14_cap012_time175.cpp`
- `simplifygeometry_v15_cap014_time175.cpp`
- `simplifygeometry_v16_time175_cap010.cpp`
- `simplifygeometry_v17_cap012_time16.cpp`

Compiled all tested versions with:

- `g++ -O2 -std=c++17 -I /usr/include/eigen3 <file> -o /tmp/<name>`

Important validation caveat:

- `tests/solver_validity_smoke.py` still has old v2 target floors, so strawberry-v8-style aggressive outputs are printed as `FAIL` on many generated cases even when topology, Hausdorff, and SSIM are acceptable.
- For these strawberry experiments, use the generated output as a score/topology/runtime diagnostic, not as a literal target-floor gate.

Generated smooth score results:

| Version | Main change | 5k | 25k | 40k | 50k |
|---|---|---:|---:|---:|---:|
| v8 baseline | current strawberry | 410 verts, SSIM 0.8694 invalid | 8750, SSIM 0.9833 valid | 7600, SSIM 0.9742 valid | 5500, SSIM 0.9697 valid |
| v9 | projected + min-radius + cap .012/time 17.5 | 386, SSIM 0.8694 invalid | 8750, SSIM 0.9823 valid | 7600, SSIM 0.9755 valid | 5500, SSIM 0.9698 valid |
| v10 | projected only | 395, SSIM 0.8704 invalid | 8750, SSIM 0.9816 valid | 7600, SSIM 0.9745 valid | 5500, SSIM 0.9690 valid |
| v11 | min-radius only | same as v9 | same as v9 | same as v9 | same as v9 |
| v12 | v9 with cap .014 | same as v9 | same as v9 | same as v9 | same as v9 |
| v13 | v9 candidate logic, baseline cap/time | same as v9 | same as v9 | same as v9 | same as v9 |
| v14 | v8 logic, cap .012/time 17.5 | same as v8 | same as v8 | same as v8 | same as v8 |
| v15 | v8 logic, cap .014/time 17.5 | same as v8 | same as v8 | same as v8 | same as v8 |

Local ppsurf dataset results:

| Version | Scenarios passed | Mean compression | Interpretation |
|---|---:|---:|---|
| v8 baseline | 4/10 | 90.213943% | Current local reference |
| v9 | 3/10 | 90.310376% | More aggressive, worse local pass count |
| v10 | 5/10 | 90.215187% | Best local quality/pass-count variant |
| v11 | 4/10 | 90.308958% | Min-radius buys compression but risks SSIM |
| v12 | 3/10 | 90.310376% | Same as v9 locally |
| v13 | 3/10 | 90.310376% | Same as v9; candidate logic caused the ppsurf shift, not cap/time |
| v14 | 4/10 | 90.213943% | Same as v8 locally |
| v15 | 4/10 | 90.213943% | Same as v8 locally |

Extreme generated runtime/topology results:

| Version | Main change | 400k result | 1M result |
|---|---|---:|---:|
| v8 baseline | current strawberry | 18,000 verts, 7.38s | 251,462 verts, 17.38s |
| v9 | projected + min-radius + cap .012/time 17.5 | 18,000 verts, 7.96s | 236,810 verts, 19.03s |
| v10 | projected only | 18,000 verts, 8.06s | 287,538 verts, 17.48s |
| v11 | min-radius only | 18,000 verts, 8.41s | 285,396 verts, 17.36s |
| v13 | v9 candidate logic, baseline cap/time | 18,000 verts, 7.93s | 275,369 verts, 17.40s |
| v14 | v8 logic, cap .012/time 17.5 | 18,000 verts, 7.49s | 193,512 verts, 18.85s |
| v15 | v8 logic, cap .014/time 17.5 | 18,000 verts, 6.29s | 127,174 verts, 18.70s |
| v16 | v8 logic, cap .010/time 17.5 | 18,000 verts, 6.32s | 172,007 verts, 18.80s |
| v17 | v8 logic, cap .012/time 16.0 | 18,000 verts, 7.14s | 268,640 verts, 17.60s |

Current interpretation:

- Projected QEM point is the safest placement addition locally: `v10` improves ppsurf pass count from 4/10 to 5/10, but it hurts 1M compression because extra candidate evaluation consumes time.
- Min-radius point is the aggressive placement addition: it improves some Hausdorff/radius numbers and compression on small varied meshes, but it can reduce SSIM and does not help 1M unless time/cap are also raised.
- The strongest immediate score candidate is not v9; it is `simplifygeometry_v15_cap014_time175.cpp`.
  - v15 matches v8 on generated smooth scoring through 50k.
  - v15 matches v8 exactly on local ppsurf.
  - v15 improves generated 1M from 251,462 kept vertices to 127,174 kept vertices while staying under 21s locally.
- `simplifygeometry_v10_projected_only.cpp` is worth keeping as the safer quality branch if official feedback shows v15 loses SSIM/Hausdorff on hidden large cases.

Optional bumpy stress for `simplifygeometry_v15_cap014_time175.cpp`:

- Command:
  - `python3 tests/solver_validity_smoke.py simplifygeometry_v15_cap014_time175.cpp --cxxflags '-I /usr/include/eigen3' --bumpy --score`
- Result:
  - smooth defaults reproduced the known strawberry behavior: 5k invalid at SSIM `0.8694`, 25k valid at SSIM `0.9833`;
  - `bumpy_5k`: 537 verts, compression `89.260%`, Hausdorff `0.1034/0.1196`, SSIM `0.6632`, invalid;
  - `bumpy_25k`: 8750 verts, compression `65.000%`, Hausdorff `0.0436/0.1203`, SSIM `0.8827`, invalid;
  - `bumpy_40k`: 7600 verts, compression `81.000%`, Hausdorff `0.0420/0.1203`, SSIM `0.8854`, invalid;
  - `bumpy_50k`: 5500 verts, compression `89.000%`, Hausdorff `0.0464/0.1203`, SSIM `0.8671`, invalid.
- Interpretation:
  - v15 is a promising large-tier compression candidate, not a cure for high-frequency detail.
  - The next quality branch should combine the safer projected-QEM behavior from v10 with adaptive targets or curvature/detail detection, rather than adding more global safety to v15.

## Plan And Results: V18-V23 Tail-Collapse Experiments From V15

Goal: use `simplifygeometry_v15_cap014_time175.cpp` as the baseline and attack the remaining huge-tier bottleneck: getting more of the last legal collapses before time/cap/heap churn stops the solver.

Created versions:

1. `simplifygeometry_v18_stats.cpp`
   - V15 behavior with optional `LOCAL_STATS` instrumentation.
   - Normal compile is submission-equivalent to v15.
   - `-DLOCAL_STATS` compile tracks stop reason, stale pops, topology rejects, envelope rejects, cost-cap rejects, elapsed time, alive vertices, and queue size.

2. `simplifygeometry_v19_tail_radius.cpp`
   - V15 plus huge-tier tail placement mode.
   - Tail trigger:
     - original vertices `> 400000`;
     - elapsed `> 11.5s` or alive vertices `< 220000`.
   - Tail placement solves QEM along the edge segment, computes the feasible QEM-cost interval under the active cap, and picks the point closest to the min-radius/equalized cluster point.
   - Tail valid-candidate choice is radius-first, then QEM.

3. `simplifygeometry_v20_tail_cap.cpp`
   - V19 plus staged huge-tier cap schedule:
     - base `0.014`;
     - after elapsed `> 13.0s` and alive `> 120000`: cap coefficient `0.016`;
     - after elapsed `> 15.0s` and alive `> 90000`: cap coefficient `0.018`.

4. `simplifygeometry_v21_tail_matching.cpp`
   - V15 plus huge-tier conflict-free tail batching.
   - Tail batch trigger:
     - original vertices `> 400000`;
     - elapsed in `[12.0s, 18.8s)`.
   - Each batch scans up to `65536` current edges, sorts low-cost candidates, greedily locks endpoints plus one-rings, then applies selected collapses sequentially with full revalidation.
   - Internal time budget raised to `18.8s`.

5. `simplifygeometry_v22_tail_combined.cpp`
   - V20 tail radius/cap schedule plus v21 batching.
   - Internal time budget `18.8s`.

6. `simplifygeometry_v23_tail_matching_safe.cpp`
   - V21 batching-only, but safer internal timing:
     - internal time budget `18.2s`;
     - batch stop elapsed `18.2s`.

Compiled:

- `g++ -O2 -std=c++17 -I /usr/include/eigen3 simplifygeometry_v18_stats.cpp -o /tmp/simplifygeometry_v18`
- `g++ -O2 -std=c++17 -I /usr/include/eigen3 -DLOCAL_STATS simplifygeometry_v18_stats.cpp -o /tmp/simplifygeometry_v18_stats`
- `g++ -O2 -std=c++17 -I /usr/include/eigen3 simplifygeometry_v19_tail_radius.cpp -o /tmp/simplifygeometry_v19`
- `g++ -O2 -std=c++17 -I /usr/include/eigen3 simplifygeometry_v20_tail_cap.cpp -o /tmp/simplifygeometry_v20`
- `g++ -O2 -std=c++17 -I /usr/include/eigen3 simplifygeometry_v21_tail_matching.cpp -o /tmp/simplifygeometry_v21`
- `g++ -O2 -std=c++17 -I /usr/include/eigen3 simplifygeometry_v22_tail_combined.cpp -o /tmp/simplifygeometry_v22`
- `g++ -O2 -std=c++17 -I /usr/include/eigen3 simplifygeometry_v23_tail_matching_safe.cpp -o /tmp/simplifygeometry_v23`

Extreme generated runtime/topology:

| Version | Main change | 400k result | 1M result |
|---|---|---:|---:|
| v15 baseline | cap `.014`, time `17.5` | 18,000 verts, 6.29s from prior run | 127,174 verts, 18.70s from prior run |
| v19 | tail radius placement | 18,000 verts, 7.34s | 202,948 verts, 18.84s |
| v20 | tail radius + staged cap | 18,000 verts, 7.20s | 185,386 verts, 18.85s |
| v21 | conflict-free tail batching | 18,000 verts, 7.26s | 107,831 verts, 19.77s |
| v22 | tail radius + staged cap + batching | 18,000 verts, 7.21s | 119,979 verts, 19.79s |
| v23 | safer batching-only | 18,000 verts, 6.18s | 100,232 verts, 19.14s |

Additional validation:

- `simplifygeometry_v21_tail_matching.cpp` generated smooth scoring through 50k matches v15 exactly:
  - 5k: 410 verts, SSIM `0.8694`, invalid;
  - 25k: 8750 verts, SSIM `0.9833`, valid;
  - 40k: 7600 verts, SSIM `0.9742`, valid;
  - 50k: 5500 verts, SSIM `0.9697`, valid.
- `simplifygeometry_v21_tail_matching.cpp` ppsurf matches v15 exactly:
  - scenarios passed `4/10`;
  - mean compression `90.213943%`.
- v23 only changes v21 timing constants and is huge-tier-only, so it should share v21/v15 behavior on small/mid local cases.

Interpretation:

- Tail radius-budgeted placement was the wrong primary bottleneck on the generated huge case:
  - v19 and v20 preserve budget too conservatively or spend too much time per collapse.
  - Combining radius placement with batching in v22 is worse than batching alone.
- Conflict-free tail batching is the useful novel idea:
  - it attacks stale heap churn and applies many independent low-cost collapses in the final phase;
  - every collapse is still revalidated with the same v15 topology/envelope checks.
- Current best experimental candidate is `simplifygeometry_v23_tail_matching_safe.cpp`.
  - It nearly reaches the local generated 1M floor: `100,232` vertices;
  - It is better than v15's `127,174` vertices;
  - It has more local time headroom than v21.

## Implementation Progress: V24-V31 Stochastic And Tuned Tail Batching

Baseline for this round: `simplifygeometry_v21_tail_matching.cpp`.

Goal: improve the huge-tier tail by tuning batch timing, conflict-lock strength, candidate source, and deterministic stochastic ordering. Also add an Optuna-based offline tuner that can generate hardcoded C++ candidates.

Created versions:

1. `simplifygeometry_v24_batch_start110_stop182.cpp`
   - v21 batching with earlier start and safer stop:
     - time budget `18.2`;
     - batch start `11.0`;
     - batch stop `18.2`;
     - full one-ring conflict locks.

2. `simplifygeometry_v25_common_lock.cpp`
   - v24 timing.
   - Lock mode changed from full one-ring to endpoints plus common neighbors.
   - Every selected collapse is still fully revalidated before application.

3. `simplifygeometry_v26_heap_batch.cpp`
   - v24 timing.
   - Batch candidates are popped/refreshed from the priority queue instead of scanning adjacency.
   - Unused candidates are pushed back; failed selected candidates are refreshed/requeued.

4. `simplifygeometry_v27_stochastic_batch.cpp`
   - v24 timing.
   - Generic deterministic stochastic batch ordering:
     - `effectiveCost = cost * (1 + jitter)`;
     - jitter is endpoint/seed/collapse-count hashed;
     - conflict mode constant supports one-ring, common-neighbor, or endpoints-only locks.
   - Fixed constants:
     - one-ring lock;
     - jitter amplitude `0.005`;
     - seed `1`.

5. `simplifygeometry_v28_jitter010_seed2.cpp`
   - v27 with jitter amplitude `0.010`, seed `2`.

6. `simplifygeometry_v29_jitter015_seed3.cpp`
   - v27 with jitter amplitude `0.015`, seed `3`.

7. `simplifygeometry_v30_jitter020_seed4.cpp`
   - v27 with jitter amplitude `0.020`, seed `4`.

8. `simplifygeometry_v31_jitter010_commonlock.cpp`
   - v27 with common-neighbor lock mode, jitter amplitude `0.010`, seed `5`.

9. `tune_v21_optuna.py`
   - Offline tuning harness using `simplifygeometry_v27_stochastic_batch.cpp` as a rewrite template.
   - Tunes:
     - time budget;
     - cost cap;
     - batch start/stop;
     - scan edge count;
     - batch target accepts;
     - conflict lock mode;
     - jitter amplitude;
     - jitter seed.
   - Runs generated extreme smoke tests and emits `simplifygeometry_v_optuna_best.cpp`.
   - Supports true Optuna/TPE when `optuna` is installed and a deterministic `--random-fallback` mode otherwise.

Compile/syntax checks:

- `python3 -m py_compile tune_v21_optuna.py`
- `g++ -O2 -std=c++17 -I /usr/include/eigen3 simplifygeometry_v24_batch_start110_stop182.cpp -o /tmp/simplifygeometry_v24`
- `g++ -O2 -std=c++17 -I /usr/include/eigen3 simplifygeometry_v25_common_lock.cpp -o /tmp/simplifygeometry_v25`
- `g++ -O2 -std=c++17 -I /usr/include/eigen3 simplifygeometry_v26_heap_batch.cpp -o /tmp/simplifygeometry_v26`
- `g++ -O2 -std=c++17 -I /usr/include/eigen3 simplifygeometry_v27_stochastic_batch.cpp -o /tmp/simplifygeometry_v27`
- `g++ -O2 -std=c++17 -I /usr/include/eigen3 simplifygeometry_v28_jitter010_seed2.cpp -o /tmp/simplifygeometry_v28`
- `g++ -O2 -std=c++17 -I /usr/include/eigen3 simplifygeometry_v29_jitter015_seed3.cpp -o /tmp/simplifygeometry_v29`
- `g++ -O2 -std=c++17 -I /usr/include/eigen3 simplifygeometry_v30_jitter020_seed4.cpp -o /tmp/simplifygeometry_v30`
- `g++ -O2 -std=c++17 -I /usr/include/eigen3 simplifygeometry_v31_jitter010_commonlock.cpp -o /tmp/simplifygeometry_v31`
- `g++ -O2 -std=c++17 -I /usr/include/eigen3 simplifygeometry_v_optuna_best.cpp -o /tmp/simplifygeometry_v_optuna_best`

Extreme generated runtime/topology:

| Version | Main change | 400k result | 1M result |
|---|---|---:|---:|
| v21 baseline | one-ring batch, start 12.0, stop 18.8 | 18,000 verts, 7.26s from prior run | 107,831 verts, 19.77s from prior run |
| v23 prior best | one-ring batch, stop 18.2 | 18,000 verts, 6.18s from prior run | 100,232 verts, 19.14s from prior run |
| v24 | one-ring batch, start 11.0, stop 18.2 | 18,000 verts, 6.45s | 74,080 verts, 19.05s |
| v25 | common-neighbor lock | 18,000 verts, 6.50s | 1,757 verts, 18.86s |
| v26 | heap-driven batch | 18,000 verts, 6.49s | 171,847 verts, 19.28s |
| v27 | jitter .005 seed 1, one-ring | 18,000 verts, 6.38s | 109,623 verts, 19.18s |
| v28 | jitter .010 seed 2, one-ring | 18,000 verts, 7.62s | 124,087 verts, 19.27s |
| v29 | jitter .015 seed 3, one-ring | 18,000 verts, 6.76s | 99,508 verts, 19.27s |
| v30 | jitter .020 seed 4, one-ring | 18,000 verts, 8.28s | 118,000 verts, 19.23s |
| v31 | jitter .010 seed 5, common-neighbor lock | 18,000 verts, 6.17s | 32,133 verts, 19.09s |

Optuna harness smoke:

- Command:
  - `python3 tune_v21_optuna.py --random-fallback --trials 1 --timeout 180 --out-source simplifygeometry_v_optuna_best.cpp`
- Result:
  - emitted `simplifygeometry_v_optuna_best.cpp`;
  - generated source compiles;
  - random-fallback trial found:
    - time budget `18.3`;
    - cost cap `0.014`;
    - batch start `11.6`;
    - batch stop `18.3`;
    - scan edges `32768`;
    - batch target `1024`;
    - conflict mode `2` endpoints-only;
    - jitter amplitude `0.020`;
    - seed `25`;
    - 400k: `18,000` vertices, `7.39s`;
    - 1M: `14,261` vertices, `19.11s`.

Interpretation:

- Earlier tail batching start is a major win:
  - v24 improves over v23 from `100,232` to `74,080` vertices on generated 1M.
- Heap-driven batching is not promising locally:
  - v26 is worse than v21/v23/v24.
- Full one-ring stochastic jitter does not beat v24 in this small sweep:
  - best one-ring jitter result was v29 at `99,508`, still worse than v24.
- Relaxing conflict locks is extremely powerful:
  - common-neighbor lock v25 reaches `1,757` vertices;
  - common-neighbor + jitter v31 reaches `32,133`;
  - endpoint-only Optuna/random candidate reaches `14,261`.
- Risk note:
  - v25/v31/Optuna-best are geometrically/topologically acceptable to the local generated smoke except for the smoke script's stale artificial 100k floor, but they may be too aggressive for hidden SSIM on real 1M cases.
  - v24 is the best conservative next candidate.
  - v25 and `simplifygeometry_v_optuna_best.cpp` are high-upside aggressive candidates that need official feedback or a stronger large-case SSIM proxy.
