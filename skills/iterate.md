# Skill: Iterate

Use this skill to iterate on a solution in a current solution family. 
Each solution family is a set of related ideas and code changes, e.g. solutions/lemon/v1.cpp. 

You are iterating on a solution in a current solution family. Each solution family is a set of related ideas and code changes, e.g. `solutions/lemon/v1.cpp`. The goal is to produce a valid Kattis submission that improves on the previous best `CompressionRate` while remaining a valid closed 2-manifold within the Hausdorff and SSIM constraints. See [`skills/submit.md`](skills/submit.md) for how to submit a solution to Kattis and receive a score. The final goal is a Kattis score of 95.

Each solution should also maintain a solution/(solution-family)/log.md file that records the changes made, the results, and a post-mortem for each iteration. The goal is to produce a valid submission that improves on the previous best `CompressionRate` while remaining a valid closed 2-manifold within the Hausdorff and SSIM constraints.

## When to use

When the user has approved to iterate on a solution in a current solution family.

## How to use

**Principled theory-driven iteration:** One family of solutions is tied to an overarching solution-theory outlined in `docs/solutions.md`.

**Iterative Workflow:** The iterative workflow is as follows:

1. Before iterating, state a hypothesis for increasing `CompressionRate`, then propose a batch of **4–6** distinct, meaningful code improvements to test against it (for example, parameter tuning or a new feature).
2. For each candidate in the batch, define a meaningful change to the code and implement it.
3. Evaluate each candidate locally using `skills/evaluate.md` and record the results. If the score has not improved, make some small changes to the code and re-evaluate until the score improves. Do this until each candidate in the batch is an improvement over the previous iteration.
4. Submit the batch of candidates to Kattis using `skills/submit.md` and record the results.
5. Update the log with the results of the batch and any new insights gained.
6. Go back to step 1 and repeat until the score is 95 or you have made 50 attempts without improvement.

**Maintain a log of iterations.** Keep a record of the changes made, the results, and the post-mortem in `solutions/(family)/log.md`. This will help you and others understand what was tried, what worked, and what did not.

**Plan a batch.** Before iterating, state a hypothesis for increasing `CompressionRate`, then propose a batch of **4–6** distinct, meaningful code improvements to test against it (for example, parameter tuning or a new feature).

**Evaluate locally.** Evaluate every candidate in the planned batch **sequentially**, after its meaningful change with `skills/evaluate.md` using the local evaluator diagnostically to verify that it works and inspect validity, SSIM, Hausdorff, and compression results. Use the diagnostics to explain score differences and refine the next hypothesis; do not treat local results as the ranking oracle.

**Submit for ground truth.** After every candidate in the batch has been locally evaluated, submit the immutable batch through the submission script as described in `skills/submit.md`. Kattis evaluation is the official source of acceptance and ranking score; local evaluation is a diagnostic approximation that helps explain and predict those results.

**Continuously improve the local evaluator.** Whenever Kattis and local behavior differ, investigate and improve the local evaluator to extract more information about the meshes and score changes. Record the parity change and the new diagnostic insight so future iterations use a more informative local signal.

**Bound the iteration.** Do not loop indefinitely. Make at most **50 attempts** to produce a valid improvement over the previous best. If none of the 50 attempts improves the best `CompressionRate`, stop.

**Hypothesis Update.** After each iteration, write a short entry in `solutions/(family)/log.md` that includes your hypotheses for why the score did not improve (e.g. which validity gate blocks further reduction, where the simplification plateaus) and suggested next directions.

**Post-mortem.** After the iterations are complete, write a short post-mortem for the user: your hypotheses for why the score did not improve (e.g. which validity gate blocks further reduction, where the simplification plateaus) and suggested next directions. Update `docs/solutions.md` with the results of your iteration and any new ideas you have for future work. Update `docs/world-model.md` with any new insights you have gained about the meshes, such as shape, topology, or properties to exploit.

**Efficient runs.** Because the score is given independently for each test-case, and our code has independent parameters for each test (tiers), you can change parameters in all tiers simultaneously in each run. This means that we can extract maximum information from each run.

**Batch submissions.** Submit the **4–6** immutable candidates from the sequentially evaluated batch together. Wait for the whole batch's results, then use local diagnostics to explain score differences and determine the next changes. See [`skills/submit.md`](submit.md) for batch submission.

**Ensure Improvments.** Do not make tiny trivial changes that do not meaningfully affect the score. Each iteration should be a meaningful change that has the potential to improve the `CompressionRate`. Look at the debug information from the local evaluator to infer deeper insights about the solution. If the score is the exact same multiple times in a row, that is a MASSIVE RED FLAG that you are not making meaningful changes.
