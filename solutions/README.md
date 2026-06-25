# Solutions

Each subdirectory holds one solver variant, available in both Python (`.py`,
used by the evaluation harness) and C++ (`.cpp`, used when the Python runtime
exceeds the 21 second grader limit on large meshes). The Python and C++ files in
a directory implement the same algorithm and produce identical output.

| Directory   | Files                       | Description                                                                 |
|-------------|-----------------------------|-----------------------------------------------------------------------------|
| `initial/`  | `initial.py`, `initial.cpp` | I/O-only scaffold: reads the mesh, runs an empty `simplify()`, writes it back. |
| `baseline/` | `baseline.py`, `baseline.cpp` | Endpoint-only QEM edge collapse (report Solution 2).                        |
| `iter1-runtime/` | `iter1-runtime.py`     | Runtime-optimized Python variant of `baseline.py`: byte-identical output, faster `simplify()` (surgical adjacency updates, inlined quadric math, fewer allocations). |
| `iter2-budget/` | `iter2-budget.py`       | `iter1-runtime` plus an internal, self-calibrating wall-clock budget and graceful degradation for grader-scale meshes. Byte-identical to `iter1-runtime` on small meshes; on large meshes it stops collapsing before the 21 s grader limit and always emits a valid closed-manifold mesh (worst case: the input unchanged) instead of timing out. |

The evaluation harness (`evaluate.sh` / `evaluate_dataset.py`) defaults to
`solutions/baseline/baseline.py`. Point it at another variant with
`SCRIPT_FILE` or `--solver`.

## `iter2-budget`: surviving the grader's 21 s / 2 GB limits

`baseline` / `iter1-runtime` are roughly linear but with a large per-vertex
constant, so on the grader's largest meshes (~1.1M vertices) the edge-collapse
loop cannot drain the heap within 21 s. A timeout produces no valid output,
which the grader scores as 0 for that mesh — the dominant reason the solver
passes only the smallest scenarios. `iter2-budget` fixes the failure mode:

- **Self-calibrating deadline.** It times setup (load + build adjacency/quadrics
  + seed the heap), reserves a proportional slice for the compact + write tail,
  and stops collapsing early enough that the *whole program* finishes within the
  budget on any interpreter. The grader's pypy3 runs setup/tail far faster than
  CPython, so it spends more of the budget on actual collapsing.
- **Graceful degradation.** It keeps the raw input bytes and, if the interpreter
  is too slow to build the QEM structures in time, emits the input mesh
  unchanged (valid, 0 % compression) rather than timing out.
- **Lower memory.** It drops a redundant per-vertex coordinate copy (endpoint
  collapses never move a survivor) and frees the input lists once the working
  copies are built.

Tune the budget with `SIMPLIFY_BUDGET_SEC` (default 18 s); calibrate it against
the real grader, which is markedly faster than the local CPython.

## Building the C++ solvers

```sh
g++ -O2 -std=c++17 solutions/baseline/baseline.cpp -o baseline
./baseline < mesh.in > mesh.out
```

`baseline.cpp` is self-contained (standard library only). `initial.cpp` is the
competition scaffold and requires Eigen on the include path:

```sh
g++ -O2 -I /path/to/Eigen solutions/initial/initial.cpp -o initial
```
