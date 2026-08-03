/*
KALE V005A — SECTION-ISOLATED PROVEN T2 FLANK

Exact control: Kale V002B crop-area, 90.545074, PPPPPPP.

PROBLEM
    A direct composition of Push19A's all-pass T2 flank with Push21B's T3 tail
    failed hidden test 4 even though the branches are mutually exclusive.
    Batches 03/04 confirm that T3 is sensitive to source work and code layout.

ALGORITHM
    replace the T2 call target with a noinline cold wrapper emitted in a
    dedicated T2 text section; do not change any executed T3 operation

    in the isolated wrapper, only when inputVertices <= 25000:
        preserve the original input reference and T2 branch-entry mesh
        execute the exact V002B T2 search
        render the preserved original from six views at 1024
        rebuild original-relative perceptual debt on the T2 result
        inspect the first eight legacy endpoint-weld candidates
        rank by unchanged crop-area disruption minus unchanged debt term
        force one candidate under score cap 0.30 and all hard guards

INTERPRETATION
    The terminal operation is the proven Push19A flank. The experiment is
    whether isolating its code and runtime from the T3 branch permits safe
    composition with V002B. No T3, T4, or large-tier algorithm changes.
*/
