# World model

This is a working set of hypotheses about the official test environment, not
ground truth.

- Tiers 2 and 3 (tests 3 and 4) are more SSIM sensitive than the others tiers. This can be seen by our tuned threshhold parameters, which are more conservative for these tiers. When we continue removing vertices using QEM, our SSIM eventually drops below the threshhold much sooner than the other tiers. If we can find a way to improve SSIM for these tiers, we can likely lower the threshholds and get better compression.
- Tier 6 (test 7) is sensitive to run-time and is prone to timeouts. This is because it has more vertices. Sometimes running the exact same code will yield much lower results (up to a difference of 0.5 score), because the longer run-time makes our solution enter a different phase at a different timing.