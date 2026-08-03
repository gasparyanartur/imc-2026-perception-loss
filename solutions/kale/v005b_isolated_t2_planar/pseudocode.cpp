/*
KALE V005B — ISOLATED POST-T2 PLANAR CONSOLIDATION

Exact control: Kale V002B crop-area, 90.545074, PPPPPPP.

ALGORITHM
    use the same dedicated noinline cold T2 wrapper as V005A
    preserve original input and execute the exact V002B T2 search

    snapshot the resulting mesh
    render its complete six-view 1024 reference metrics
    run the existing topology-safe planar-disk consolidator on the result:
        discover connected near-coplanar face disks
        preserve disk boundary and inherited Hausdorff coverage anchors
        retriangulate each disk with fewer interior vertices
        require a valid closed oriented manifold and positive vertex gain
    render the consolidated mesh against the preserved original at 1024
    commit only if mean, minimum-view, minimum-normal, and minimum-depth SSIM
        are each non-regressive relative to the pre-transaction T2 result
    otherwise restore the snapshot exactly

    rebuild debt and execute the proven one-candidate V005A terminal flank

INTERPRETATION
    QEM can leave redundant vertices inside newly flattened regions. Exact
    planar retriangulation can remove many at once without changing the
    rendered surface; the complete non-regression audit makes this a
    transaction rather than an unguarded ratio push. All work is T2-only and
    section-isolated from T3.
*/
