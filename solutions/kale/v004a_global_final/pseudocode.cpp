/*
KALE V004A — INCREMENTAL GLOBAL SSIM FOR THE FINAL STRIKE

Exact control: Kale V002B crop-area, 90.545074, PPPPPPP.

FAILURE BEING CORRECTED
    V003 compared original/current/candidate SSIM averages inside an isolated
    crop. A crop average is not an additive part of the judge score: its
    foreground-window denominator differs from the complete view and can also
    change after a silhouette weld. Thus a locally improving delta can still
    reduce whole-view SSIM catastrophically.

ALGORITHM
    execute V002B unchanged through its four weak strategic strikes
    execute V002B's first concentrated tail strike unchanged

    whenever the existing original-vs-current debt render is built:
        for each official view and each of nx, ny, nz, depth:
            cache fullCurrentSsim[view][channel]
        cache fullCurrentForegroundWindowCount[view]

    for each candidate inspected by only the final aggressive strike:
        retain all V002B legacy eligibility, crop, and topology guards
        render current-before and candidate-after with full mesh context
        extract the matching original reference crop

        for each view and channel:
            compute beforeLocalSsim and beforeLocalForegroundWindowCount
            compute afterLocalSsim and afterLocalForegroundWindowCount

            currentSum = fullCurrentSsim * fullCurrentWindowCount
            predictedAfterSum = currentSum
                              - beforeLocalSsim * beforeLocalWindowCount
                              + afterLocalSsim * afterLocalWindowCount
            predictedAfterCount = fullCurrentWindowCount
                                - beforeLocalWindowCount
                                + afterLocalWindowCount
            predictedAfterSsim = predictedAfterSum / predictedAfterCount

        predictedGlobalQuality = mean across six views of
            0.5 * (mean predicted normal-channel SSIM + predicted depth SSIM)
        localLoss = 1 - predictedGlobalQuality
        affectedFraction = mean(beforeLocalWindowCount / fullWindowCount)
        rank = localLoss
             - existing final-strike debt coefficient
               * candidateDebt * affectedFraction

        force the minimum-rank candidate with all existing hard guards

INTERPRETATION
    This is an incremental evaluation of the actual global objective, not a
    crop proxy. V002B's proven early path remains intact; only its highest-risk
    final choice is replaced. Scaling debt by affected global support makes its
    units consistent with whole-view loss; otherwise a local debt value would
    numerically overwhelm a globally diluted SSIM delta. No strike, prefix,
    threshold, topology rule, or non-T3 branch changes.
*/
