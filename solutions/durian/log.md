# Durian iteration log
# Durian iteration log

## Goal
Beat the v072 baseline score (90.5 pre-drift, ~74.05 post-drift). Kattis-safe candidates scoring 7/7 (PPPPPPP) need to exceed 74.06.

## Strategy
v072 has T5/T6 weld/pair-disk DISABLED. Pineapple log shows enabling them gave +0.005pt — but Kattis may behave differently. Test systematically.

## Iteration

### Batch 1 — durian-v003, durian-v004
**Hypothesis:** Enabling T5 weld (v003) and T5+T6 weld (v004) per Pineapple v079 direction.

| Version | Description | Kattis cases | Kattis score |
|---|---|---|---|
| durian-v003 | v072 + T5 weld enable (maxValence 10, scan 1500) | PPPPPPF | 74.059678 |
| durian-v004 | v072 + T5+T6 weld enable (T5 val 10 / scan 1500, T6 val 8 / scan 2500 / maxSec 5.0) | PPPPPPF | 74.059678 |

**KEY FINDING:** Both v003 AND v004 break case 7. T5 weld at scanVertices 1500 alone is too aggressive for hidden case 7. Score 74.059678 (case 7 still fails).

**Implication:** T5 weld direction is DEAD for v072-like configs. Need either much smaller T5 scanVertices (e.g., 600) or skip T5 weld entirely. The T7 mesh (case 7) likely has many small flat patches where weld deletes collapse good detail.
