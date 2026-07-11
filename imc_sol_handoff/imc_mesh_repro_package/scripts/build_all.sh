#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"
make all
printf '\nBuilt executables:\n'
ls -lh build/v33_t7 build/v23 build/v33 build/pineapple_v072 build/imc_proxy_eval
