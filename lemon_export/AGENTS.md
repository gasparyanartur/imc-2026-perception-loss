# AGENTS.md — Mesh Simplification Optimization (Lemon Family)

## Quick Start

See `QUICKSTART.md` for the 5-step build/submit/test cycle. Use `tools/limekit/quick_check.py` for offline testing.

## Problem Formulation

You are tuning a mesh simplification algorithm for Kattis problem `simplifygeometry`. Read the problem statement first:

- **Input**: A 3D mesh (vertices + triangular faces in OBJ-like format)
- **Output**: A simplified mesh with fewer faces/vertices
- **Objective**: Maximize compression score
- **Hard constraint**: `FinalSSIM >= 0.9` on every test case (otherwise case fails)
- **Cases**: 7 test cases with different output size bounds (10, 5k, 25k, 40k, 50k, 400k, 1.1M max verts)
- **Time limit**: 20.2 seconds per case (kill switch only)

Read `kattis_problem/` directory for the full problem statement. Then read `data/world-belief.md` for accumulated insights.

## How to Iterate

The algorithm runs 4 phases per case:
1. **QEM collapse** (vertex-bounded via `targetV`)
2. **Star collapse** (collapseInvisibleEdges, vertex-bounded)
3. **Vega SSIM pass** × N (3 passes by default, vertex-bounded)
4. **Tail batch** (only if mesh is huge)

Each phase has work parameters that limit what it does. The tier classification picks which parameter set applies. **Tune these carefully — see `iteration-instruction.md`.**

## Files in This Workspace

```
/workspace/
├── AGENTS.md                          ← this file
├── README.md
├── lemon_v<N>.cpp                     ← submission files (one per Kattis run)
├── archive/
│   └── iteration-instruction.md       ← THE iteration guide (READ THIS)
├── data/
│   ├── world-belief.md                ← accumulated insights (B1, B2, ...)
│   ├── synth_bench/                   ← offline test meshes
│   └── submissions.csv                ← all submission history
├── instructions/                      ← (legacy, mostly empty)
├── kattis_problem/                    ← problem statement
├── lemon/
│   └── lemon_v30/Sharon.cpp           ← current source (EDIT THIS)
├── tools/
│   └── limekit/submit.py              ← submission script
└── validators/                        ← output validators
```

## How to Edit the Source

The source file is `/workspace/lemon/lemon_v30/Sharon.cpp`. The file is structured:

1. **Banner** (lines 1-82): The design contract. DO NOT REMOVE.
2. **TIER_THRESHOLD + SharedParams + TierParams + tierTable[6]** (lines 83-300):
   - `SharedParams`: tier-INDEPENDENT constants (I/O, geometric constants, Vega rendering)
   - `TierParams`: per-tier parameters (one entry per tier in `tierTable`)
   - **`originalNv`** is the FROZEN input vertex count — `currentTier()` is based on this, NOT on current `nV`. This makes tuning independent per case.
3. **Algorithm code** (lines 300-1280): QEM, star, vega, tail.
4. **main()** at the bottom.

### Tier system

The 6 tiers are based on `originalNv` (the initial input vertex count):

| Tier | originalNv range | Used by case |
|------|------------------|--------------|
| T1 | 0-5,000 | case 1, case 2 (small input) |
| T2 | 5,000-25,000 | case 2, case 3 |
| T3 | 25,000-45,000 | case 3, case 4 |
| T4 | 45,000-50,000 | case 4 |
| T5 | 50,000-400,000 | case 5, case 6 |
| T6 | ≥ 400,000 | case 6, case 7 |

**Tier is FROZEN at INPUT size.** Phase work (QEM) may reduce `nV`, but `currentTier()` always uses `originalNv`. So each case maps to exactly ONE tier.

### Invariants

- **NO time thresholds** for phase decisions. Time is the kill switch only.
- All tier-dependent values live in `TierParams`, not as magic numbers.
- Every candidate must pass `vegaSsimStarPass` validation when `vegaPasses > 0`.
- The file must compile with: `g++-14 -g -O2 -std=gnu++23 -static -lrt -Wl,--whole-archive -lpthread -Wl,--no-whole-archive`

