#!/usr/bin/env python3
"""Orchestrate the unified native evaluator over a candidate suite.

The single binary in ``build/evaluators/evaluator`` reports every metric we
need (topology, geometry, Hausdorff, six-view SSIM, IoU) as KEY=VALUE pairs.
This script compiles the candidate, runs it on each dataset mesh, and
collects the results.

By default no threshold comparison is performed; the orchestrator just
records every metric. Pass --strict to compare against the documented
acceptance thresholds (SSIM >= 0.9, Hausdorff usage <= 100%, etc.).
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
DEFAULT_TIME_BUDGET = 20.2
DEFAULT_SSIM_THRESHOLD = 0.9
# Synthetic suite covers tiers 1-4, stress adds tiers 5-6.
DEFAULT_DATASETS = ["data/ppsurf", "data/synth_bench"]
TIER_BY_VERTEX_COUNT = [
    (3000, "tier1"),
    (10000, "tier2"),
    (30000, "tier3"),
    (150000, "tier4"),
    (600000, "tier5"),
    (10**12, "tier6"),
]


def classify_tier(vertex_count: int) -> str:
    for hi, name in TIER_BY_VERTEX_COUNT:
        if vertex_count <= hi:
            return name
    return "tier6+"


def inputs_for(path: Path) -> list[Path]:
    if path.is_file():
        return [path]
    return sorted(p for p in path.iterdir() if p.suffix in (".txt", ".obj"))


def parse_kv_block(text: str) -> dict[str, str]:
    out: dict[str, str] = {}
    for line in text.splitlines():
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        if "=" not in line:
            continue
        key, _, val = line.partition("=")
        out[key.strip()] = val.strip()
    return out


def parse_per_view(text: str) -> list[dict[str, float]]:
    pattern = re.compile(
        r"V(\d+)_NORMAL_SSIM=([-+0-9.eE]+) V\d+_DEPTH_SSIM=([-+0-9.eE]+) "
        r"V\d+_COMBINED_SSIM=([-+0-9.eE]+) V\d+_NORM_IOU=([-+0-9.eE]+) "
        r"V\d+_DEPTH_IOU=([-+0-9.eE]+) V\d+_FG_ORIG=(\d+) V\d+_FG_SIMP=(\d+)"
    )
    out = []
    for m in pattern.finditer(text):
        out.append({
            "view": int(m.group(1)),
            "normal_ssim": float(m.group(2)),
            "depth_ssim": float(m.group(3)),
            "combined_ssim": float(m.group(4)),
            "normal_iou": float(m.group(5)),
            "depth_iou": float(m.group(6)),
            "fg_orig": int(m.group(7)),
            "fg_simp": int(m.group(8)),
        })
    return out


def file_sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1 << 20), b""):
            digest.update(chunk)
    return digest.hexdigest()


def run_candidate(command: list[str], input_path: Path, output_path: Path,
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
        return False, (message[-1] if message else "solver failed"), elapsed
    return True, "ok", elapsed


def build_candidate(candidate: Path, temp_dir: Path) -> list[str]:
    if candidate.suffix != ".cpp":
        raise SystemExit(f"candidate must be a C++ source (*.cpp): {candidate}")
    output = temp_dir / candidate.stem
    subprocess.run([str(ROOT / "scripts/build.sh"), str(candidate), str(output)],
                   check=True)
    return [str(output)]


def run_native_evaluator(evaluator_path: Path, original: Path,
                         simplified: Path, timeout: float) -> dict[str, object]:
    cmd = [str(evaluator_path), str(original), str(simplified), "--profile"]
    try:
        result = subprocess.run(cmd, capture_output=True, text=True,
                                timeout=timeout)
    except subprocess.TimeoutExpired:
        return {"evaluator_error": f"evaluator timeout after {timeout:.0f}s"}
    kv = parse_kv_block(result.stdout)
    metrics: dict[str, object] = {"raw": kv, "per_view": parse_per_view(result.stdout)}
    if result.returncode and not kv:
        metrics["evaluator_error"] = "evaluator failed: " + (result.stderr.strip() or "no output")
    return metrics


def coerce_numeric(metrics: dict[str, object]) -> dict[str, float | int | None]:
    raw = metrics.get("raw", {})
    out: dict[str, float | int | None] = {}
    for k, v in raw.items():
        try:
            if "." in v or "e" in v.lower() or "E" in v:
                out[k] = float(v)
            else:
                out[k] = int(v)
        except ValueError:
            out[k] = v  # type: ignore[assignment]
    return out


def evaluate_one(candidate_cmd: list[str], original_path: Path,
                 timeout: float, time_budget: float, evaluator_path: Path,
                 strict: bool) -> dict:
    record: dict[str, object] = {
        "name": original_path.stem,
        "input": str(original_path),
        "valid": True,
        "compression": 0.0,
        "seconds": None,
        "note": "",
        "strict_failures": [],
    }
    with tempfile.TemporaryDirectory(prefix="mesh-eval-") as tmp:
        output_path = Path(tmp) / "simplified.txt"
        ok, message, seconds = run_candidate(candidate_cmd, original_path,
                                             output_path, timeout)
        record["seconds"] = seconds
        if not ok:
            record["valid"] = False
            record["note"] = message
            return record
        record["output_bytes"] = output_path.stat().st_size
        record["output_sha256"] = file_sha256(output_path)

        # Run the unified evaluator
        eval_metrics = run_native_evaluator(evaluator_path, original_path,
                                            output_path, timeout=180.0)
        record["metrics"] = coerce_numeric(eval_metrics)
        record["per_view"] = eval_metrics.get("per_view", [])

        if "evaluator_error" in eval_metrics:
            record["valid"] = False
            record["note"] = eval_metrics["evaluator_error"]
            return record

        m = record["metrics"]
        V_orig = int(m.get("ORIGINAL_VERTICES", 0) or 0)
        V_simp = int(m.get("SIMPLIFIED_VERTICES", 0) or 0)
        if V_orig <= 0 or V_simp <= 0:
            record["valid"] = False
            record["note"] = "evaluator produced zero-vertex result"
            return record

        record["tier"] = classify_tier(V_orig)
        record["original_vertices"] = V_orig
        record["simplified_vertices"] = V_simp
        record["compression"] = float(m.get("VERTEX_REDUCTION_PCT", 0.0) or 0.0)
        record["face_reduction"] = float(m.get("FACE_REDUCTION_PCT", 0.0) or 0.0)
        record["final_ssim"] = float(m.get("FINAL_SSIM", 0.0) or 0.0)
        record["normal_ssim"] = float(m.get("NORMAL_SSIM", 0.0) or 0.0)
        record["depth_ssim"] = float(m.get("DEPTH_SSIM", 0.0) or 0.0)
        record["hausdorff_sym"] = float(m.get("HAUSDORFF_SYM", 0.0) or 0.0)
        record["hausdorff_usage_pct"] = float(m.get("HAUSDORFF_USAGE_PCT", 0.0) or 0.0)
        record["hausdorff_limit"] = float(m.get("HAUSDORFF_LIMIT_5PCT", 0.0) or 0.0)
        record["nonmanifold_edges"] = int(m.get("NONMANIFOLD_EDGES_SIMP", 0) or 0)
        record["degenerate_faces"] = int(m.get("DEGENERATE_FACES_SIMP", 0) or 0)
        record["repeated_faces"] = int(m.get("REPEATED_FACES_SIMP", 0) or 0)
        record["orientation_errors"] = int(m.get("ORIENTATION_ERRORS_SIMP", 0) or 0)
        record["boundary_edges"] = int(m.get("BOUNDARY_EDGES_SIMP", 0) or 0)
        record["euler_chi"] = int(m.get("EULER_CHI_SIMP", 0) or 0)
        record["genus"] = int(m.get("GENUS_SIMP", -1) or -1)
        record["sharp_edges_60"] = int(m.get("SHARP_EDGES_60_SIMP", 0) or 0)
        record["min_tri_area"] = float(m.get("MIN_TRI_AREA_SIMP", 0.0) or 0.0)
        record["max_edge_len"] = float(m.get("MAX_EDGE_LEN_SIMP", 0.0) or 0.0)
        record["render_res"] = int(m.get("RENDER_RES", 0) or 0)
        record["render_ss"] = int(m.get("RENDER_SS", 0) or 0)
        record["render_threads"] = int(m.get("RENDER_THREADS", 0) or 0)
        record["hausdorff_samples"] = int(m.get("HAUSDORFF_SAMPLES", 0) or 0)
        record["normal_iou_avg"] = float(m.get("NORMAL_IOU_AVG", 1.0) or 1.0)
        record["depth_iou_avg"] = float(m.get("DEPTH_IOU_AVG", 1.0) or 1.0)
        record["fg_pixels_orig"] = int(m.get("FG_PIXELS_ORIG_SUM", 0) or 0)
        record["fg_pixels_simp"] = int(m.get("FG_PIXELS_SIMP_SUM", 0) or 0)

        # Validity heuristics: reject if topology obviously broken.
        invalid_reasons = []
        if record["nonmanifold_edges"] > 0:
            invalid_reasons.append(f"nonmanifold_edges={record['nonmanifold_edges']}")
        if record["orientation_errors"] > 0:
            invalid_reasons.append(f"orientation_errors={record['orientation_errors']}")
        if record["degenerate_faces"] > 0:
            invalid_reasons.append(f"degenerate_faces={record['degenerate_faces']}")
        if V_simp > V_orig:
            invalid_reasons.append("simplified has more vertices than original")
        if record["boundary_edges"] > 0:
            invalid_reasons.append(f"boundary_edges={record['boundary_edges']}")

        if strict:
            if record["final_ssim"] < DEFAULT_SSIM_THRESHOLD:
                invalid_reasons.append(
                    f"FinalSSIM={record['final_ssim']:.4f} < {DEFAULT_SSIM_THRESHOLD}"
                )
            if record["hausdorff_usage_pct"] > 100.0:
                invalid_reasons.append(
                    f"Hausdorff usage={record['hausdorff_usage_pct']:.1f}% > 100%"
                )

        over_budget = time_budget > 0 and seconds > time_budget
        if over_budget:
            invalid_reasons.append(f"over time budget {seconds:.2f}s > {time_budget:.2f}s")

        record["strict_failures"] = invalid_reasons
        record["valid"] = not invalid_reasons
        record["note"] = "; ".join(invalid_reasons) if invalid_reasons else "ok"
    return record


def print_report(records: list[dict], strict: bool) -> None:
    width = 150
    print("=" * width)
    print("IMC 2026 unified native evaluation")
    print("=" * width)
    header = (
        f"{'scenario':<28} {'tier':<7} {'valid':<5} {'compr%':>8} "
        f"{'SSIM':>7} {'Haus%':>7} {'IoU_N':>6} {'res':>4} "
        f"{'secs':>6} {'finger':<10}  {'note'}"
    )
    print(header)
    print("-" * width)
    for r in records:
        valid = "PASS" if r["valid"] else "FAIL"
        ssim = r.get("final_ssim")
        haus = r.get("hausdorff_usage_pct")
        iou = r.get("normal_iou_avg")
        ssim_s = f"{ssim:.4f}" if ssim is not None else "-"
        haus_s = f"{haus:.1f}" if haus is not None else "-"
        iou_s = f"{iou:.3f}" if iou is not None else "-"
        res = r.get("render_res")
        res_s = str(res) if res else "-"
        secs = r.get("seconds")
        secs_s = f"{secs:.2f}" if secs is not None else "-"
        finger = (r.get("output_sha256") or "-")[:8]
        print(
            f"{r['name']:<28} {r.get('tier','-'):<7} {valid:<5} "
            f"{r.get('compression',0.0):8.4f} {ssim_s:>7} {haus_s:>7} {iou_s:>6} "
            f"{res_s:>4} {secs_s:>6} {finger:<10}  {r.get('note','')}"
        )
    print("-" * width)
    total = len(records)
    passed = sum(1 for r in records if r["valid"])
    if total > 0:
        mean_compr = sum(float(r.get("compression", 0.0)) for r in records) / total
    else:
        mean_compr = 0.0
    print(f"Scenarios passed : {passed} / {total}")
    print(f"Mean compression : {mean_compr:.6f} %")
    if strict:
        print(f"Strict thresholds: SSIM >= {DEFAULT_SSIM_THRESHOLD}, Hausdorff usage <= 100%")
    else:
        print("Strict mode off: all reported metrics, no acceptance gate enforced")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--candidate", required=True,
                        help="C++ source file (*.cpp)")
    parser.add_argument("--dataset", action="append", default=None,
                        help="dataset file/directory; repeatable")
    parser.add_argument("--include-synthetic", action="store_true",
                        help="also evaluate data/synth_bench")
    parser.add_argument("--include-stress", action="store_true",
                        help="also evaluate data/stress (tiers 5/6)")
    parser.add_argument("--time-budget", type=float, default=DEFAULT_TIME_BUDGET)
    parser.add_argument("--solver-timeout", type=float, default=30.0)
    parser.add_argument("--strict", action="store_true",
                        help="apply documented SSIM and Hausdorff thresholds")
    parser.add_argument("--json", type=Path, help="write detailed records")
    args = parser.parse_args(argv)

    candidate = Path(args.candidate).resolve()
    if not candidate.exists():
        parser.error(f"candidate not found: {candidate}")
    if candidate.suffix.lower() != ".cpp":
        parser.error("only C++ source candidates (*.cpp) are supported")

    dataset_paths = [Path(p).resolve() for p in (args.dataset or [])]
    if not dataset_paths:
        dataset_paths = [Path(ROOT / "data/ppsurf").resolve()]
    if args.include_synthetic and not any(str(p).endswith("synth_bench") for p in dataset_paths):
        dataset_paths.append((ROOT / "data/synth_bench").resolve())
    if args.include_stress and not any(str(p).endswith("stress") for p in dataset_paths):
        dataset_paths.append((ROOT / "data/stress").resolve())

    inputs: list[Path] = []
    for dataset in dataset_paths:
        inputs.extend(inputs_for(dataset))
    if not inputs:
        parser.error("no input meshes found")

    evaluator_path = ROOT / "build/evaluators/evaluator"
    if not evaluator_path.is_file():
        parser.error("evaluator binary missing: " + str(evaluator_path)
                     + "; run scripts/build-evaluators.sh")

    with tempfile.TemporaryDirectory(prefix="candidate-build-") as tmp:
        command = build_candidate(candidate, Path(tmp))
        records = [evaluate_one(command, path, args.solver_timeout,
                                args.time_budget, evaluator_path, args.strict)
                   for path in inputs]

    print_report(records, args.strict)
    total = len(records)
    passed = sum(1 for r in records if r["valid"])
    mean = (sum(float
(r.get("compression", 0.0)) for r in records) / max(1, total))
    print(f"RESULT={'PASS' if passed == total else 'FAIL'}")
    print(f"SCENARIOS_TOTAL={total}")
    print(f"SCENARIOS_PASSED={passed}")
    print(f"COMPRESSION_RATE={mean:.6f}")
    if args.strict:
        print(f"NATIVE_SSIM_THRESHOLD={DEFAULT_SSIM_THRESHOLD:.6f}")
    if args.json:
        args.json.parent.mkdir(parents=True, exist_ok=True)
        args.json.write_text(json.dumps(records, indent=2, allow_nan=True) + "\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
