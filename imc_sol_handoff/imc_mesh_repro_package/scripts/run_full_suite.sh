#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"
make all >/dev/null
OUT="results/reproduced_full"
rm -rf "$OUT"
python3 tools/imc_shape_benchmark.py \
  --solvers v23="$ROOT/build/v23" v33="$ROOT/build/v33" current="$ROOT/build/v33_t7" pineapple="$ROOT/build/pineapple_v072" \
  --eval "$ROOT/build/imc_proxy_eval" \
  --out "$OUT" \
  --resolution 256 \
  --timeout 60
printf '\nFull report: %s/%s\n' "$ROOT" "$OUT/REPORT.md"
