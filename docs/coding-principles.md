# Coding Principles

- **No hardcoded constants.** There are no anonymous literal thresholds inside the algorithm body. All thresholds should be defined in parameters structures.
Separate all parameters into two types: 
- *Shared parameters*: Constants and parameters that do not change across mesh size, e.g. camera parameters, mesh I/O format, etc. These should be prefixed with SPARAM_ and defined in a struct `SharedParameters` at the top of the code file.
- *Tiered parameters*: Constants and parameters that change across mesh size. These should be prefixed with TPARAM_ and be defined in a struct `TieredParameters`, which gets defined when tier is set.

- **No time-dependent decisions.** The solution should not depend on the time taken to run, or the order of execution. Phase-decisions should only depend on vertex count. The exception is a global kill-switch, that should be set to terminate the program if it exceeds a maximum time limit (e.g. 20 seconds). The program should terminate gracefully and return the smallest mesh found so far.

- **Thorough documentation.** All parameters should be extremely well-explained with comments. It should be easy to reason about how an algorithm works and how it can be improved. The code should be readable and maintainable, with clear function names, variable names, and comments.

- **Performance is critical.** The solution should be optimized for performance, with a focus on reducing the number of vertices while maintaining a valid closed 2-manifold within the Hausdorff and SSIM constraints. The solution should be able to handle large meshes efficiently. The more time we can save, the more iterations we can run, and the better the final solution will be.

- **Tiered solutions.** Maintain 6 tiers. The tiers are defined by initial mesh size, listed in the table below: 
  
| Tier | Initial Mesh Size |
|------|------------------|
| 1 | <= 5k |
| 2 | <= 25k |
| 3 | <= 40k |
| 4 | <= 50k |
| 5 | <= 400k |
| 6 | <= 1.1M |



