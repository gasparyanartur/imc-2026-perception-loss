/*
KALE V013A — QEM-RELOCATED SIXTH T3 COLLAPSE

Exact base: canonical all-pass V010A. Preserve its preliminary work, T2 pair,
four weak T3 strategic strikes, and one concentrated 1e-4 strike byte-for-byte.

The inherited strategic operation is an endpoint weld: removed is mapped to
kept and the merged vertex must remain at kept's old position. For the one
additional sixth strike, replace that restriction with an edge-collapse
placement transaction.

Rebuild current-vs-original perceptual debt and the legacy legal directed-edge
frontier. Inspect the same first eight candidates that the retired aggressive
tail used. For every directed edge removed -> kept:

    collect the unique current faces incident to either endpoint
    sum their plane quadrics
    analytically minimize the summed quadric along the closed edge segment
    if the segment-constrained QEM optimum is strictly interior:
        placement options = {QEM optimum, midpoint}
    otherwise:
        placement options = {midpoint}

    for each option:
        rewrite both endpoints to the proposed placement in a temporary mesh
        reject zero-area faces, face inversion, duplicate faces, non-manifold
            topology, or inherited coverage failure
        render before/after with complete current-mesh occlusion context in all
            affected official views
        measure the existing 1024 local normal/depth SSIM loss

    retain the lower-loss legal *interior* placement for this edge

Choose the edge/placement with minimum measured local loss; debt is not a
reward in this safety-oriented sixth strike. Commit exactly one collapse and
rerun all existing reconstruction, manifold, radius, coverage, and vertex-gain
checks. All non-T3 paths remain V010A.

This tests whether the hidden sixth-strike failure is caused by endpoint-only
geometry rather than by the existence of one additional topological collapse.

The first endpoint-optional local attempt selected the old endpoint for every
winner and exactly reproduced the retired unsafe six-strike mesh. Excluding
endpoints is therefore part of the experiment's semantic contract, not a
quality-threshold adjustment.
*/
