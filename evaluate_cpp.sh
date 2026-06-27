#!/usr/bin/env bash
#
# Compile and score a C++ mesh simplifier with the local dataset evaluator.
# Usage:
#   ./evaluate_cpp.sh simplifygeometry_v2_aggressive.cpp
#   DATASET_DIR=data/ppsurf RESOLUTION=1024 ./evaluate_cpp.sh my_solver.cpp
#   CXXFLAGS="-I /usr/include/eigen3" ./evaluate_cpp.sh simplifygeometry.cpp
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$repo_root"

src="${1:-simplifygeometry_v2_aggressive.cpp}"
dataset_dir="${DATASET_DIR:-data/ppsurf}"
cxx="${CXX:-g++}"
read -r -a cxxflags <<< "${CXXFLAGS:-}"
tmp_bin="${TMPDIR:-/tmp}/$(basename "$src" .cpp)-$$"

cleanup() {
    rm -f "$tmp_bin"
}
trap cleanup EXIT

"$cxx" -O2 -std=c++17 "${cxxflags[@]}" "$src" -o "$tmp_bin"

eval_args=(evaluate_dataset.py --python "$tmp_bin" --solver ignored
           --dataset "$dataset_dir" --summary)
if [ -n "${RESOLUTION:-}" ]; then
    eval_args+=(--resolution "$RESOLUTION")
fi

python3 "${eval_args[@]}"
