/*
KALE V012B — RETRIANGULATED-SURFACE COVERAGE ANCHORS

Exact base: V010A. Preserve its all-pass terminal schedules and every planar
patch topology, orientation, and transaction rollback guard.

Replace only the preliminary planar transaction's anchor-selection model.
The legacy rule asks whether a removed interior vertex is close to a retained
vertex. The required geometric question is whether it is close to the retained
triangulated surface.

For each accepted single-boundary disk patch:
    anchors = empty
    triangulate the boundary polygon

    repeat:
        for every original interior vertex x not in anchors:
            coverage_error(x) = radius[x]
                              + minimum 3D point-to-triangle distance from x
                                to the current replacement triangulation

        if every coverage_error <= existing planar Hausdorff cap:
            accept the current anchor set

        otherwise sort all uncovered vertices by decreasing excess, promote
        that batch to anchors, and retriangulate boundary plus anchors

        if retriangulation fails, or all interior vertices become anchors:
            reject this patch and leave it unchanged

Batch promotion makes the loop practical even for a 30–40k-vertex planar grid.
After convergence, recompute coverage against the final triangulation and
accept only if every removed interior vertex passes. Retained anchors represent
their original points exactly. This is a geometric coverage certificate in
surface space; it does not loosen the existing cap.
The patch still passes the complete mesh manifold/orientation validator before
commit. Every non-planar stage and all T2/T3 terminal behavior are unchanged.
*/
