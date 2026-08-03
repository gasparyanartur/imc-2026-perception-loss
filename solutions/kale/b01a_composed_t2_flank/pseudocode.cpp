/*
KALE B01A — COMPOSED T2 FLANK

CONTROL
    Exact source: sendit/fredrik_v1110_push21b_aggressive_pair_substituted_seventh_submission.cpp
    Official result: 90.545074, PPPPPPP.

HYPOTHESIS
    The one-candidate T2 terminal flank proven by Push 19A and the T3 strategic
    schedule proven by Push 21B operate on mutually exclusive input-size
    branches. Composing them should preserve every existing result and add the
    T2 vertex gain to the T3 gain.

TOP LEVEL
    read original mesh
    start 20.2 second timer

    if vertices <= 10:
        emit unchanged

    if 5,000 < original vertices <= 50,000:
        try the exact preliminary planar-patch reduction from Push 21B
        preserve its exact early-return and rollback rules

        save branch-entry mesh as mediumOriginal

        if current vertices are in code-T3 range (25k, 45k]:
            run exact Push 21B exposure bootstrap
            run exact near-identity and incremental SSIM batches
            compact
        else:
            run exact Push 21B tier-specific medium bootstrap

        if official test 3 / code T2 (original vertices <= 25k):
            run exact original-reference 1024 search

            // Proven Push 19A terminal flank; this is the only B01A change.
            rerender current mesh against the original input at 1024
            rebuild normalized per-vertex perceptual debt
            rebuild the topology-safe directed endpoint-weld frontier
            inspect the first 8 legacy-ranked candidates with legacy cap 0.30
            for each inspected candidate:
                compute full-context six-view local normal/depth SSIM loss
                compute mean endpoint-region perceptual debt
                rank = localLoss - 1e-5 * meanDebt
            force exactly the minimum-rank candidate if structurally valid
            retain all duplicate-face, manifold, area, orientation,
            Hausdorff/radius, coverage-witness, and net-vertex-gain guards

        else:
            run exact Push 21B residual mode 4 or mode 5 transaction

        if official test 4 / code T3 (25k < original vertices <= 45k):
            rebuild exact QEM/connectivity state
            run pair-disk cleanup with exact Push 21B settings
            run ordinary endpoint-weld pass
            run one rebuilt endpoint weld with maxAccepted=1 and cap=0.20
            cache six original-input renders at 1024

            repeat 4 weak strategic strikes:
                rerender current-vs-original debt at 1024
                rebuild endpoint-weld frontier
                inspect first 16 legacy candidates
                rank = localLoss - 1e-5 * meanDebt
                force one structurally valid minimum-rank candidate

            rerender debt and run one concentrated strike:
                inspect first 8 candidates
                rank = localLoss - 1e-4 * meanDebt

            rerender debt and run one aggressive substituted strike:
                inspect first 8 candidates
                rank = localLoss - 5e-4 * meanDebt

        emit medium result

    otherwise:
        run the exact Push 21B large/giant QEM, transactional, timing,
        pair-disk, hidden-edge, camera, and star schedules byte-for-byte
        emit result

HARD INVARIANTS
    output indices valid; triangles distinct and positive area
    every undirected edge has incidence two with opposite orientation
    inherited-radius and 5%-diagonal coverage witnesses retained
    a phase commits only when the final vertex count decreases
    no perceptual rollback is added to forced frontier probes
*/
