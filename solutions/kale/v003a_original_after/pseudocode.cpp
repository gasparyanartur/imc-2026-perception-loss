/*
KALE V003A — ORIGINAL-TO-AFTER PATCH FIDELITY

Exact control: Kale V002B crop-area, 90.545074, PPPPPPP.

PROBLEM WITH THE CONTROL OBJECTIVE
    Control localLoss compares currentMesh to candidateMesh. It measures change
    magnitude but cannot tell whether that change repairs an existing error or
    moves farther from the original. Unsigned vertex debt only says where the
    current output is already wrong.

ALGORITHM
    execute the complete V002B / Push 21B pipeline unchanged
    cache the six existing 1024 original-input reference renders for access by
        the strategic candidate evaluator

    for each strategic directed endpoint weld in the unchanged legacy prefix:
        build the unchanged affected-face crop and hard crop-size checks
        render the full candidate-after mesh in the crop
        extract the matching original-reference normal/depth/foreground crop

        normalAfter = mean channel SSIM(original, candidateAfter)
        depthAfter = SSIM(original, candidateAfter)
        afterLoss = 1 - 0.5 * (normalAfter + depthAfter)

        weight by cropArea, the officially safe V002B support aggregation
        localLoss = sum(afterLoss * cropArea) / sum(cropArea)
        rank = localLoss - existingStrikeDebtWeight * meanDebt

        force the minimum-rank candidate with all existing hard guards

INTERPRETATION
    This ranks the final local rendered state, not merely perturbation size. A
    candidate that makes a larger change can win if its after-state is closer
    to the original. Strike counts, prefixes, debt weights, topology, coverage,
    and every non-T3 branch remain unchanged. The only lever is the perceptual
    objective supplied to candidate selection.
*/
