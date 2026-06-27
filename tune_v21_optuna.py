#!/usr/bin/env python3
"""Tune v21/v27 tail-batching constants with Optuna.

The tuner rewrites only the hyperparameter block of
``simplifygeometry_v27_stochastic_batch.cpp`` into temporary C++ sources,
runs the generated extreme smoke test, parses the ``torus_1m`` result, and
emits the best source as ``simplifygeometry_v_optuna_best.cpp``.

Optuna is preferred. If it is not installed, pass ``--random-fallback`` for a
small deterministic random search using the same objective.
"""

from __future__ import annotations

import argparse
import math
import random
import re
import shutil
import subprocess
import sys
import tempfile
from dataclasses import dataclass
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent


@dataclass(frozen=True)
class TrialParams:
    time_budget: float
    cost_cap: float
    batch_start: float
    batch_stop: float
    scan_edges: int
    batch_target: int
    conflict_mode: int
    jitter_amplitude: float
    jitter_seed: int


@dataclass
class TrialResult:
    objective: float
    vertices_1m: int = 10**9
    time_1m: float = 999.0
    vertices_400k: int = 10**9
    time_400k: float = 999.0
    output: str = ""


def replace_const(source: str, name: str, value: str) -> str:
    pattern = re.compile(
        rf"(static constexpr (?:double|int|unsigned long long)\s+{re.escape(name)}\s*=\s*)[^;]+(;)"
    )
    source, count = pattern.subn(rf"\g<1>{value}\2", source)
    if count != 1:
        raise ValueError(f"could not replace {name}; matches={count}")
    return source


def render_source(template: str, params: TrialParams) -> str:
    source = template
    source = replace_const(source, "HParam_TimeBudgetSeconds", f"{params.time_budget:.6f}")
    source = replace_const(source, "HParam_QemCostCapCoeff", f"{params.cost_cap:.6f}")
    source = replace_const(source, "HParam_TailBatchElapsedStart", f"{params.batch_start:.6f}")
    source = replace_const(source, "HParam_TailBatchStopElapsed", f"{params.batch_stop:.6f}")
    source = replace_const(source, "HParam_TailBatchScanEdges", str(params.scan_edges))
    source = replace_const(source, "HParam_TailBatchTargetAccepts", str(params.batch_target))
    source = replace_const(source, "HParam_TailBatchConflictMode", str(params.conflict_mode))
    source = replace_const(source, "HParam_TailBatchJitterAmplitude", f"{params.jitter_amplitude:.6f}")
    source = replace_const(source, "HParam_TailBatchJitterSeed", f"{params.jitter_seed}ULL")
    return source


def parse_case_line(output: str, name: str) -> tuple[int, float, bool]:
    for line in output.splitlines():
        if not line.startswith(name):
            continue
        simplified = re.search(r"simplified=\s*(\d+)", line)
        elapsed = re.search(r"time=\s*([0-9.]+)s", line)
        if not simplified or not elapsed:
            break
        hard_bad = any(
            marker in line
            for marker in (
                "non-manifold",
                "invalid face",
                "face index",
                "degenerate",
                "solver error",
                "timed out",
            )
        )
        return int(simplified.group(1)), float(elapsed.group(1)), not hard_bad
    return 10**9, 999.0, False


def evaluate_source(source_path: Path, cxxflags: str, timeout: float) -> TrialResult:
    cmd = [
        sys.executable,
        "tests/solver_validity_smoke.py",
        str(source_path),
        "--cxxflags",
        cxxflags,
        "--extreme",
        "--timeout",
        str(timeout),
    ]
    proc = subprocess.run(
        cmd,
        cwd=REPO_ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=False,
    )
    output = proc.stdout
    v1m, t1m, ok1m = parse_case_line(output, "torus_1m")
    v400, t400, ok400 = parse_case_line(output, "torus_400k")

    objective = float(v1m)
    if not ok1m or not ok400:
        objective += 1_000_000_000.0
    objective += max(0.0, t1m - 20.0) * 250_000.0
    objective += max(0.0, t400 - 12.0) * 25_000.0

    return TrialResult(
        objective=objective,
        vertices_1m=v1m,
        time_1m=t1m,
        vertices_400k=v400,
        time_400k=t400,
        output=output,
    )


def sample_params_optuna(trial) -> TrialParams:
    batch_start = trial.suggest_float("batch_start", 9.8, 12.5, step=0.1)
    batch_stop = trial.suggest_float("batch_stop", 17.6, 18.8, step=0.1)
    if batch_stop <= batch_start + 4.0:
        batch_stop = batch_start + 4.0
    time_budget = batch_stop
    return TrialParams(
        time_budget=time_budget,
        cost_cap=trial.suggest_float("cost_cap", 0.014, 0.020, step=0.001),
        batch_start=batch_start,
        batch_stop=batch_stop,
        scan_edges=trial.suggest_categorical("scan_edges", [32768, 65536, 131072, 262144]),
        batch_target=trial.suggest_categorical("batch_target", [512, 1024, 2048, 4096, 8192]),
        conflict_mode=trial.suggest_categorical("conflict_mode", [0, 1, 2]),
        jitter_amplitude=trial.suggest_float("jitter_amplitude", 0.0, 0.025, step=0.0025),
        jitter_seed=trial.suggest_int("jitter_seed", 1, 32),
    )


