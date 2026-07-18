# Kale solution family

Every candidate is developed in `pseudocode.cpp` before `code.cpp`. The final
meaningfully named `.cpp` file in each directory is the immutable compact
upload artifact.

| Version | Pseudocode | Readable code | Upload artifact | Status |
|---|---|---|---|---|
| B01A composed T2 flank | [`b01a_composed_t2_flank/pseudocode.cpp`](b01a_composed_t2_flank/pseudocode.cpp) | [`b01a_composed_t2_flank/code.cpp`](b01a_composed_t2_flank/code.cpp) | `b01a_composed_t2_flank/submission.cpp` | 76.208348, `PPPFPPP` |
| B01B window mass | [`b01b_window_mass_ranking/pseudocode.cpp`](b01b_window_mass_ranking/pseudocode.cpp) | [`b01b_window_mass_ranking/code.cpp`](b01b_window_mass_ranking/code.cpp) | `b01b_window_mass_ranking/submission.cpp` | 76.208348, `PPPFPPP` |
| V002A crop side | [`v002a_crop_side/pseudocode.cpp`](v002a_crop_side/pseudocode.cpp) | [`v002a_crop_side/code.cpp`](v002a_crop_side/code.cpp) | `v002a_crop_side/kale-v002a-crop-side.cpp` | 76.208963, `PPPFPPP` |
| V002B crop area | [`v002b_crop_area/pseudocode.cpp`](v002b_crop_area/pseudocode.cpp) | [`v002b_crop_area/code.cpp`](v002b_crop_area/code.cpp) | `v002b_crop_area/kale-v002b-crop-area.cpp` | 90.545074, `PPPPPPP`; safe control |
| V003A original-after | [`v003a_original_after/pseudocode.cpp`](v003a_original_after/pseudocode.cpp) | `v003a_original_after/code.cpp` | `v003a_original_after/kale-v003a-original-after.cpp` | pseudocode complete |
| V003B original-delta | [`v003b_original_delta/pseudocode.cpp`](v003b_original_delta/pseudocode.cpp) | `v003b_original_delta/code.cpp` | `v003b_original_delta/kale-v003b-original-delta.cpp` | pseudocode complete |
| V010A five-strike control | [`v010a_drop_aggressive_final/pseudocode.cpp`](v010a_drop_aggressive_final/pseudocode.cpp) | [`v010a_drop_aggressive_final/code.cpp`](v010a_drop_aggressive_final/code.cpp) | `v010a_drop_aggressive_final/kale-v010a-drop-aggressive-final.cpp` | **90.546038**, `PPPPPPP`; canonical |
| V012A exact planar surface | [`v012a_exact_planar_surface_equivalence/pseudocode.cpp`](v012a_exact_planar_surface_equivalence/pseudocode.cpp) | [`v012a_exact_planar_surface_equivalence/code.cpp`](v012a_exact_planar_surface_equivalence/code.cpp) | `v012a_exact_planar_surface_equivalence/kale-v012a-exact-planar-surface-equivalence.cpp` | 90.546038, `PPPPPPP` |
| V012B surface coverage | [`v012b_surface_coverage_anchors/pseudocode.cpp`](v012b_surface_coverage_anchors/pseudocode.cpp) | [`v012b_surface_coverage_anchors/code.cpp`](v012b_surface_coverage_anchors/code.cpp) | `v012b_surface_coverage_anchors/kale-v012b-surface-coverage-anchors.cpp` | 90.546038, `PPPPPPP` |
| V013A QEM relocated sixth | [`v013a_qem_relocated_sixth/pseudocode.cpp`](v013a_qem_relocated_sixth/pseudocode.cpp) | [`v013a_qem_relocated_sixth/code.cpp`](v013a_qem_relocated_sixth/code.cpp) | `v013a_qem_relocated_sixth/kale-v013a-qem-relocated-sixth.cpp` | 76.209067, `PPPFPPP` |
| V013B render relocated sixth | [`v013b_render_relocated_sixth/pseudocode.cpp`](v013b_render_relocated_sixth/pseudocode.cpp) | [`v013b_render_relocated_sixth/code.cpp`](v013b_render_relocated_sixth/code.cpp) | `v013b_render_relocated_sixth/kale-v013b-render-relocated-sixth.cpp` | 76.210400, `PPPFPPP` |

Family records:

- [`log.md`](log.md) — hypotheses, diagnostics, official results, post-mortems.
- [`jobs.md`](jobs.md) — immutable filenames, submission IDs, and terminal states.
