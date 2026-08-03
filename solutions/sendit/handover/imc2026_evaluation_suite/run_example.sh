#!/usr/bin/env bash
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
SOLVER="${1:?usage: run_example.sh /path/to/solver-or-source.cpp}"
CASES="$HERE/example_cases"
python3 "$HERE/hard_mesh_suite.py" --out-dir "$CASES" --target-vertices 30000
if [[ "$SOLVER" == *.cpp ]]; then
  g++ -std=c++17 -O2 -pipe "$SOLVER" -o "$CASES/candidate_solver"
  SOLVER="$CASES/candidate_solver"
fi
python3 "$HERE/evaluate_solver.py" --solver "$SOLVER" --root "$CASES" --variant candidate
