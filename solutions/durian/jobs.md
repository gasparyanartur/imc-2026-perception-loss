# Durian Kattis job ledger

## Kattis Batch Job Ledger (chronological)

### 2026-07-11 10:34 UTC — Batch 1 (v003, v004)
- v003: T5 weld enable only — 74.059678 PPPPPPF (case 7 fail)
- v004: T5+T6 weld enable — 74.059678 PPPPPPF (case 7 fail)

### 2026-07-11 10:39 UTC — v055 resubmit
- v055: 90.22 → 74.054286 (case 7 NOW fails per Kattis drift)

### 2026-07-11 10:45 UTC — v002 resubmit (= ariel/v32)
- v002: 90.233554 PPPPPPP — STILL passes 7/7 (this is the new champion!)

### 2026-07-11 10:39 UTC — Batch 2 (v005, v006)
- v005: T5/T6 weld scan 760 — 74.058617 PPPPPPF
- v006: T5/T6 weld+pdisk scan 760/90 — 74.058706 PPPPPPF

### 2026-07-11 10:41 UTC — Batch 3 (v007, v008)
- v007: T4 weld loosen — 74.054286 PPPPPPF (no change)
- v008: T6 keepRatio 0.028→0.027 — 74.054286 PPPPPPF (no change)

### 2026-07-11 10:45 UTC — Batch 4 (v009, v010)
- v009: T5 weld scan 1500 — 74.059678 PPPPPPF
- v010: cost cap +20% — 74.054286 PPPPPPF (HURT)

### 2026-07-11 10:47 UTC — Batch 5 (v014, v015)
- v014: T3 keepRatio 0.16→0.155 — 74.054286 PPPPPPF
- v015: T5 keepRatio 0.025→0.0245 — **74.062595** (NEW BEST at the time)

### 2026-07-11 22:22 UTC — Batch 6 (v017, v018)
- v017: T5 0.0243 + T6 0.0278 — 74.065999 PPPPPPF
- v018: T5 0.0242 — **74.067678** (NEW BEST)

### 2026-07-11 22:30 UTC — Batch 7 (v019, v020) — KEY BREAKTHROUGH
- v019: v002 + T5 0.024 — **74.099686** PPPPPPF (BEST EVER)
- v020: v002 + T2 0.31 — 74.08351 PPPPPPF

### 2026-07-11 22:34 UTC — Batch 8 (v021, v022)
- v021: v019 + T2 0.31 — 74.099686 (same as v019)
- v022: v002 + T5 0.023 — **57.820157 PPPPPFF** (case 6 ALSO fails)

### 2026-07-11 22:37 UTC — Batch 9 (v023, v024)
- v023: v019 + T4 0.095 — 74.099686 (=v019)
- v024: v019 + T6 0.031 — 74.099686 (=v019)

### 2026-07-11 22:54 UTC — Batch 10 (v025, v026)
- v025: v019 + cost cap -10% — 74.099686 (=v019)
- v026: v019 + MEMLESS screen only — **57.820157 PPPPPFF** (REGRESSED)

### 2026-07-11 23:03 UTC — Batch 11 (v027, v028)
- v027: v019 + T2 endgame tighter 0.205-0.285 — 74.08351 (LOST score)
- v028: v019 + T3 endgame tighter 0.090-0.140 — 74.08351 (LOST score)

### 2026-07-11 23:21 UTC — Batch 12 (v029, v030)
- v029: v002 + lower minSafe — 74.08351 (no improvement)
- v030: v002 + lower txGuard — **47.066303 PPFPFPF** (BROKE multiple cases)

### 2026-07-11 23:34 UTC — Batch 13 (v031, v032)
- v031: v019 + T6 0.030 — 74.099686 (=v019)
- v032: v019 + T5 0.0245 — 74.091598

### 2026-07-11 23:38 UTC — Batch 14 (v033, v034) - IN FLIGHT
- v033: v002 + T2 0.30 (tightening)
- v034: v002 + T3 0.15 (tightening)

### 2026-07-12 23:26 UTC — Batch 57 (v137, v138) — IN FLIGHT
- v137: v131 + absoluteQemEndgame minSafe 0.910
- v138: v131 + absoluteQemEndgame minSafe 0.930
- Local eval: both 94.79% compression
- Batch file: `data/submission-batches/batch-20260712-232601.json`

