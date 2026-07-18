/*
KALE V013B — RENDER-OPTIMAL RELOCATED SIXTH T3 COLLAPSE

Exact base and unchanged five-strike schedule: V013A pseudocode.

For the sixth strike, optimize placement in perceptual rather than quadric
space. For each of the first eight legacy legal directed edges:

    parameterize the open segment between kept and removed
    evaluate a deterministic coarse-to-fine set of interior positions using
        the same complete-scene 1024 affected-view normal/depth SSIM loss
    discard any position failing face-area, orientation, topology, duplicate,
        inherited-radius, or coverage constraints
    retain the legal position with minimum measured loss for that edge

Choose the edge and position with minimum measured local loss across the
frontier and commit exactly one collapse. Do not apply perceptual debt reward:
the extra strike is selected for least incremental rendered damage. Preserve
V010A everywhere outside this single placement-aware sixth strike.

This orthogonally asks whether the QEM optimum misses a screen-space placement
that makes the extra topological reduction safe under the official renderer.
Endpoint positions are deliberately excluded because the endpoint-optional
attempt exactly reproduced the already-rejected six-strike control.
*/
