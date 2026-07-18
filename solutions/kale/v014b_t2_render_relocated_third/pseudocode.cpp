/*
KALE V014B — T2 RENDER-OPTIMAL RELOCATED THIRD COLLAPSE

Exact base and branch isolation: V014A pseudocode.

After the unchanged safe T2 independent pair, select one additional sequential
candidate from a freshly rebuilt frontier. For each of the first eight legal
directed edges, search a deterministic coarse-to-fine set of positions on the
open endpoint segment. Evaluate each position with complete-current-scene 1024
normal/depth SSIM and retain the lowest-loss legal interior position.

Choose the globally lowest-loss candidate/position and commit exactly one
collapse under all existing structural, radius, coverage, and gain checks.
No T3 or other-tier call site changes.

This asks whether render-space relocation transfers better than QEM placement
to the official T2 mesh where the endpoint-only third candidate failed.
*/