### 2026-07-12 23:29 UTC — Batch 58 (v139, v140) — IN FLIGHT
- v139: v131 + CParam_HausdorffDiagFraction 0.055
- v140: v131 + CParam_HausdorffDiagFraction 0.0575
- Local eval: v139 94.64% / 0.854 SSIM; v140 94.73% / 0.851 SSIM
- Batch file: `data/submission-batches/batch-20260712-232939.json`

### 2026-07-13 04:03 UTC — Batch 59 (v141, v142) — IN FLIGHT
- v141: v140 + T5 keepRatio 0.0235
- v142: v140 + T6 keepRatio 0.0195
- Local eval: both 94.73% / 0.851 SSIM
- Batch file: `data/submission-batches/batch-20260713-040338.json` (also 040625)

### 2026-07-13 04:09 UTC — Batch 60 (v145, v146) — IN FLIGHT
- v145: v129 (T5=0.0238) + T5 keepRatio 0.0235
- v146: v129 (T5=0.0238) + T5 keepRatio 0.0236
- Batch file: `data/submission-batches/batch-20260713-040941.json`
- Submission IDs: 81da8a54-404b-405d-a286-c9f8d333c521, 978aae1d-6459-445b-a345-7c83866a460f

### 2026-07-13 04:10 UTC — Batch 61 (v143, v144) — IN FLIGHT
- v143: v083 (T5=0.024, T6=0.020) + T6 keepRatio 0.0198
- v144: v083 (T5=0.024, T6=0.020) + T5 keepRatio 0.0238
- Batch file: `data/submission-batches/batch-20260713-041042.json`
- Submission IDs: 9ef6f6e4-563c-4b84-aabf-c2abd0959f47, 7da3cb67-4443-419f-a71e-f8c880c38db8

### 2026-07-12 23:29 UTC — Batch 58 (v139, v140) — IN FLIGHT
- v139: v131 + CParam_HausdorffDiagFraction 0.055
- v140: v131 + CParam_HausdorffDiagFraction 0.0575
- Local eval: v139 94.64% / 0.854 SSIM; v140 94.73% / 0.851 SSIM
- Batch file: `data/submission-batches/batch-20260712-232939.json`
- Submission IDs: d59e9644-9472-45e2-9c86-9777849ec0d2, 29b7d613-49b9-49b4-b770-e89d604cfd56


### 2026-07-12 23:26 UTC — Batch 57 (v137, v138) — IN FLIGHT
- v137: v131 + absoluteQemEndgame minSafe 0.910
- v138: v131 + absoluteQemEndgame minSafe 0.930
- Local eval: both 94.79% compression
- Batch file: `data/submission-batches/batch-20260712-232601.json`
- Submission IDs: ddccda3c-e68a-49f7-9aa4-7971173dc3e7, f18d5c8e-4a1b-4f69-9172-f5bbd44d8abc


### 2026-07-12 23:21 UTC — Batch 56 (v135, v136) — IN FLIGHT
- v135: v131 + CParam_HausdorffDiagFraction 0.0625
- v136: v131 + CParam_HausdorffDiagFraction 0.0675
- Local eval: v135 94.88% / 0.846 SSIM; v136 94.93% / 0.841 SSIM
- Batch file: `data/submission-batches/batch-20260712-232141.json`
- Submission IDs: 05f4cb28-3419-4b26-a3c0-27eb5cb938d7, 5d39a28c-6011-457c-a2e0-9643c08d53b0


### 2026-07-12 23:17 UTC — Batch 55 (v133, v134) — IN FLIGHT
- v133: v131 + CParam_HausdorffDiagFraction 0.065
- v134: v131 + CParam_HausdorffDiagFraction 0.07
- Local eval: v133 94.94% / 0.841 SSIM; v134 94.97% / 0.830 SSIM
- Batch file: `data/submission-batches/batch-20260712-231750.json`
- Submission IDs: e404bb2b-aa5c-4876-bf05-48e36f40ad88, efcf30b8-f185-4c5c-b006-d4d0e816730c


### 2026-07-12 23:14 UTC — Batch 54 (v131, v132) — IN FLIGHT
- v131: v119 + CParam_HausdorffDiagFraction 0.06 (higher compression, lower SSIM locally)
- v132: v119 + CParam_HausdorffDiagFraction 0.05 (lower compression, higher SSIM locally)
- Local eval: v131 94.79% / 0.847 SSIM; v132 94.42% / 0.865 SSIM
- Batch file: `data/submission-batches/batch-20260712-231423.json`
- Submission IDs: 78b7ae75-4a1d-4f45-8610-677fc137ef9a, dfb47a6c-763e-4435-b963-7f59178cbd07


