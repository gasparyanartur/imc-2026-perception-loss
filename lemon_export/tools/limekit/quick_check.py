#!/usr/bin/env python3
"""Quick offline check using synth_bench meshes."""
import os, subprocess, sys

BENCH_DIR = os.path.join(os.path.dirname(__file__), '..', '..', 'data', 'synth_bench')
EXE = os.path.join(os.path.dirname(__file__), '..', '..', 'lemon', 'lemon_v30', 'Sharon')

def main():
    if not os.path.exists(EXE):
        print(f"ERROR: {EXE} not found. Build first: cd lemon/lemon_v30 && ./build.sh")
        sys.exit(1)
    if not os.path.exists(BENCH_DIR):
        print(f"ERROR: {BENCH_DIR} not found.")
        sys.exit(1)
    print(f"Running on meshes in {BENCH_DIR} using {EXE}\n")
    for f in sorted(os.listdir(BENCH_DIR)):
        if not f.endswith('.obj'): continue
        path = os.path.join(BENCH_DIR, f)
        try:
            r = subprocess.run([EXE], stdin=open(path), capture_output=True, text=True, timeout=30)
            first_line = r.stdout.split('\n')[0] if r.stdout else "(no output)"
            print(f"  {f}: {first_line}")
        except subprocess.TimeoutExpired:
            print(f"  {f}: TIMEOUT")

if __name__ == '__main__':
    main()
