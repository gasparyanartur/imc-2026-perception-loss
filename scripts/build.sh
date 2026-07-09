#!/usr/bin/env bash
# Build one C++ solution with the same defaults used by the contest toolchain.
#
# Usage:
#   scripts/build.sh solutions/lemon/v115.cpp [build/v115]
#
# The source and output may be absolute paths or paths relative to the
# repository root. Override CXX, CXXFLAGS, LDFLAGS, or STATIC=0 when a local
# toolchain does not provide static linking.
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"

if [[ $# -lt 1 || $# -gt 2 ]]; then
    echo "usage: $0 SOURCE.cpp [OUTPUT]" >&2
    exit 2
fi

source_path="$1"
if [[ "$source_path" != /* ]]; then
    source_path="$repo_root/$source_path"
fi
if [[ ! -f "$source_path" ]]; then
    echo "build.sh: source file not found: $source_path" >&2
    exit 1
fi
if [[ "$source_path" != *.cpp ]]; then
    echo "build.sh: only C++ source files (*.cpp) are supported: $source_path" >&2
    exit 1
fi

if [[ $# -eq 2 ]]; then
    output_path="$2"
else
    stem="$(basename "$source_path" .cpp)"
    output_path="$repo_root/build/$stem"
fi
if [[ "$output_path" != /* ]]; then
    output_path="$repo_root/$output_path"
fi
mkdir -p "$(dirname "$output_path")"

if [[ -n "${CXX:-}" ]]; then
    cxx="$CXX"
elif command -v g++-14 >/dev/null 2>&1; then
    cxx="g++-14"
else
    cxx="g++"
fi
read -r -a cxxflags <<< "${CXXFLAGS:--g -O2 -std=gnu++23}"
read -r -a ldflags <<< "${LDFLAGS:--lrt -Wl,--whole-archive -lpthread -Wl,--no-whole-archive}"

if [[ "${STATIC:-1}" == "1" ]]; then
    cxxflags+=("-static")
fi

echo "Building $(realpath --relative-to="$repo_root" "$source_path") -> $(realpath --relative-to="$repo_root" "$output_path")"
"$cxx" "${cxxflags[@]}" "$source_path" "${ldflags[@]}" -o "$output_path"