/*
KALE V004B — INCREMENTAL GLOBAL SSIM FOR BOTH TAIL STRIKES

Exact control: Kale V002B crop-area, 90.545074, PPPPPPP.

ALGORITHM
    use the same exact incremental global SSIM calculation specified in
    V004A: replace affected original-vs-current window sums and counts with
    affected original-vs-candidate sums and counts inside the cached complete
    six-view objective. Scale the inherited debt contribution by the affected
    fraction of global foreground windows so loss and debt share global units.

    execute V002B's four weak strikes unchanged

    for the first concentrated tail strike:
        rerender the complete current mesh as V002B already does
        cache full-view SSIM sums/counts
        rank its first eight legacy candidates by predicted global after-state
        minus the unchanged 1e-4 debt term

    for the final aggressive tail strike:
        rerender and refresh complete sums/counts after the preceding commit
        rank its first eight candidates by predicted global after-state
        minus the unchanged 5e-4 debt term

    retain every existing hard guard and force one candidate per tail strike

INTERPRETATION
    V004A is the conservative scope test. V004B tests whether exact additive
    global accounting should govern the entire concentrated tail once the
    early conservative path has established a safe mesh. The experiment is
    about objective scope, not hyperparameter values; all numerical schedules
    are inherited unchanged.
*/
