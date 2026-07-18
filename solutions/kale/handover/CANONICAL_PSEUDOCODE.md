# Kale canonical pseudocode

This is the current control algorithm for the kale family. Keep the pseudocode and `v010a_drop_aggressive_final/code.cpp` synchronized.

## Control objective

Minimize output vertices while preserving:

- valid face indices
- positive-area triangular faces
- closed watertight 2-manifold topology
- symmetric Hausdorff within the hidden threshold
- six-view SSIM within the hidden threshold
- output size under the byte limit

## Canonical control shape

The current champion is `v010a_drop_aggressive_final`.

```text
read mesh
build the normal QEM / transaction state

if input vertices <= 10:
    emit unchanged mesh
    stop

if 5k < input vertices <= 50k:
    run the preliminary screened reduction
    if the special early-success condition is met:
        emit that result
        stop

    preserve the branch-entry mesh

    if the mesh is in the medium code-T3 band:
        run the exposure bootstrap
        run the near-identity batch
        run two incremental SSIM batches
        run one incremental SSIM batch at 1024 with overlap allowance 2
        compact
    else:
        run the tier-specific medium bootstrap

    if input vertices <= 25000:
        run the isolated T2 flank
    else:
        run the residual medium transaction
            mode 4 for official test 4 / code T3
            mode 5 for official test 5 / code T4

    if official test 4 / code T3:
        run exactly:
            four weak strategic strikes
            one concentrated strike
        stop

    emit
    stop

otherwise:
    run the large-tier QEM and transactional tail
    emit
```

## T3 control schedule

The safe T3 control is:

```text
four weak strategic strikes:
    refresh 1024 debt
    use prefix 16
    use debt coefficient 1e-5

one concentrated strike:
    refresh debt
    use prefix 8
    use debt coefficient 1e-4

stop
```

Do not add the final aggressive prefix-8 `5e-4` strike. That was the removed change that separated the current champion from the next worse family member.

## Safe T2 control

The safe T2 branch that remains part of the canonical family is the isolated flank built in `v006a_t2_independent_pair` and inherited by `v010a`.

```text
run the existing original-reference search
render original vs current and build perceptual debt
commit the existing independent pair unchanged
rebuild frontier and debt
emit
```

## Recent experimental branches

### V013A

```text
keep the five-strike T3 control
add one additional sixth collapse
for each of the first eight legal directed-edge candidates:
    build incident quadrics
    try QEM optimum on the open segment
    compare against midpoint
    keep the lower-loss legal interior placement
choose the best edge/placement
commit exactly one collapse
```

This failed official test 4.

### V013B

```text
same as V013A, but search perceptual positions on the segment
instead of using only QEM-vs-midpoint placement
```

This also failed official test 4.

### V014A / V014B

These are the current T2-relocation experiments:

```text
after the safe T2 independent pair,
try one additional sequential collapse from a freshly rebuilt frontier
either:
    choose QEM-optimal interior placement
or:
    choose render-optimal interior placement
commit exactly one candidate
```

They are still experimental and should not be treated as control.
