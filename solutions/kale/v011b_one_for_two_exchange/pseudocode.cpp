/*
KALE V011B — ONE-FOR-TWO PERCEPTUAL SET EXCHANGE

Exact base and unchanged constraints: V011A pseudocode.

After constructing the better of the damage-first and conflict-aware T3 sets,
perform a bounded local augmentation in selection space before touching mesh
topology:

    for each selected candidate S:
        consider rejected candidates A and B such that:
            A and B are mutually compatible
            each conflicts with no selected candidate except S
            replacing S by A+B stays under the unchanged exact aggregate and
                weakest-view SSIM ledgers
            the existing wave cap is not exceeded

        rank feasible exchanges by:
            greatest vertex-count gain (always +1 for a 1-for-2 exchange)
            then highest predicted aggregate SSIM
            then highest predicted weakest-view SSIM

    apply the best exchange and stop after one augmentation

Commit the augmented independent set. Render the complete resulting mesh and
use the unchanged full-wave audit/rollback. Thus the experiment asks whether a
single greedy conflict is blocking two perceptually affordable collapses; it
does not loosen quality, topology, overlap, Hausdorff, runtime, or strike
budgets.

All non-T3 paths and the five terminal strikes remain unchanged.
*/