### 2026-07-12 23:10 UTC — Batch 53 (v129, v130) — IN FLIGHT
- v129: v119 + T5 keepRatio 0.0238
- v130: v119 + T6 keepRatio 0.0198
- Local eval: both 94.64% compression
- Batch file: `data/submission-batches/batch-20260712-231045.json`
- Submission IDs: fd7e90d4-aff9-4abd-a3d6-6ebe7f476e93, b6129c7e-0568-44ed-bc13-6f73c854f6aa


### 2026-07-12 23:06 UTC — Batch 52 (v127, v128) — IN FLIGHT
- v127: v119 + HParam_QemCostCapCoeff 0.05
- v128: v119 + HParam_QemCostCapCoeff 0.025
- Local eval: both 94.64% compression
- Batch file: `data/submission-batches/batch-20260712-230628.json`
- Submission IDs: f3e9d111-e0d2-4728-a4bb-54675bcca0a8, c4c08ca8-d93d-4049-b726-af32f959ae25


### 2026-07-12 23:02 UTC — Batch 51 (v125, v126) — IN FLIGHT
- v125: v119 + vegaSsimEdgePass original-reference threshold 0.9990
- v126: v119 + vegaSsimEdgePass scan 4800 and cap 400
- Local eval: both 94.64% compression
- Batch file: `data/submission-batches/batch-20260712-230242.json`
- Submission IDs: 52bfab20-9dd0-48a3-8a86-c0ffada8f062, 90652392-445d-4be5-be2c-9c68b67c71de


### 2026-07-12 22:57 UTC — Batch 50 (v123, v124) — IN FLIGHT
- v123: v119 + absoluteQemEndgame minSafe 0.910
- v124: v119 + absoluteQemEndgame minSafe 0.930
- Local eval: both 94.64% compression
- Batch file: `data/submission-batches/batch-20260712-225755.json`
- Submission IDs: 6e0b2f4b-0923-4d51-bff8-86d4ca07c547, ca9cdc57-edbc-4318-a568-4c62f6598ec6


### 2026-07-12 22:53 UTC — Batch 49 (v121, v122) — IN FLIGHT
- v121: v119 + absoluteQemEndgame T2 0.120, T3/T4 0.055, 7 iterations
- v122: v119 + absoluteQemEndgame T2 0.115, T3/T4 0.050, 7 iterations
- Local eval: v121 94.63% compression; v122 94.63% compression
- Batch file: `data/submission-batches/batch-20260712-225351.json`
- Submission IDs: 21fcfe65-c88e-483a-b28e-97a05992ecdb, 32cf0dae-7df4-4967-a644-41a54e5d9faa


### 2026-07-12 22:46 UTC — Batch 48 (v119, v120) — IN FLIGHT
- v119: v115 + 7 binary-search iterations
- v120: v115 + relax time guard to 0.50s
- Local eval: v119 94.64% compression (new best); v120 94.63% compression
- Batch file: `data/submission-batches/batch-20260712-224647.json`
- Submission IDs: 57e3c52d-8f91-469c-8414-9116cf7f93e8, d9c87bf0-ce62-4df9-aa8f-2500c53f80a1


### 2026-07-12 22:43 UTC — Batch 47 (v117, v118) — IN FLIGHT
- v117: v115 + absoluteQemEndgame T2 0.115, T3/T4 0.050
- v118: v115 + absoluteQemEndgame T2 0.110, T3/T4 0.045
- Local eval: v117 94.61% compression; v118 94.60% compression
- Batch file: `data/submission-batches/batch-20260712-224306.json`
- Submission IDs: 260ad598-869a-4167-825a-82c333859c34, 2784999b-f1f8-4a0f-95fc-c27d72906339


### 2026-07-12 22:39 UTC — Batch 46 (v115, v116) — IN FLIGHT
- v115: v113 + absoluteQemEndgame T2 0.125, T3/T4 0.060
- v116: v113 + absoluteQemEndgame T2 0.120, T3/T4 0.055
- Local eval: v115 94.63% compression (new best); v116 94.61% compression
- Batch file: `data/submission-batches/batch-20260712-223916.json`
- Submission IDs: b7dbab82-bb6e-4efa-92ea-6e188140d6c3, e8ba5a88-0c59-4043-939f-76fd436aafb4


