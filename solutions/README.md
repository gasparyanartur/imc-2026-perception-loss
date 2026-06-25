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

The evaluation harness (`evaluate.sh` / `evaluate_dataset.py`) defaults to
`solutions/baseline/baseline.py`. Point it at another variant with
`SCRIPT_FILE` or `--solver`.

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
