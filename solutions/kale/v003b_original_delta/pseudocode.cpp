/*
KALE V003B — SIGNED ORIGINAL-RELATIVE PATCH DELTA

Exact control: Kale V002B crop-area, 90.545074, PPPPPPP.

PROBLEM WITH THE CONTROL OBJECTIVE
    Current-vs-after SSIM is unsigned disruption. Original-to-after absolute
    fidelity (V003A) can over-penalize a candidate merely because its crop was
    already damaged before the weld. The signed delta isolates what the weld
    itself contributes to official-style error.

ALGORITHM
    execute the complete V002B / Push 21B pipeline unchanged
    cache the six existing 1024 original-input reference renders for access by
        the strategic candidate evaluator

    for each strategic directed endpoint weld in the unchanged legacy prefix:
        build the unchanged affected-face crop and hard crop-size checks
        render full current-before and candidate-after mesh context in the crop
        extract the matching original-reference crop

        beforeLoss = 1 - officialStyleLocalSSIM(original, currentBefore)
        afterLoss  = 1 - officialStyleLocalSSIM(original, candidateAfter)
        signedDelta = afterLoss - beforeLoss

        // Negative is a repair; positive is additional perceptual debt.
        weight by cropArea, the officially safe V002B support aggregation
        localLoss = sum(signedDelta * cropArea) / sum(cropArea)
        rank = localLoss - existingStrikeDebtWeight * meanDebt

        force the minimum-rank candidate with all existing hard guards

INTERPRETATION
    This is a local marginal estimate of the official objective. It permits a
    geometrically larger weld when it repairs existing normal/depth error and
    rejects a visually small weld that worsens an already fragile patch. Strike
    counts, prefixes, debt weights, topology, coverage, and every non-T3 branch
    remain unchanged. The only lever is the perceptual objective.
*/
