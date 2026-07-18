#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
import json
import subprocess
import time
from pathlib import Path

from mesh_validate import read_mesh, validate


def main() -> int:
    parser = argparse.ArgumentParser(description="Run a solver on *.in cases, validate outputs, compute six-view SSIM, and create contact sheets.")
    parser.add_argument("--solver", type=Path, required=True)
    parser.add_argument("--root", type=Path, required=True, help="Directory containing *.in meshes")
    parser.add_argument("--variant", default="candidate")
    parser.add_argument("--timeout", type=float, default=45.0)
    parser.add_argument("--resolution", type=int, default=1024)
    parser.add_argument("cases", nargs="*")
    args = parser.parse_args()

    suite = Path(__file__).resolve().parent
    inputs = [args.root / f"{name}.in" for name in args.cases] if args.cases else sorted(args.root.glob("*.in"))
    if not inputs:
        raise SystemExit(f"No .in files found in {args.root}")

    render_root = args.root / f"renders_{args.variant}"
    render_root.mkdir(parents=True, exist_ok=True)
    records: list[dict[str, object]] = []

    for input_path in inputs:
        case = input_path.stem
        output_path = args.root / f"{case}.{args.variant}.out"
        stderr_path = args.root / f"{case}.{args.variant}.err"
        start = time.perf_counter()
        try:
            with input_path.open("rb") as src, output_path.open("wb") as dst, stderr_path.open("wb") as log:
                proc = subprocess.run([str(args.solver.resolve())], stdin=src, stdout=dst, stderr=log, timeout=args.timeout)
            timed_out = False
        except subprocess.TimeoutExpired:
            proc = None
            timed_out = True
        seconds = time.perf_counter() - start

        record: dict[str, object] = {"case": case, "seconds": seconds, "timed_out": timed_out}
        input_mesh = read_mesh(input_path)
        record["input_vertices"] = len(input_mesh.vertices)
        record["input_faces"] = len(input_mesh.faces)

        if timed_out or proc is None or proc.returncode != 0:
            record.update(exit_code=None if proc is None else proc.returncode, valid=False)
            records.append(record)
            print(f"{case:28s} FAIL timeout/exit")
            continue

        output_mesh = read_mesh(output_path)
        validity = validate(output_mesh)
        record.update(
            exit_code=proc.returncode,
            output_vertices=len(output_mesh.vertices),
            output_faces=len(output_mesh.faces),
            removed_vertices=len(input_mesh.vertices) - len(output_mesh.vertices),
            compression_percent=100.0 * (1.0 - len(output_mesh.vertices) / len(input_mesh.vertices)),
            valid=validity.valid,
            validation=validity.__dict__,
        )

        destination = render_root / case
        subprocess.run(
            [
                "python3", str(suite / "make_case_sheet.py"),
                str(input_path), str(output_path),
                "--dest", str(destination),
                "--case", case,
                "--resolution", str(args.resolution),
            ],
            check=True,
            stdout=subprocess.DEVNULL,
        )
        metrics = json.loads((destination / "render_metrics.json").read_text())
        for key in ("mean_ssim", "min_view_ssim", "min_normal_ssim", "min_depth_ssim", "contact_sheet"):
            record[key] = metrics[key]
        stderr_lines = stderr_path.read_text(errors="replace").splitlines()
        record["diagnostics"] = stderr_lines[-1] if stderr_lines else ""
        records.append(record)
        print(
            f"{case:28s} V={len(output_mesh.vertices):7d} valid={validity.valid} "
            f"SSIM={metrics['mean_ssim']:.6f} Nmin={metrics['min_normal_ssim']:.6f} "
            f"Dmin={metrics['min_depth_ssim']:.6f} t={seconds:.2f}s"
        )

    json_path = args.root / f"visual_results_{args.variant}.json"
    csv_path = args.root / f"visual_results_{args.variant}.csv"
    json_path.write_text(json.dumps(records, indent=2))
    flat_records = []
    for record in records:
        flat = {k: v for k, v in record.items() if not isinstance(v, (dict, list))}
        flat_records.append(flat)
    keys = sorted({key for record in flat_records for key in record})
    with csv_path.open("w", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=keys)
        writer.writeheader()
        writer.writerows(flat_records)
    print(f"Wrote {json_path}")
    print(f"Wrote {csv_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
