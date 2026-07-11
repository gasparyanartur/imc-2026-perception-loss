# Pineapple Kattis job ledger

This file is the durable retrieval index for Pineapple Kattis submissions. Per
iterate.md, every batch launch is appended here with launch time, source
filenames, batch/status file, returned submission IDs, and final results.

## Format
```
## YYYY-MM-DD HH:MM:SS UTC — <brief description>
- Source files: vNNN.cpp vMMM.cpp
- Batch file: data/submission-batches/batch-YYYYMMDD-HHMMSS.json
- Submission IDs: <id1>, <id2>
- Launch time: YYYY-MM-DD HH:MM:SS UTC
- Final status: <pending|scored|failed|cancelled>
- Scores: vNNN=<score> vMMM=<score>
- Case strings: vNNN=<cases> vMMM=<cases>
```

## 2026-07-11 06:56:57 UTC — batch 32 (v183, v184)
- Source files: solutions/pineapple/v183.cpp, solutions/pineapple/v184.cpp
- Hypothesis: extend v182's T4 weld push without crossing Kattis timeout risk
  - v183: v182 + T4 weld maxValence 16→20 + scanVertices 2800→3000
  - v184: v182 + T4 weld maxSeconds 2.30→5.00
- Status: in progress (build → local eval → submit → poll)

---