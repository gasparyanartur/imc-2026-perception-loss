#!/usr/bin/env python3
"""Run internal evaluator on a candidate binary vs synth_bench."""
import argparse
import subprocess
import sys
import tempfile
from pathlib import Path

# Resolve paths relative to this script
SCRIPT_DIR = Path(__file__).resolve().parent
REPO_ROOT = SCRIPT_DIR.parent.parent

BENCH_DIR = REPO_ROOT / "data" / "synth_bench"
DIAG_FAST = REPO_ROOT / "validators" / "fast" / "diag_small"
DIAG_FULL = REPO_ROOT / "validators" / "diagnostic_v3"
HAUSDORFF = REPO_ROOT / "validators" / "hausdorff_validator"

def run_candidate(binary, mesh_path, out_path):
    """Run candidate binary on mesh, write output to out_path."""
    with open(mesh_path) as inp, open(out_path, "w") as out:
        r = subprocess.run([str(binary)], stdin=inp, stdout=out,
                          stderr=subprocess.DEVNULL, timeout=30)
    return r.returncode == 0

def eval_one(binary, mesh_path, fast=True):
    """Eval candidate on a single mesh. Returns (ssim, normal, depth)."""
    with tempfile.NamedTemporaryFile(suffix=".obj", delete=False) as f:
        out_path = Path(f.name)
    try:
        if not run_candidate(binary, mesh_path, out_path):
            return None
        diag = DIAG_FAST if fast and DIAG_FAST.exists() else DIAG_FULL
        with open(out_path) as inp:
            r = subprocess.run([str(diag), str(mesh_path), str(out_path)],
                              stdin=inp, stdout=subprocess.PIPE,
                              stderr=subprocess.DEVNULL, text=True, timeout=30)
        out = r.stdout
        # Parse: FinalSSIM=0.9055 ... NormalSSIM=0.8366 DepthSSIM=0.9907
        ssim = None
        normal = None
        depth = None
        for line in out.split("\n"):
            if line.startswith("FinalSSIM="):
                ssim = float(line.split("=")[1].split()[0])
            elif line.startswith("NormalSSIM="):
                normal = float(line.split("=")[1].split()[0])
            elif line.startswith("DepthSSIM="):
                depth = float(line.split("=")[1].split()[0])
        if ssim is None:
            return None
        return ssim, normal, depth
    finally:
        out_path.unlink(missing_ok=True)

def compare(baseline_binary, candidate_binary, mesh_names=None, fast=True):
    """Compare baseline vs candidate on synth_bench."""
    if mesh_names is None:
        mesh_names = ["tier1_bumpy_hard", "tier2_bumpy", "tier3_bumpy"]
    print(f"{'Mesh':<25} {'Baseline':<10} {'Candidate':<10} {'Diff':<10} {'Verdict'}")
    print("-" * 75)
    n_regression = 0
    for name in mesh_names:
        path = BENCH_DIR / f"{name}.obj"
        if not path.exists():
            continue
        base = eval_one(baseline_binary, path, fast=fast)
        cand = eval_one(candidate_binary, path, fast=fast)
        if base is None or cand is None:
            print(f"{name:<25} (eval failed)")
            continue
        diff = cand[0] - base[0]
        verdict = "REGRESSION" if cand[0] < 0.9 else ("BETTER" if diff > 0.01 else "OK")
        print(f"{name:<25} {base[0]:<10.4f} {cand[0]:<10.4f} {diff:<+10.4f} {verdict}")
        if cand[0] < 0.9:
            n_regression += 1
    return n_regression

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("binary", help="Path to mesh simplification binary")
    parser.add_argument("--compare", help="Compare against baseline binary")
    parser.add_argument("--mesh", help="Single mesh to evaluate")
    parser.add_argument("--fast", action="store_true", help="Use fast evaluator")
    args = parser.parse_args()

    binary = Path(args.binary).resolve()
    if not binary.exists():
        print(f"ERROR: {binary} not found")
        sys.exit(1)

    if args.mesh:
        path = Path(args.mesh)
        r = eval_one(binary, path, fast=args.fast)
        if r:
            ssim, normal, depth = r
            print(f"FinalSSIM={ssim:.4f}  NormalSSIM={normal:.4f}  DepthSSIM={depth:.4f}")
            sys.exit(0 if ssim >= 0.9 else 1)
        sys.exit(2)
    elif args.compare:
        baseline = Path(args.compare).resolve()
        n = compare(baseline, binary, fast=args.fast)
        sys.exit(0 if n == 0 else 1)
    else:
        # Eval all synth_bench meshes
        print(f"Evaluating {binary.name} on synth_bench meshes\n")
        print(f"{'Mesh':<25} {'FinalSSIM':<12} {'Normal':<10} {'Depth':<10} {'Verdict'}")
        print("-" * 70)
        n_fail = 0
        for f in sorted(BENCH_DIR.glob("*.obj")):
            r = eval_one(binary, f, fast=args.fast)
            if r is None:
                print(f"{f.name:<25} (eval failed)")
                n_fail += 1
                continue
            ssim, normal, depth = r
            verdict = "PASS" if ssim >= 0.9 else "FAIL"
            print(f"{f.name:<25} {ssim:<12.4f} {normal:<10.4f} {depth:<10.4f} {verdict}")
            if ssim < 0.9:
                n_fail += 1
        sys.exit(0 if n_fail == 0 else 1)

if __name__ == "__main__":
    main()
