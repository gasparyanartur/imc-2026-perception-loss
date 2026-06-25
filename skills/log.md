# Skill: log

Use this skill to record the reasoning behind a change — the thoughts,
hypotheses, plans, and measured results that belong with the current commit —
in a persistent engineering log.

## When to use

- **Before** a change: jot the hypothesis and the plan you are about to try.
- **After** an evaluation: paste the key numbers (compression, pass/fail,
  solver runtime) and what you concluded.
- At any milestone, decision, or dead end you want the next iteration (or the
  user) to be able to read back.

The log is the narrative companion to `outputs/` (raw evaluator logs) and the
git history (the code itself). It answers *why*, not just *what*.

## How to run

From the repository root:

```sh
./log.sh -t "Title" "One or more sentences describing the thought or plan."
```

The body can also come from stdin, which is handy for capturing command output:

```sh
# free-form note from a heredoc
./log.sh -t "Roadmap" <<'EOF'
Hypothesis: the solver times out at grader scale (~1.1M verts) ...
Plan: add an internal wall-clock budget so it always emits a valid mesh.
EOF

# capture an evaluation result verbatim
./evaluate.sh 2>&1 | ./log.sh -t "evaluate.sh: iter2 vs iter1"
```

Each call appends an entry to `log/<YYYY-MM-DD>.txt` (one file per day). Every
entry is stamped with the time and the current branch + short commit SHA, so a
note is always tied to the state of the tree it describes:

```
## 2026-06-25T23:40:12+0200  [artur-dev @ 84b01b1 +dirty]
### Roadmap
<body>
```

`+dirty` means there were uncommitted changes when the entry was written.

## Configuration

| Variable  | Default | Meaning                 |
| --------- | ------- | ----------------------- |
| `LOG_DIR` | `log`   | directory for log files |

## Conventions

- Keep entries short and factual; prefer numbers over adjectives.
- Write one entry per meaningful step (a hypothesis, an experiment, a result),
  not one giant entry at the end.
- The `log/` directory **is** committed — it is part of the project record, so
  do not delete past entries; append new ones.
