/*
KALE V006B — T2 INDEPENDENT-TRIPLET TERMINAL FLANK

Exact control: Kale V005A, 90.545792, PPPPPPP.

ALGORITHM
    use the exact V006A one-frontier independent-set algorithm, but admit up
    to three mutually locked endpoint welds rather than two
    keep the same best-first perceptual reorder, score cap, topology checks,
    coverage restoration, and reconstruction transaction

INTERPRETATION
    V006A/V006B bracket how much spatially disjoint work exists on the same
    safe late T2 frontier. This is an equal-byte `1 -> 3` semantic substitution
    inside the isolated helper; no code-size or T3-path change is introduced.
*/
