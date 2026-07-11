#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"
find . -type f \
  ! -path './build/*' \
  ! -path './generated/*' \
  ! -name 'MANIFEST.sha256' \
  ! -name '*.zip' \
  -print0 | sort -z | xargs -0 sha256sum > MANIFEST.sha256
cat MANIFEST.sha256
