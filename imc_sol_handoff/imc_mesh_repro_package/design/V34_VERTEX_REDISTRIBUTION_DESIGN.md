# v34 concept: perceptual vertex-budget redistribution

## Why this is different

The existing solver family is monotone: vertices are collapsed, deleted, or moved, but never reintroduced. Once an early collapse allocates too few triangles to a silhouette or high-normal-variation region, later passes cannot repair that allocation without undoing the whole trajectory.

v34 should use a budget-neutral remeshing transaction:

- split high-error triangles or edges;
- collapse multiple low-impact vertices elsewhere;
- require a net vertex reduction;
- compare the full transaction against the original six-view rendering.

## State

Start from the exact v23 candidate and retain it as fallback.

For the first prototype, target only T2. All other tiers must run a deterministic, isolated v23 path.

## Residual map

Render original and v23 at 512 resolution. For each candidate face, accumulate:

- foreground mismatch;
- normal disagreement;
- depth disagreement;
- silhouette-window overlap.

The priority should be normal residual first, since all recent experiments were limited primarily by normal-map loss.

## Split candidates

Candidate A: longest-edge split.

1. Insert the edge midpoint.
2. Project it onto the closest original triangle associated with the residual pixels.
3. Replace the two incident triangles with four.

Candidate B: face centroid split.

1. Insert the projected centroid.
2. Replace one face with three.

Reject splits that create short edges, poor angles, duplicate triangles, or orientation changes.

## Collapse payment candidates

Collect v23-valid QEM collapses from faces with:

- low or zero visible pixel ownership;
- low silhouette-window participation;
- low predicted normal rotation;
- large distance from split regions;
- substantial remaining collapse-radius margin.

A transaction with `s` new vertices must remove at least `s + g`, where `g >= 1` is the desired net gain.

## Atomic transaction

1. Snapshot v23.
2. Apply 1–4 splits in high-error, non-overlapping regions.
3. Rebuild local connectivity and quadrics.
4. Apply enough independent low-impact collapses to pay for the splits plus net gain.
5. Perform one constrained tangential relocation pass around split regions.
6. Validate the entire candidate against the original mesh.
7. Commit only if the candidate has fewer vertices and does not reduce the trusted perceptual margin.

## Search

Use a tiny beam of at most four transactions:

- silhouette repair;
- normal-interior repair;
- depth-discontinuity repair;
- mixed repair.

Do not optimize many scalar thresholds before the operation proves useful across the multi-shape benchmark.

## Determinism requirement

`CollapseCandidate::operator<` must include stable tie-breakers after cost:

1. quantized cost;
2. minimum endpoint;
3. maximum endpoint;
4. absorbed endpoint;
5. kept endpoint.

The original input tier must be stored once and never inferred from the current simplified vertex count.

## First promotion target

A useful first prototype should meet all of these locally:

- improve vertex count on sphere, a concave form, a sharp form, and torus/capsule;
- no failure on thin box and high-frequency form;
- no change in T3–T7 outputs;
- at least 0.25% aggregate vertex reduction over v23 across the accepted cases;
- no material minimum-normal regression.
