#!/usr/bin/env python3
"""Orchestrate the native C++ mesh evaluator over a candidate suite.

The metrics and validity gates are implemented by the binaries in
``build/evaluators``. This file only manages candidate compilation, process
isolation, dataset iteration, and report aggregation.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import re
import subprocess
import tempfile
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SSIM_THRESHOLD = 0.9
DEFAULT_TIME_BUDGET = 20.2


def inputs_for(path: Path) -> list[Path]:
    if path.is_file():
        return [path]
    return sorted(p for p in path.iterdir() if p.suffix in (".txt", ".obj"))


def parse_metric(text: str, name: str) -> float | None:
    matches = re.findall(r"(?:^|\n).*?" + re.escape(name)
                        + r"(?:=|:)\s*([-+0-9.eE]+)", text)
    return float(matches[-1]) if matches else None


def parse_line_metric(text: str, name: str) -> float | None:
    matches = re.findall(r"^" + re.escape(name)
                        + r"=([-+0-9.eE]+)", text, re.M)
    return float(matches[-1]) if matches else None


def parse_native_views(text: str) -> list[dict[str, float]]:
    pattern = re.compile(
        r"View(\d+)_NormalSSIM=([-+0-9.eE]+) "
        r"View\d+_DepthSSIM=([-+0-9.eE]+) "
        r"View\d+_CombinedSSIM=([-+0-9.eE]+)")
    return [
        {"view": float(view), "normal": float(normal),
         "depth": float(depth), "combined": float(combined)}
        for view, normal, depth, combined in pattern.findall(text)
    ]


def file_sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1 << 20), b""):
            digest.update(chunk)
    return digest.hexdigest()


def run_process(command: list[str], input_path: Path, output_path: Path,
                timeout: float) -> tuple[bool, str, float]:
    start = time.perf_counter()
    try:
        with input_path.open("rb") as source, output_path.open("wb") as target:
            proc = subprocess.run(command, stdin=source, stdout=target,
                                  stderr=subprocess.PIPE, timeout=timeout)
    except subprocess.TimeoutExpired:
        return False, f"solver timeout after {timeout:.1f}s", timeout
    elapsed = time.perf_counter() - start
    if proc.returncode:
        message = proc.stderr.decode("utf-8", "replace").strip().splitlines()
        return False, message[-1] if message else "solver failed", elapsed
    return True, "ok", elapsed


def build_candidate(candidate: Path, temp_dir: Path) -> list[str]:
    if candidate.suffix != ".cpp":
        raise SystemExit(f"candidate must be a C++ source (*.cpp): {candidate}")
    output = temp_dir / candidate.stem
    subprocess.run([str(ROOT / "scripts/build.sh"), str(candidate), str(output)],
                   check=True)
    return [str(output)]


def native_metrics(evaluator_dir: Path, original: Path, simplified: Path,
                   samples: int, diagnostic_resolution: int) -> dict[str, object]:
    metrics: dict[str, object] = {
        "cpp_final_ssim": None, "cpp_normal_ssim": None,
        "cpp_depth_ssim": None, "surface_hausdorff": None,
        "surface_limit": None, "cpp_per_view": [],
    }
    use_fast_diag = diagnostic_resolution < 1024
    diag = evaluator_dir / ("diag_small" if use_fast_diag else "diagnostic_v3")
    if diag.exists():
        command = [str(diag), str(original), str(simplified)]
        result = subprocess.run(command,
                                capture_output=True, text=True, timeout=120)
        metrics["diagnostic_resolution"] = diagnostic_resolution
        metrics["cpp_final_ssim"] = parse_metric(result.stdout, "FinalSSIM")
        metrics["cpp_normal_ssim"] = parse_line_metric(result.stdout, "NormalSSIM")
        metrics["cpp_depth_ssim"] = parse_line_metric(result.stdout, "DepthSSIM")
        metrics["cpp_per_view"] = parse_native_views(result.stdout)
        if result.returncode and metrics["cpp_final_ssim"] is None:
            metrics["native_note"] = "diagnostic_v3 failed"

    haus = evaluator_dir / "hausdorff_validator"
    if haus.exists():
        result = subprocess.run([str(haus), str(original), str(simplified),
                                 str(samples)], capture_output=True, text=True,
                                timeout=120)
        metrics["surface_hausdorff"] = parse_metric(
            result.stdout, "Hausdorff symmetric")
        metrics["surface_limit"] = parse_metric(
            result.stdout, "Hausdorff limit (5% diag)")
    return metrics


def native_validity(evaluator_dir: Path, original: Path,
                    simplified: Path) -> dict[str, object]:
    validator = evaluator_dir / "mesh_validity"
    if not validator.exists():
        return {"native_validity_ok": False,
                "native_validity_note": "mesh_validity unavailable"}
    result = subprocess.run([str(validator), str(original), str(simplified)],
                            capture_output=True, text=True, timeout=30)
    values: dict[str, object] = {"native_validity_ok": False}
    for line in result.stdout.splitlines():
        if "=" not in line:
            continue
        key, value = line.split("=", 1)
        if key == "VALIDITY":
            values["native_validity_ok"] = value == "VALID"
        elif value.isdigit():
            values[key.lower()] = int(value)
    if result.returncode and "native_validity_note" not in values:
        values["native_validity_note"] = "native validity gate failed"
    return values


def evaluate_one(candidate_cmd: list[str], original_path: Path,
                 timeout: float, time_budget: float, evaluator_dir: Path,
                 surface_samples: int, diagnostic_resolution: int,
                 metrics_max_vertices: int) -> dict:
    record = {
        "name": original_path.stem, "input": str(original_path),
        "valid": False, "compression": 0.0, "seconds": None, "note": "",
    }
    with tempfile.TemporaryDirectory(prefix="mesh-eval-") as tmp:
        output_path = Path(tmp) / "simplified.txt"
        ok, message, seconds = run_process(candidate_cmd, original_path,
                                            output_path, timeout)
        record["seconds"] = seconds
        if not ok:
            record["note"] = message
            return record
        record["output_bytes"] = output_path.stat().st_size
        record["output_sha256"] = file_sha256(output_path)

        validity = native_validity(evaluator_dir, original_path, output_path)
        record.update(validity)
        original_vertices = int(validity.get("original_vertices", 0))
        simplified_vertices = int(validity.get("simplified_vertices", 0))
        if not original_vertices or not simplified_vertices:
            record["note"] = "native mesh parser unavailable"
            return record
        record["original_vertices"] = original_vertices
        record["simplified_vertices"] = simplified_vertices
        record["compression"] = 100.0 * (1.0 - simplified_vertices
                                          / original_vertices)
        record["manifold_ok"] = bool(validity.get("native_validity_ok"))
        record["validity_reasons"] = []

        run_metrics = (metrics_max_vertices <= 0
                       or original_vertices <= metrics_max_vertices)
        if run_metrics:
            record.update(native_metrics(evaluator_dir, original_path, output_path,
                                         surface_samples, diagnostic_resolution))
            cpp_ssim = record.get("cpp_final_ssim")
            native_ok = cpp_ssim is not None and float(cpp_ssim) >= SSIM_THRESHOLD
            surface_h = record.get("surface_hausdorff")
            surface_bound = record.get("surface_limit")
            geometry_ok = bool(record["manifold_ok"])
            if surface_h is None or surface_bound is None:
                geometry_ok = False
            else:
                geometry_ok = (geometry_ok
                               and float(surface_h) <= float(surface_bound))
        else:
            record["metrics_skipped"] = True
            record["diagnostic_resolution"] = diagnostic_resolution
            cpp_ssim = None
            native_ok = True
            surface_h = None
            surface_bound = None
            geometry_ok = bool(record["manifold_ok"])

        over_budget = time_budget > 0 and seconds > time_budget
        record["native_ssim_ok"] = native_ok if run_metrics else None
        record["over_budget"] = over_budget
        record["valid"] = bool(native_ok and geometry_ok and not over_budget)

        notes = []
        if not record["manifold_ok"]:
            notes.append("native topology/geometry gate failed")
        if surface_h is not None and surface_bound is not None \
                and float(surface_h) > float(surface_bound):
            notes.append(f"surface Hausdorff {surface_h:.6f} > {surface_bound:.6f}")
        if run_metrics and not native_ok:
            notes.append(f"native FinalSSIM {cpp_ssim} < {SSIM_THRESHOLD:.2f}")
        if not run_metrics:
            notes.append("perceptual/Hausdorff metrics skipped for fast high-tier probe")
        if over_budget:
            notes.append(f"over time budget {seconds:.2f}s > {time_budget:.2f}s")
        record["note"] = "; ".join(notes) or "ok"
    return record


def print_report(records: list[dict]) -> None:
    print("=" * 132)
    print("IMC 2026 native C++ evaluation")
    print("=" * 132)
    print(f"{'scenario':<28} {'result':<7} {'compr%':>8} {'cppSSIM':>8} "
          f"{'sHaus':>9} {'secs':>7} {'finger':<8}  note")
    print("-" * 132)
    for record in records:
        def value(key: str, fmt: str = ".4f") -> str:
            value = record.get(key)
            return "-" if value is None else format(value, fmt)
        print(f"{record['name']:<28} "
              f"{('VALID' if record['valid'] else 'INVALID'):<7} "
              f"{record.get('compression', 0.0):8.4f} "
              f"{value('cpp_final_ssim'):>8} "
              f"{value('surface_hausdorff'):>9} "
              f"{value('seconds', '.2f'):>7} "
              f"{str(record.get('output_sha256', '-'))[:8]:<8}  "
              f"{record.get('note', '')}")
    total = len(records)
    passed = sum(bool(record["valid"]) for record in records)
    mean = sum(float(record.get("compression", 0.0))
               for record in records) / max(1, total)
    print("-" * 132)
    print(f"Scenarios passed : {passed} / {total}")
    print(f"Mean compression : {mean:.6f} %")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--candidate", required=True,
                        help="C++ source file (*.cpp)")
    parser.add_argument("--dataset", action="append", default=None,
                        help="dataset file/directory; repeatable")
    parser.add_argument("--include-synthetic", action="store_true",
                        help="also evaluate data/synth_bench")
    parser.add_argument("--time-budget", type=float, default=DEFAULT_TIME_BUDGET)
    parser.add_argument("--solver-timeout", type=float, default=30.0)
    parser.add_argument("--surface-samples", type=int, default=500)
    parser.add_argument(
        "--diagnostic-resolution", type=int, choices=(256, 1024), default=1024,
        help="native SSIM render resolution; 256 is a fast diagnostic, "
             "1024 matches the full local acceptance path")
    parser.add_argument(
        "--metrics-max-vertices", type=int, default=0,
        help="skip SSIM/Hausdorff above this original vertex count while still "
             "checking solver time, topology, counts, and output hash; "
             "0 evaluates metrics on every mesh")
    parser.add_argument("--json", type=Path, help="write detailed records")
    args = parser.parse_args(argv)

    candidate = Path(args.candidate).resolve()
    if not candidate.exists():
        parser.error(f"candidate not found: {candidate}")
    if candidate.suffix.lower() != ".cpp":
        parser.error("only C++ source candidates (*.cpp) are supported")

    dataset_paths = [Path(p).resolve() for p in (args.dataset or ["data/ppsurf"])]
    if args.include_synthetic:
        dataset_paths.append((ROOT / "data/synth_bench").resolve())
    inputs = []
    for dataset in dataset_paths:
        inputs.extend(inputs_for(dataset))
    if not inputs:
        parser.error("no input meshes found")

    evaluator_dir = ROOT / "build/evaluators"
    diagnostic_binary = ("diag_small" if args.diagnostic_resolution < 1024
                         else "diagnostic_v3")
    required_evaluators = (diagnostic_binary, "hausdorff_validator",
                           "mesh_validity")
    missing = [name for name in required_evaluators
               if not (evaluator_dir / name).is_file()]
    if missing:
        parser.error("native evaluator binaries missing: " + ", ".join(missing)
                     + "; run scripts/build-evaluators.sh")

    with tempfile.TemporaryDirectory(prefix="candidate-build-") as tmp:
        command = build_candidate(candidate, Path(tmp))
        records = [evaluate_one(command, path, args.solver_timeout,
                                args.time_budget, evaluator_dir,
                                args.surface_samples, args.diagnostic_resolution,
                                args.metrics_max_vertices)
                   for path in inputs]

    print_report(records)
    total = len(records)
    passed = sum(bool(record["valid"]) for record in records)
    mean = sum(float(record.get("compression", 0.0))
               for record in records) / total
    print(f"RESULT={'VALID' if passed == total else 'INVALID'}")
    print(f"SCENARIOS_TOTAL={total}")
    print(f"SCENARIOS_PASSED={passed}")
    print(f"COMPRESSION_RATE={mean:.6f}")
    print(f"NATIVE_SSIM_THRESHOLD={SSIM_THRESHOLD:.6f}")
    if args.json:
        args.json.parent.mkdir(parents=True, exist_ok=True)
        args.json.write_text(json.dumps(records, indent=2, allow_nan=True) + "\n")
    return 0 if passed == total else 1


if __name__ == "__main__":
    raise SystemExit(main())
