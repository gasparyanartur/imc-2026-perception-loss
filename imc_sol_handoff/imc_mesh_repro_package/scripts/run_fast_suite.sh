#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"
make all >/dev/null
OUT="results/reproduced_fast"
rm -rf "$OUT"
python3 tools/imc_shape_benchmark.py \
  --solvers v23="$ROOT/build/v23" v33="$ROOT/build/v33" current="$ROOT/build/v33_t7" \
  --eval "$ROOT/build/imc_proxy_eval" \
  --out "$OUT" \
  --resolution 128 \
  --timeout 35 \
  --skip-hausdorff \
  --only sphere_smooth ellipsoid peanut_concave dimple_concave torus rounded_cube wavy_highfreq capsule
printf '\nFast report: %s/%s\n' "$ROOT" "$OUT/REPORT.md"
