# Cherry/Goji/Sharon/lime — IMC Kattis Submission Project

## Current Status

**Champion**: lime v9 = **89.333** (75% pass rate, 16 Kattis submissions)
**Target**: 91.5 (gap 2.17 — **NOT REACHED**)
**Phase**: SELF-REFLECTION COMPLETE, awaiting structural changes

## Quick Links

| Need to... | Go to |
|---|---|
| Submit code | `/workspace/tools/limekit/submit.py` |
| Poll Kattis | `/workspace/tools/limekit/poll.py` |
| Run internal eval | `/workspace/tools/limekit/eval.py` |
| Analyze submissions | `/workspace/tools/limekit/analyze.py` |
| Read the workflow | `/workspace/instructions/MASTER.md` |
| See the model | `/workspace/data/MODEL.md` |
| See beliefs | `/workspace/data/world-belief.md` |
| See algorithm ideas | `/workspace/reports/10_ALGORITHMIC_IMPROVEMENTS.md` |

## Architecture

```
/workspace/
├── README.md               ← you are here
├── instructions/
│   ├── MASTER.md           ← canonical workflow
│   └── README.md           ← index
├── reports/                ← 5 active reports (rest archived)
│   ├── 10_ALGORITHMIC_IMPROVEMENTS.md
│   ├── FULL_BREAKDOWN.md
│   ├── LIME_FINAL_REPORT.md
│   ├── RIGOROUS_EXPLANATION.md
│   └── SELF_REFLECTION_2026_07_08.md
├── data/
│   ├── MODEL.md            ← single algorithmic source
│   ├── world-belief.md     ← 17 confirmed + 17 refuted beliefs
│   ├── LOGBOOK.md          ← activity trail
│   ├── submissions.csv     ← all submissions
│   ├── binding_constraints.csv
│   ├── lime-hypothesis-log.md
│   └── synth_bench/        ← internal test meshes
├── tools/
│   ├── limekit/            ← Python submission/eval/poll tools
│   ├── Makefile
│   └── evaluate.sh
├── validators/             ← internal evaluation tools
│   ├── diagnostic_v3       ← per-channel SSIM (slow)
│   ├── hausdorff_validator ← Hausdorff + mesh validity
│   └── fast/diag_small     ← fast version (0.85s)
├── generators/             ← synth mesh generators
├── bundles/                ← submission bundles
│   ├── lime_v9/            ← CURRENT CHAMPION
│   └── README.md
├── lime/                   ← v1-v38 + lime_v9 (champion)
├── sharon/                 ← v1-v132 (archived family)
├── goji/                   ← v1-v130 (archived family)
├── cherry/                 ← v1-v34 (oldest, archived)
└── archive/                ← all dead stuff
```

## Score progression

| Family | Champion | Score | Notes |
|---|---|---|---|
| Cherry | v24 | 88.236 | ceiling exploration |
| Goji | v98 | 88.311 | Vega tier 2/3 |
| Sharon | v114 | 89.334 | from pulsar |
| **lime** | **v9** | **89.333** | **CURRENT** |

## Self-Assessment

See `/workspace/reports/SELF_REFLECTION_2026_07_08.md` for the full
self-assessment, including:
- Workflow compliance: 4/10
- Sample efficiency: 3/10
- Insight extraction: 7/10
- Algorithmic improvement: 2/10
- Documentation: 8/10
- Code hygiene: 5/10

**Bottom line**: 100% parameter tuning, 0% structural change. To break
the 89.33 ceiling, need 4-12 hour structural change (real SSIM oracle,
beam search, NN predictor).