## Build and Submit

```bash
# Build
cd /workspace/lemon/lemon_v30 && ./build.sh

# Test on synth_bench (offline)
for f in /workspace/data/synth_bench/*.obj; do
  echo "$(basename $f): $(timeout 30 ./Sharon < $f 2>/dev/null | head -1)"
done

# Copy source to submission path
cp /workspace/lemon/lemon_v30/Sharon.cpp /workspace/lemon_v<N>.cpp

# Submit
python3 /workspace/tools/limekit/submit.py \
  /workspace/lemon_v<N>.cpp \
  "lemon_v<N>.cpp" \
  "lemon" \
  --hypothesis "What changed and why" \
  --predicted "expected score range"

# Check status
python3 -c "
import subprocess, json
sub_id = 'YOUR_SUBMISSION_ID'
url = f'https://imc2-cvmaxxing.arturspace.dev/submission/{sub_id}'
r = subprocess.run(['curl', '-m', '10', '-s', url], capture_output=True, text=True)
d = json.loads(r.stdout)
print('Status:', d.get('status'))
print('Score:', d.get('score'))
print('Cases:', d.get('cases'))
"
```

## Hypothesis-Based Iteration Loop

1. **Read `iteration-instruction.md`** for the current state of the algorithm and what's been tried.
2. **Form a hypothesis** about what to change. The strongest hypotheses come from:
   - Looking at which cases pass/fail (e.g., `PPPPPPF` = case 7 fails, others pass)
   - Understanding the per-case → per-tier mapping
   - Checking the per-tier SSIM/damage/pass counts
3. **Edit the source** to test the hypothesis. **Change ALL 6 tiers in ONE submission** when possible (tiers are independent — see iteration-instruction.md).
4. **Build and submit**.
5. **Wait for scoring** before submitting the next batch (max 4 in flight).
6. **Update `data/world-belief.md`** with new insights.
7. **If 3+ submissions score the same** → your change isn't reaching the algorithm. Revert and try something else.

## What NOT to Do

- Don't use time-based thresholds for phase work. Time is the kill switch only.
- Don't tune one tier at a time. Tiers are independent — change all 6 in one run.
- Don't change `originalNv` semantics. It's frozen at INPUT.
- Don't remove the banner. It's the design contract.
- Don't add fixed fallbacks to the old version. Improvements must be universal.
- Don't talk about "dynamic tiers" — that's not how this works.

## Current State

- **Best score: 89.30** with `lemon_v89.cpp` (frozen tier + T2 SSIM 0.930 + T3 SSIM 0.920, other tiers at v71 baseline)
- The previous best (89.31 with dynamic tier) used the OLD algorithm where `currentTier()` could shift mid-run. We've moved to **frozen tier** which is cleaner but requires different tuning.
- Active family: `lemon`
- See `data/world-belief.md` for the full belief state.

## Key Insights

1. **Tighter SSIM = higher SSIM but less compression**. There's a sweet spot per tier.
2. **`vegaPasses = 3` is the default**. More passes (4) can help but consume time budget.
3. **Per-tier `vegaSsimMin`** is the main compression-vs-quality knob. Lower = more compression.
4. **Per-tier `vegaDamageMax`** rejects candidates that change SSIM too much. Lower = stricter.
5. **`vegaPatchGeomFrac`** is the geometric deviation cap. 0.80 for T2/T6, 0.42 for T3, 1e100 = disabled.
6. **T6 settings need to be safe for BOTH huge initial mesh AND small post-QEM mesh** (since tier is frozen).

## Past Failures (Don't Repeat)

- Adding `originalNv` field without setting it in `readMesh()` → originalNv=0 → all meshes classified as T1 → catastrophic failure (v88 = 16.55).
- Making `currentTier()` static (frozen) without re-tuning T6 for case 7's needs → case 7 fails because T6 settings are too aggressive for the post-QEM mesh.
- Tighter `vegaSsimMin` than 0.87 on T6 → case 7 fails (FinalSSIM < 0.9).
