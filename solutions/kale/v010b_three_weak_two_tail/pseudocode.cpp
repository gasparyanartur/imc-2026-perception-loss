/*
KALE V010B — THREE WEAK STRIKES PLUS BOTH CONCENTRATED TAILS

Exact base: V006A.

For T3, preserve the complete ordinary setup and run:
    three weak prefix-16 strikes at debt coefficient 1e-5
    one prefix-8 concentrated strike at coefficient 1e-4
    one prefix-8 aggressive strike at coefficient 5e-4

Each strike refreshes complete 1024 debt as before. All guards and other tiers
remain unchanged. This keeps the debt-concentrating tail but removes one early
low-debt weld, matching the previously all-pass Push22B schedule shape.
*/
