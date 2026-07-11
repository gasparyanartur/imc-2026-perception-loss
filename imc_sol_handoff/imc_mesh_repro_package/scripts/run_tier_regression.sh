#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"
INCLUDE_HUGE=0
if [[ "${1:-}" == "--include-huge" ]]; then INCLUDE_HUGE=1; fi
make all >/dev/null
OUT="$ROOT/results/reproduced_tiers"
MESHES="$ROOT/generated/tier_meshes"
rm -rf "$OUT"
mkdir -p "$OUT" "$MESHES"

run_case() {
  local preset="$1" shape="$2" resolution="$3" timeout="$4"
  local input="$MESHES/${preset}_${shape}.mesh"
  python3 tools/imc_generate_tier_mesh.py "$input" --preset "$preset" --shape "$shape"
  for solver in v23 v33 v33_t7; do
    local exe="$ROOT/build/$solver"
    local output="$OUT/${preset}_${shape}__${solver}.mesh"
    echo "[$preset/$shape] $solver"
    timeout "$timeout" "$exe" < "$input" > "$output"
    python3 tools/imc_validate_mesh.py "$output" --pretty > "$OUT/${preset}_${shape}__${solver}.validation.json"
    "$ROOT/build/imc_proxy_eval" "$input" "$output" "$resolution" > "$OUT/${preset}_${shape}__${solver}.proxy.json"
  done
  python3 tools/imc_compare_outputs.py \
    "$OUT/${preset}_${shape}__v33.mesh" "$OUT/${preset}_${shape}__v33_t7.mesh" \
    --original "$input" --evaluator "$ROOT/build/imc_proxy_eval" --resolution "$resolution" --pretty \
    > "$OUT/${preset}_${shape}__v33_vs_current.json"
}

# Small/medium tier isolation tests. These should be byte-identical between v33 and current.
run_case t2 sphere 128 45
run_case t3 ellipsoid 128 50
run_case t4 peanut 128 55

if [[ "$INCLUDE_HUGE" -eq 1 ]]; then
  echo "Generating and running the 1,024,002-vertex T7 diagnostic. This is intentionally opt-in."
  run_case t7 sphere 64 90
  run_case t7 ellipsoid 64 90
  run_case t7 peanut 64 90
else
  echo "Skipping T7. Re-run with --include-huge to generate 1,024,002-vertex diagnostics."
fi

echo "Tier regression outputs: $OUT"
