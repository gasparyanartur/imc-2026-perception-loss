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
