# Skill: Submit

Submission is an online side effect and requires an explicit user request or
approval for the specific C++ artifact(s).

## One submission

Upload one C++ source with:

```sh
python3 scripts/submit.py --family lemon solutions/lemon/v115.cpp
```

Required input is the family and source file. Defaults are:

- service: `https://imc2-cvmaxxing.arturspace.dev/submit`;
- team secret: `cvmaxxing-95`;
- problem: `simplifygeometry`;
- username: `gasparyanartur`.

Override a default with `--teamsecret`, `--problem`, or `--username`.
The source must be a `.cpp` file. The service response and `SUBMISSION_ID` are
printed to stdout.

## Batch submission and polling

Upload an arbitrary number of immutable C++ sources and wait for all of them:

```sh
python3 scripts/submit_batch.py --family lemon \
  solutions/lemon/v115.cpp solutions/lemon/v116.cpp
```

The batch command:

1. uploads every file and continues if one upload fails;
2. immediately writes every returned ID to a JSON batch file under
   `data/submission-batches/`;
3. polls all returned IDs until each reaches a terminal status;
4. updates the same JSON file with each status response and final result.

For a pending submission, the service-provided `retry_in` or
`retry_in_seconds` value is used as the delay, with 30 seconds added before the
next poll. If the service does not provide a retry interval, the batch waits 30
seconds. Use `--ids-file PATH` to choose the JSON output path or
`--max-wait SECONDS` to impose a timeout; the default is to wait until all
submissions finish.

The batch command does not edit, rebuild, or otherwise modify submitted source
files.
