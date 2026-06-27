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
