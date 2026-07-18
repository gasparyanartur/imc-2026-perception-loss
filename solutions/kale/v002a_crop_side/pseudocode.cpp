/*
KALE V002A — CROP-SIDE-WEIGHTED STRATEGIC LOSS

Exact control: Push 21B, 90.545074, PPPPPPP. Do not include Batch 01's T2 flank.

Execute the complete Push 21B algorithm and every tier schedule unchanged.

Only inside the existing full-context strategic endpoint-weld evaluator:
    totalWeightedLoss = 0
    totalWeight = 0
    for each affected official view:
        build the unchanged projected union crop with 6-pixel SSIM margin
        reject unchanged crop limits: side < 11 or side > 384
        render unchanged full current-mesh before/after context
        viewLoss = 1 - 0.5 * (normalSSIM + depthSSIM)

        // Compact proxy for affected foreground-window mass.
        viewWeight = cropSide
        totalWeightedLoss += viewLoss * viewWeight
        totalWeight += viewWeight

    localLoss = totalWeightedLoss / totalWeight
    rank = localLoss - currentStrikeDebtWeight * meanDebt

Preserve the exact four weak, one concentrated, and one aggressive Push 21B
strikes, candidate prefixes, topology/area/orientation checks, coverage
witnesses, and all non-T3 code. The changed lever is only per-view aggregation.
*/
