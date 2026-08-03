/*
KALE V012A — EXACT PLANAR SURFACE EQUIVALENCE

Exact base: V010A, including its all-pass T2 pair and five-strike T3 schedule.

The existing preliminary planar transaction treats Hausdorff coverage as
vertex-to-vertex coverage. That is unnecessarily restrictive for a planar
disk: Hausdorff distance is measured to the replacement surface, not only to
replacement vertices.

For each connected candidate patch, preserve all existing requirements:
    one consistently oriented boundary loop
    disk Euler characteristic
    no interior vertex incident to a face outside the patch
    successful boundary-polygon ear clipping
    complete manifold/orientation validation after replacement

Add a strict surface-equivalence certificate:
    choose the area-weighted patch support plane
    let scale = max(1, input bounding-box diagonal)
    require every patch vertex's signed plane residual to be within a
        roundoff-sized tolerance derived from scale and machine epsilon
    require every face normal to agree with the support-plane normal to the
        corresponding roundoff-sized angular tolerance

If certified, triangulate only the unchanged boundary ring and retain no
interior anchors. The old and new patches then represent the same planar
polygonal surface: their boundary, positions, orientation, and continuous
depth/normal fields are identical; only the internal triangulation changes.

If the strict certificate fails, use V010A's legacy radius/vertex coverage
anchor selection unchanged. All downstream simplification is unchanged.
*/
