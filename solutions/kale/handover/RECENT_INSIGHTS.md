# Recent insights

## Stable conclusions

- `v010a_drop_aggressive_final` is the current canonical control and official champion at `90.546038`.
- `v012a_exact_planar_surface_equivalence` and `v012b_surface_coverage_anchors` are all-pass and tie the same score, but they did not improve the champion.
- `v013a_qem_relocated_sixth` and `v013b_render_relocated_sixth` both failed official test 4. The hidden medium cliff is sensitive to sixth-strike placement changes.
- Dead code and source layout matter. Even unreachable branches can move the medium hidden behavior.
- The evaluator does not need the million-vertex stress case to be informative; the useful discriminator is the 30k-40k medium range.

## Practical rules learned

- Keep pseudocode and code paired.
- Keep every candidate under the upload size limit by minifying only if necessary.
- Evaluate exactly one local diagnostic per immutable candidate before upload.
- Submit in batches of 2 only.
- Prefer changing the algorithmic lever, not just a threshold.

## Current hypothesis

The remaining gain is probably not another small T3 score tweak. The safer next direction is likely a structural change in the medium band, especially around test 3 / test 4 behavior, while preserving the exact current control path for the existing all-pass branch.

## Latest branch status

- `v014a_t2_qem_relocated_third`: code exists; local run was started on the 19,888-vertex T2 fixture.
- `v014b_t2_render_relocated_third`: code exists; should be paired with `v014a` before any submission.

Treat the T2-relocated third collapse as experimental until the pair is fully evaluated and the results are recorded in `solutions/kale/jobs.md`.
