#!/usr/bin/env python3
"""Compare two solver outputs by bytes, counts, topology, and optional proxy score."""
from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
from pathlib import Path

from imc_validate_mesh import read_mesh, validate


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def proxy(executable: Path, original: Path, candidate: Path, resolution: int) -> dict:
    text = subprocess.check_output(
        [str(executable), str(original), str(candidate), str(resolution)], text=True
    )
    return json.loads(text)


def main() -> None:
    p = argparse.ArgumentParser()
    p.add_argument("first", type=Path)
    p.add_argument("second", type=Path)
    p.add_argument("--original", type=Path)
    p.add_argument("--evaluator", type=Path)
    p.add_argument("--resolution", type=int, default=256)
    p.add_argument("--pretty", action="store_true")
    args = p.parse_args()

    av, af = read_mesh(args.first)
    bv, bf = read_mesh(args.second)
    out = {
        "byte_identical": args.first.read_bytes() == args.second.read_bytes(),
        "first_sha256": sha256(args.first),
        "second_sha256": sha256(args.second),
        "first": validate(av, af),
        "second": validate(bv, bf),
        "vertex_delta_second_minus_first": int(len(bv) - len(av)),
        "face_delta_second_minus_first": int(len(bf) - len(af)),
    }
    if args.original and args.evaluator:
        out["first_proxy"] = proxy(args.evaluator, args.original, args.first, args.resolution)
        out["second_proxy"] = proxy(args.evaluator, args.original, args.second, args.resolution)
    print(json.dumps(out, indent=2 if args.pretty else None, sort_keys=True))


if __name__ == "__main__":
    main()
