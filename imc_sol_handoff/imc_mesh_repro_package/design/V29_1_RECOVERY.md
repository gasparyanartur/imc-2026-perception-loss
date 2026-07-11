# v29.1 Recovery Instructions

The previous downloadable `apply_v29_flip_unlock.py` was a **source generator**, not a judge-ready solver. Submitting that Python file directly causes every test to fail because it expects a C++ source filename as a command-line argument rather than reading a mesh from standard input.

v29.1 also removes the previous global timing modification. It preserves the exact v23 simplification trajectory and runs Flip-Unlock only if at least 1.30 seconds remain after the normal v23 mid-tier path.

## Files

- `apply_v29_1_flip_unlock.py`: corrected source generator.
- `build_and_smoke_test_v29_1.py`: generates, compiles, and tests the final C++ submission.

Place both scripts beside `nebula_ceiling_mix_v23.cpp`, then run:

```bash
python build_and_smoke_test_v29_1.py nebula_ceiling_mix_v23.cpp \
  -o nebula_flip_unlock_v29_1.cpp
```

A successful run ends with:

```text
PASS: generated source compiles.
PASS: official 9-vertex sample output is byte-for-byte identical to v23.
SUBMIT THIS FILE: nebula_flip_unlock_v29_1.cpp
```

Submit only:

```text
nebula_flip_unlock_v29_1.cpp
```

Do **not** submit either Python file.

## Manual build, optional

```bash
python apply_v29_1_flip_unlock.py nebula_ceiling_mix_v23.cpp \
  -o nebula_flip_unlock_v29_1.cpp

g++ -std=c++17 -O3 -DNDEBUG -march=native \
  nebula_flip_unlock_v29_1.cpp -o nebula_flip_unlock_v29_1
```

The verified builder is preferred because it catches accidental submission of the wrong artifact and confirms that the sample path is unchanged.