def sample_params_random(rng: random.Random) -> TrialParams:
    batch_start = rng.choice([round(9.8 + 0.1 * i, 1) for i in range(28)])
    batch_stop = rng.choice([round(17.6 + 0.1 * i, 1) for i in range(13)])
    if batch_stop <= batch_start + 4.0:
        batch_stop = batch_start + 4.0
    return TrialParams(
        time_budget=batch_stop,
        cost_cap=rng.choice([0.014, 0.015, 0.016, 0.017, 0.018, 0.019, 0.020]),
        batch_start=batch_start,
        batch_stop=batch_stop,
        scan_edges=rng.choice([32768, 65536, 131072, 262144]),
        batch_target=rng.choice([512, 1024, 2048, 4096, 8192]),
        conflict_mode=rng.choice([0, 1, 2]),
        jitter_amplitude=rng.choice([0.0, 0.0025, 0.005, 0.0075, 0.010, 0.0125, 0.015, 0.020, 0.025]),
        jitter_seed=rng.randint(1, 32),
    )


def write_best(out_source: Path, source: str, params: TrialParams, result: TrialResult) -> None:
    header = (
        "/*\n"
        "Generated by tune_v21_optuna.py\n"
        f"objective={result.objective:.6f}\n"
        f"torus_1m_vertices={result.vertices_1m} torus_1m_time={result.time_1m:.3f}\n"
        f"torus_400k_vertices={result.vertices_400k} torus_400k_time={result.time_400k:.3f}\n"
        f"params={params}\n"
        "*/\n"
    )
    out_source.write_text(header + source, encoding="utf-8")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--template", default="simplifygeometry_v27_stochastic_batch.cpp")
    parser.add_argument("--out-source", default="simplifygeometry_v_optuna_best.cpp")
    parser.add_argument("--trials", type=int, default=40)
    parser.add_argument("--timeout", type=float, default=180.0)
    parser.add_argument("--cxxflags", default="-I /usr/include/eigen3")
    parser.add_argument("--study-name", default="imc_v21_tail_batch")
    parser.add_argument("--storage", default=None)
    parser.add_argument("--random-fallback", action="store_true")
    parser.add_argument("--seed", type=int, default=20260627)
    args = parser.parse_args(argv)

    template_path = REPO_ROOT / args.template
    template = template_path.read_text(encoding="utf-8")
    out_source = REPO_ROOT / args.out_source

    best_params: TrialParams | None = None
    best_result = TrialResult(objective=math.inf)

    with tempfile.TemporaryDirectory(prefix="imc-optuna-") as tmp_name:
        tmp = Path(tmp_name)

        def evaluate_params(params: TrialParams, label: str) -> float:
            nonlocal best_params, best_result
            source = render_source(template, params)
            source_path = tmp / f"{label}.cpp"
            source_path.write_text(source, encoding="utf-8")
            result = evaluate_source(source_path, args.cxxflags, args.timeout)
            print(
                f"{label}: objective={result.objective:.3f} "
                f"1m={result.vertices_1m} time={result.time_1m:.2f}s "
                f"400k={result.vertices_400k} time={result.time_400k:.2f}s "
                f"params={params}",
                flush=True,
            )
            if result.objective < best_result.objective:
                best_params = params
                best_result = result
                write_best(out_source, source, params, result)
                print(f"new best written to {out_source}", flush=True)
            return result.objective

        if args.random_fallback:
            rng = random.Random(args.seed)
            for i in range(args.trials):
                evaluate_params(sample_params_random(rng), f"trial_{i:04d}")
        else:
            try:
                import optuna  # type: ignore
            except ImportError:
                print(
                    "Optuna is not installed. Install optuna or rerun with --random-fallback.",
                    file=sys.stderr,
                )
                return 2

            sampler = optuna.samplers.TPESampler(seed=args.seed)
            study = optuna.create_study(
                direction="minimize",
                sampler=sampler,
                study_name=args.study_name,
                storage=args.storage,
                load_if_exists=bool(args.storage),
            )

            def objective(trial) -> float:
                return evaluate_params(sample_params_optuna(trial), f"trial_{trial.number:04d}")

            study.optimize(objective, n_trials=args.trials)

    if best_params is None:
        print("No trials completed.", file=sys.stderr)
        return 1

    print(f"BEST objective={best_result.objective:.3f} params={best_params}")
    print(f"BEST source={out_source}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
