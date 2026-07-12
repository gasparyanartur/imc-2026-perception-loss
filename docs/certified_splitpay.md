# Certified Split/Pay Redistribution

## Status and parent

The sole parent is `v46_isolation_control.cpp`, SHA-256
`9a5246f196a0f9e2ed7a9578e28fd623513059fd47164e15ebb05f980f1a1d09`.

This branch was created after the v51-v55 cohort family failed official T4 and
did not improve official T3 or T5. It does not call or inherit the cohort code.

The active candidates are:

- `v62_splitpay_v46.cpp`: stronger certified redistribution;
- `v63_splitpay_v46safe.cpp`: higher perceptual-margin version.

Both preserve v46 byte-for-byte on T1, T2, T4, T6, and T7. Only official T3
and T5 can enter the new transaction.

## Central idea

Delete-only QEM decides where the remaining vertices live while it is also
deciding how many vertices remain. If it spends too few vertices on a visible
ridge or silhouette, continuing the same queue cannot repair that allocation.

Split/pay decouples those decisions:

1. Start from the exact v46 result (M_b).
2. Oversimplify (M_b) to create a pool of removed vertices.
3. Measure the candidate's normal/depth residual against the original mesh.
4. Spend part of the removed-vertex pool by splitting the highest-residual
   faces and projecting the new vertices onto the original surface.
5. Commit only when the repaired mesh is still smaller than (M_b), passes an
   explicit bidirectional geometric envelope, remains a closed oriented
   manifold, and passes a native 1024 original-relative render check.

This is a vertex-budget redistribution transaction, not a lower QEM keep ratio.
Vertices removed from low-impact regions pay for new vertices in under-resolved
high-impact regions.

## Residual allocation

At a diagnostic raster resolution, every visible candidate pixel is attributed
to its winning candidate face. For candidate face (f), the residual is

\[
E_f = \frac{1}{N_f+4}\sum_{p\mapsto f}
\left[
3\lVert (N_0(p)-N_c(p))/255\rVert^2
+1.5\min(1,|D_0(p)-D_c(p)|/0.02)
\right].
\]

A foreground mismatch receives a fixed large penalty. Faces are ordered by
(E_f). To repair a selected face ((a,b,c)), the solver starts from its
centroid, finds the closest sampled original triangle, projects the centroid
onto that triangle, inserts the projected point (z), and replaces the face by
((a,b,z),(b,c,z),(c,a,z)). Every new triangle must retain the original face
orientation and positive area.

If oversimplification removes (r) vertices, repair budgets of approximately
(r/4), (r/2), and (3r/4) are tested. Thus every accepted transaction has
a strictly positive net reduction.

## Explicit geometric certificate

The v46 cluster radius is useful during its original trajectory, but resetting
another QEM trajectory loses the history represented by that radius. The new
branch therefore validates the complete transaction directly.

Let (H=0.049\,\mathrm{Diagonal}). It checks both

\[
\max_{x\in V(M_0)} d(x,M_c) \le H,
\qquad
\max_{y\in V(M_c)} d(y,M_0) \le H.
\]

The decision is accelerated with a uniform 20x20x20 grid. Every triangle is
inserted into all cells overlapped by its AABB expanded by (H). Therefore, if
a query vertex has any triangle within (H), that triangle is present in the
query vertex's cell. Exact point-to-triangle distance is then evaluated for the
cell's candidates. The process is run in both directions.

This is materially stronger than accepting a local proxy or a newly reset
cluster radius, and the `0.049` factor leaves explicit room below the stated
`0.05` bound.

## Perceptual and topology gates

The complete repaired mesh must:

- be a closed consistently oriented triangular manifold;
- have fewer vertices than the exact v46 fallback;
- pass the bidirectional envelope above;
- retain depth and normal safety at the diagnostic checkpoint;
- achieve original-relative aggregate SSIM at native 1024 resolution.

`v62` uses a final native floor of `0.915`. `v63` uses `0.935` and a stricter
normal checkpoint. Any failure restores the exact v46 result.

## Tier isolation

Using the seven-tier convention:

| Tier | Input cap | Behavior |
|---|---:|---|
| T1 | 10 | exact v46/sample passthrough |
| T2 | 5,000 | byte-identical to v46 |
| T3 | 25,000 | split/pay enabled |
| T4 | 40,000 | byte-identical to v46 |
| T5 | 50,000 | split/pay enabled |
| T6 | 400,000 | byte-identical to v46 |
| T7 | 1.1M | byte-identical to v46 |

The source is a token-preserving minification of v46 plus the isolated
transaction. The minified v46 control was compiled and produced byte-identical
T3/T4/T5 diagnostics before the transaction was inserted.

## Diverse local evidence

Native 1024 results:

| Fixture | v46 vertices / SSIM | v62 | v63 |
|---|---:|---:|---:|
| T3 ellipsoid, 24,877 V | `7071 / .991458` | `5479 / .986363` | `5479 / .986363` |
| T3 bumpy, 24,877 V | `7321 / .958749` | `5673 / .947699` | `6222 / .952801` |
| T3 wavy, 24,877 V | `7463 / .866371` | fallback | fallback |
| T4 ellipsoid, 39,752 V | `5764 / .985335` | byte-identical | byte-identical |
| T4 bumpy, 39,752 V | `5764 / .941270` | byte-identical | byte-identical |
| T5 peanut, 48,002 V | `3600 / .978971` | `2790 / .972184` | `3060 / .975184` |
| T5 bumpy, 48,002 V | `3792 / .924280` | fallback | fallback |
| ABC CAD 00010009, 9,636 V | `2601 / .998617` | `2015 / .996853` | not required |
| ABC CAD 00011084, 8,071 V | `2179 / .997401` | `1688 / .991608` | not required |

The independent evaluator marked the checked changed outputs valid. Examples:

- T3 ellipsoid v62: Hausdorff `0.036282 <= 0.128332`;
- T3 bumpy v63: Hausdorff `0.085280 <= 0.181716`;
- T5 peanut v63: Hausdorff `0.056892 <= 0.146540`.

## Official-test order

1. `v63_splitpay_v46safe.cpp` -- safer transfer test, changes only T3/T5.
2. `v62_splitpay_v46.cpp` -- stronger version if v63 passes.

No T4 placement/continuation variant should be mixed into these submissions.