### 2026-07-12 22:34 UTC — Batch 45 (v113, v114) — IN FLIGHT
- v113: v107 + 6 binary-search iterations (T2 0.13, T3/T4 0.065)
- v114: v107 + relax time guard to 0.80s
- Local eval: v113 94.62% compression (new best); v114 94.60% compression
- Batch file: `data/submission-batches/batch-20260712-223440.json`
- Submission IDs: 4f86fc19-9ba3-4e09-a310-8f04225ae83a, e6ab4816-bc14-46e5-868c-720183134c67


### 2026-07-12 22:30 UTC — Batch 44 (v111, v112) — IN FLIGHT
- v111: v105 + 6 binary-search iterations in absoluteQemEndgame
- v112: v105 + relax absoluteQemEndgame time guard to 0.80s
- Local eval: v111 94.55% compression; v112 94.53% compression
- Batch file: `data/submission-batches/batch-20260712-223030.json`
- Submission IDs: a39d87aa-9360-4d0b-a516-d714a5a6cd04, abbe9b04-1da2-460a-90e9-34c94bddd68b


### 2026-07-12 22:25 UTC — Batch 43 (v109, v110) — IN FLIGHT
- v109: v105 + absoluteQemEndgame T2 0.135, T3/T4 0.072
- v110: v105 + absoluteQemEndgame T2 0.145, T3/T4 0.068
- Local eval: v109 94.57% compression; v110 94.45% compression
- Batch file: `data/submission-batches/batch-20260712-222559.json`
- Submission IDs: 99fcb47e-e3b0-4806-9eb7-194c530fc943, 50a158f2-f790-4a83-816b-b201848fa803


### 2026-07-12 22:16 UTC — Batch 41 (v105, v106) — COMPLETE
- v105: v097 + absoluteQemEndgame bounds T2 0.14/T3/T4 0.07, 5 iters — PPPPPPP 90.433026
- v106: v105 + runScreenCoreMid T2 0.28/T3 0.125 — PPFFPPP 64.515983
- Batch file: `data/submission-batches/batch-20260712-221638.json`
- Submission IDs: 2ce7c3af-88d0-4a53-873c-8adf706c40cf, 77525ba6-73ea-4d02-bc3f-147e265ad2a3


### 2026-07-12 22:21 UTC — Batch 42 (v107, v108) — IN FLIGHT
- v107: v105 + absoluteQemEndgame lower bounds T2 0.13, T3/T4 0.065
- v108: v105 + runScreenCoreMid T2 targets {0.30,0.28,0.26}
- Local eval: v107 94.60% compression; v108 94.54% compression
- Batch file: `data/submission-batches/batch-20260712-222123.json`


### 2026-07-12 22:16 UTC — Batch 41 (v105, v106) — IN FLIGHT
- v105: v097 + absoluteQemEndgame lower bounds tightened (T2 0.14, T3/T4 0.07), 5 iterations
- v106: v105 + runScreenCoreMid targets tightened (T2 final 0.28, T3 final 0.125)
- Local eval: v105 94.53% compression / 0.854 SSIM; v106 94.49% compression / 0.854 SSIM (first local improvement over 94.29% baseline)
- Batch file: `data/submission-batches/batch-20260712-221638.json`
- Submission IDs: 2ce7c3af-88d0-4a53-873c-8adf706c40cf, 77525ba6-73ea-4d02-bc3f-147e265ad2a3


### 2026-07-12 22:04 UTC — Batch 40 (v103, v104) — COMPLETE
- v103: v101 but raster-importance weights only for T5 (50k–400k) — PPPPPPP 90.420871
- v104: v103 + T5 keepRatio 0.0235 — PPPPPFP 74.153497
- Local eval: both 94.29% compression, 0.854 SSIM on ppsurf
- Batch file: `data/submission-batches/batch-20260712-220433.json`
- Submission IDs: 68994580-6836-4eb0-b5b1-3769d8ced602, 76fa3789-4bbc-4e63-8a30-b7764d2106bc


### 2026-07-12 22:04 UTC — Batch 40 (v103, v104) — IN FLIGHT
- v103: v101 but raster-importance weights only for T5 (50k–400k)
- v104: v103 + T5 keepRatio 0.0235
- Local eval: both 94.29% compression, 0.854 SSIM on ppsurf
- Batch file: `data/submission-batches/batch-20260712-220433.json`
- Submission IDs: 68994580-6836-4eb0-b5b1-3769d8ced602, 76fa3789-4bbc-4e63-8a30-b7764d2106bc


