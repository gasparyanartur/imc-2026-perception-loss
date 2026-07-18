# Instructions for the next soldier

## Mission

Read beyond **92 score points** without losing the proven six passing cases or destroying official test 4. Treat solution A as canonical control and solution B as an alternative branch.

## 1. First actions in a new session

1. Retrieve the exact  readable source, compact submission and pseudocode from the File Library.
2. Compile the unmodified compact source with the competition compiler settings.
3. Record its byte size; it must remain under the submission limit used by this project (approximately 100 KB source).
4. Diff any proposed variant against the exact control before compiling.
5. Update pseudocode before or together with code. The code and pseudocode must describe the same algorithm.

Never begin from a remembered or reconstructed version.

## 2. Experimental doctrine

### One lever per submission

A useful experiment changes exactly one of:

```text
number of strikes
probe prefix size
perceptual-debt reward
local SSIM scoring formula
candidate eligibility
phase placement/order
one specific official tier
```

Do not simultaneously refactor shared helpers, rename large sections and alter behavior.

### Use two submissions intelligently

When two submissions are available, use them as a bracket or an orthogonal pair:

```text
A = conservative interpolation from the current control
B = aggressive or structurally different probe
```

This tests whether the different sixth candidates expose different safe seventh frontiers.

A second good pair is:

Do not simply increase the number of candidates and take the same ranking minimum; that path already failed at the ordinary sixth strike.

## 3. Fallback doctrine

Distinguish two kinds of fallback.

### Hard structural rejection — keep

Never remove checks that prevent:

```text
invalid indices
duplicate or degenerate faces
non-manifold edges
orientation inversion
Hausdorff/radius violation
coverage padding erasing the claimed vertex gain
```

These are not feedback-obscuring fallbacks. They prevent invalid output.

### Perceptual or score rollback — disable during frontier probes

When the purpose is to learn whether a proposed candidate is safe, do not silently restore the control based on an internal approximate SSIM gate. Force the structurally valid change and let the judge answer.

Once the frontier is understood, a final production version may use a gate—but only if the gate is calibrated against known official outcomes.

## 6. Runtime discipline

The global budget is approximately 20.2 seconds. Local synthetic runtime is not the official runtime. Every additional 1024 render must be justified.

To reduce cost without changing candidate semantics:

```text
cache original reference images
reuse current render to build both debt and foreground-window counts
limit expensive local SSIM to a small legacy-ranked prefix
reject huge projected patches before rasterization
cache face-to-view projected bounds for the current strike
```

Do not add an entry-time gate that silently skips the new experiment; that produces no judge feedback.

## 7. Validation before submission

For every variant:

```text
compile readable and compact sources
verify they emit byte-identical output on probes
verify compact source size
run T2 and T3 synthetic probes
validate:
    indices
    positive triangle area
    duplicate faces
    every edge incidence == 2
    output vertex count <= input
```

Record the output vertex count and runtime, but do not infer official SSIM from synthetic geometry.

## 8. Reporting format after judge feedback

Record every submission as:

```text
name:
exact control:
single changed lever:
official score:
pass/fail pattern:
which test changed:
internal probe vertex count:
interpretation:
next discriminating experiment:
```

Do not write “same score means fallback” unless the output is proven byte-identical or the changed branch is proven unreachable.

## 9. Standing orders

- Preserve exact controls.
- Keep pseudocode synchronized.
- Prefer algorithmic candidate-quality improvements over larger caps.
- Never trust a proxy merely because it improves a synthetic sphere or torus.
- The judge is the final experiment.
- When a forced probe fails, extract the boundary it reveals; do not hide it behind another fallback.
