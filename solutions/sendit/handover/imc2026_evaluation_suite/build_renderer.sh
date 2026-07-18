#!/usr/bin/env bash
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
g++ -std=c++17 -O2 -pipe "$HERE/official_like_renderer.cpp" -o "$HERE/official_like_renderer"
echo "Built $HERE/official_like_renderer"