### 2026-07-12 21:27 UTC — Batch 39 (v101, v102) — COMPLETE
- v101: v097 + raster importance weights for T5/T6/T7 main QEM — PPPPPPF 74.087532
- v102: v101 + T6 keepRatio 0.0195 — PPPPPPF 74.099686
- Local eval: both 94.29% compression, 0.854 SSIM on ppsurf
- Batch file: `data/submission-batches/batch-20260712-212701.json`
- Submission IDs: ae3a8a51-e06e-4f7f-b639-1e0cd2651463, b7373216-e9c7-46e6-ae51-d9a4b50c1a74


### 2026-07-12 21:27 UTC — Batch 39 (v101, v102) — IN FLIGHT
- v101: v097 + view-dependent face weights for large tiers (build raster importance at R=256 before main QEM)
- v102: v101 + T6 keepRatio 0.0195
- Local eval: both 94.29% compression, 0.854 SSIM on ppsurf
- Batch file: `data/submission-batches/batch-20260712-212701.json`
- Submission IDs: ae3a8a51-e06e-4f7f-b639-1e0cd2651463, b7373216-e9c7-46e6-ae51-d9a4b50c1a74


### 2026-07-12 21:17 UTC — Batch 38 (v099, v100) — COMPLETE
- v099: v097 + original-reference threshold 0.9990, cap 400 — PPPPPPP 90.433026
- v100: v097 + original-reference threshold 0.9995, scan 4800, cap 400 — PPPPPPP 90.433026
- Local eval: both 94.29% compression, 0.854 SSIM on ppsurf
- Batch file: `data/submission-batches/batch-20260712-211702.json`
- Submission IDs: 844f5f1d-b7f5-475b-a251-4bd14dcd5bb9, c9e9f15e-e13c-4ea0-947d-e3d0e7c7f3fe


### 2026-07-12 21:17 UTC — Batch 38 (v099, v100) — IN FLIGHT
- v099: v097 + original-reference threshold 0.9990, cap 400
- v100: v097 + original-reference threshold 0.9995, scan 4800, cap 400
- Local eval: both 94.29% compression, 0.854 SSIM on ppsurf (identical profile)
- Batch file: `data/submission-batches/batch-20260712-211702.json`


### 2026-07-12 21:12 UTC — Batch 37 (v097, v098) — IN FLIGHT
- v097: vegaSsimEdgePass uses original-mesh reference for medium tiers (full-view render once per pass), threshold 0.9995
- v098: vegaSsimEdgePass at higher patch resolution 768 px (with v096 looser threshold)
- Local eval: both 94.29% compression, ~0.854 SSIM on ppsurf (identical profile)
- Batch file: `data/submission-batches/batch-20260712-211202.json`
- Submission IDs: 786796b5-e27c-4b7c-97cf-deebfd4c5c28, 50c85a77-3d4d-45d6-b116-80d54f4189eb


### 2026-07-12 21:00 UTC — Batch 36 (v095, v096) — COMPLETE
- v095: vegaSsimEdgePass threshold 0.99950, scan 3600, cap 300 — PPPPPPP 90.433026
- v096: vegaSsimEdgePass threshold 0.99900, scan 5000, cap 500 — PPPPPPP 90.433026
- Local eval: both 94.29% compression, 0.854 SSIM on ppsurf (identical to baseline local profile)
- Batch file: `data/submission-batches/batch-20260712-210045.json`
- Submission IDs: e12731f7-076c-4e89-8a42-04c05fe9b17e, 02c20887-7ee2-4bdf-b641-95597955edaf


### 2026-07-12 21:00 UTC — Batch 36 (v095, v096) — IN FLIGHT
- v095: vegaSsimEdgePass threshold 0.99950, scan 3600, cap 300
- v096: vegaSsimEdgePass threshold 0.99900, scan 5000, cap 500
- Local eval: both 94.29% compression, 0.854 SSIM on ppsurf (identical to baseline local profile)
- Batch file: `data/submission-batches/batch-20260712-210045.json`
- Submission IDs: e12731f7-076c-4e89-8a42-04c05fe9b17e, 02c20887-7ee2-4bdf-b641-95597955edaf
