# Skill: Iterate

Use this skill to iterate on a solution in a current solution family. 
Each solution family is a set of related ideas and code changes, e.g. solutions/lemon/v1.cpp. 

Each solution should also maintain a solution/(solution-family)/log.md file that records the changes made, the results, and a post-mortem for each iteration. The goal is to produce a valid submission that improves on the previous best `CompressionRate` while remaining a valid closed 2-manifold within the Hausdorff and SSIM constraints.

## When to use

When the user has approved to iterate on a solution in a current solution family.

## How to use

* **Iterative Solutions:** You are iterating on a solution in a current solution family. Each solution family is a set of related ideas and code changes, e.g. `solutions/lemon/v1.cpp`. The goal is to produce a valid submission that improves on the previous best `CompressionRate` while remaining a valid closed 2-manifold within the Hausdorff and SSIM constraints. See [`skills/submit.md`](skills/submit.md) for how to submit a solution to Kattis and receive a score.

* **Maintain a log of iterations.** Keep a record of the changes made, the results, and the post-mortem in `solutions/(family)/log.md`. This will help you and others understand what was tried, what worked, and what did not.

* **Test every iteration.** After each change, evaluate the solution using the [`skills/evaluate.md`](skills/evaluate.md) skill (which runs `./evaluate.sh`). Only accept a change if the skill reports a **valid** submission that **improves** on the previous best `CompressionRate`. If it is invalid, errors, or regresses, read the logged result in `outputs/`, diagnose, and iterate.

* **Bound the iteration.** Do not loop indefinitely. Make at most **50 attempts** to produce a valid improvement over the previous best. If none of the 50 attempts improves the best `CompressionRate`, stop and write a short **post-mortem** for the user: your hypotheses for why the score did not improve (e.g. which validity gate blocks further reduction, where the simplification plateaus) and suggested next directions.

* **Hypothesis Update.** After each iteration, write a short entry in `solutions/(family)/log.md` that includes your hypotheses for why the score did not improve (e.g. which validity gate blocks further reduction, where the simplification plateaus) and suggested next directions.

* **Post-mortem.** After the iterations are complete, write a short post-mortem for the user: your hypotheses for why the score did not improve (e.g. which validity gate blocks further reduction, where the simplification plateaus) and suggested next directions. Update `docs/solutions.md` with the results of your iteration and any new ideas you have for future work. Update `docs/world-model.md` with any new insights you have gained about the meshes, such as shape, topology, or properties to exploit.

* **Efficient runs.** Because the score is given independently for each test-case, and our code has independent parameters for each test (tiers), you can change parameters in all tiers simultaneously in each run. This means that we can extract maximum information from each run.

* **Parallel runs.** You can run multiple iterations in parallel to speed up the search for an improved solution. Run tests in batches of 3-5, wait for the results, and then analyze the results to determine the next set of parameters to try. See [`skills/submit.md`](skills/submit.md) for how to submit a batch of solutions to Kattis and receive scores for each.

* **Ensure Improvments.** Do not make tiny trivial changes that do not meaningfully affect the score. Each iteration should be a meaningful change that has the potential to improve the `CompressionRate`. Look at the debug information from the local evaluator to infer deeper insights about the solution. If the score is the exact same multiple times in a row, that is a MASSIVE RED FLAG that you are not making meaningful changes.