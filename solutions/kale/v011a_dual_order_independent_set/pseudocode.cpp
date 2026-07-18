/*
KALE V011A — DUAL-ORDER PERCEPTUAL INDEPENDENT SET

Exact base: V010A, including the validated T2 independent pair and the robust
five-strike T3 terminal budget.

Change only the exact-window collapse-wave selector when the input is in T3.
Candidate generation, placement, topology guards, exact per-view SSIM deltas,
damage budgets, overlap rule, and final full-render audit remain unchanged.

Evaluate the existing candidate pool once. For each candidate, retain:
    exact additive per-view/per-channel SSIM ledger delta
    exact foreground-window-count delta
    topology endpoints and one-ring
    projected affected rectangles
    ordinary damage and future-collapsibility values

Define two candidates as conflicting when the existing selector could not take
both: their endpoint/one-ring locks intersect, or their projected rectangles
exceed the wave's overlap allowance.

Construct two feasible sets without changing the mesh:

    damage_first = existing greedy order by:
        damage, then future collapsibility, then QEM cost

    conflict_aware = greedy order by:
        opportunityCost = max(0, damage)
                        + waveBudget * conflictDegree / candidateCount
        then damage, then QEM cost

For either construction, add a candidate only when:
    it conflicts with no already selected candidate
    adding its exact SSIM/window ledgers remains inside every existing global
        and weakest-view budget
    selected count remains below the existing cap

Choose the set with more collapses. On equal count choose the set with the
higher predicted aggregate score, then higher weakest-view score.

Commit only the chosen independent set, render the complete resulting mesh,
and keep the existing full-wave rollback if the exact audit misses any budget.

All T2, T4, large, and giant paths use the original selector byte-for-byte.
*/
