# Current handoff

Historical milestones and prior findings are recorded in
[HISTORY.md](HISTORY.md). This file is the current state and next action.

## Session

- Last updated: 2026-11-07.
- Assistant: Claude.
- Branch: `main`.
- Base commit: `ea54309` (`* MISRA 10.4 remediated, status traceability
  and MC/DC coverage`), pushed at the owner's request. Its CI run
  (`42343375057`): every job concluded success, including
  `cppcheck and MISRA C 2012` (image cppcheck against the new manifest)
  and the coverage job with gcc-14.
- Objective: continue the open list from the previous handoff: classify
  the remaining MC/DC gaps, turn the per-expression 10.4 review into a
  check, read CI.
- Status: completed and verified locally; committed and pushed at the
  owner's standing request ("ne gerekiyorsa yap"). The CI run of that
  commit is the next thing to read.

## Changes

1. **MC/DC classified in all fourteen modules.** 128 missing condition
   outcomes became cases in `sarray` (generator), `smath` (generator),
   `sstring`, `smemory`, `sfilter`, `sring`, `sscale`, `sfixed`, `sdiag`,
   each in a `testConditionCoverage` group or the generated templates.
   The rest are unreachable and listed in `CLAUDE.md`.
2. **Nine mutants** that pass the old suites and die only to the new cases:
   `sarray` M23-M25, `smath` M16, `sstring` M20-M21, `smemory` M9,
   `sscale` M19, `sfilter` M15. 221 in total.
3. **Rule 10.4 by expression.** `tools/misra.sh` runs the addon with
   `--cli` and passes the column; `tools/misra_occurrences.py
   --parse-addon` turns the JSON into `line rule column`, and with
   `--dumps` the checker requires the operator at that column to be
   `==`/`!=` with a `TRUE`/`FALSE` expansion or the module's own enum
   member as an operand. The addon control now goes through the same
   `--cli` path.
4. **CI.** `coverage.sh` prints its table as one `::notice` under GitHub
   Actions. `actions/upload-artifact@v7`: v4 and v5 both drew a Node.js 20
   deprecation warning; v6 and later declare `node24`.

## Decisions and reasons

- The expression test reads only the direct operands of the reported
  operator. All 359 occurrences are `==`/`!=` with such an operand, so a
  stricter rule was not needed and a looser one would admit more.
- Enum members are taken from the module's own header, not a prefix
  pattern, so another module's status cannot satisfy the test.
- One notice for the coverage table rather than one per module: a step
  shows only the first ten annotations of a kind.

## Verification (final tree, this session)

- `bash tools/run_all.sh` with zig clang and with MinGW gcc 14.2: all suite
  lines `0 failed` (4026 module cases + 38 integration), strict warnings 0,
  UBSan clean, apicheck 0, doxcheck 0 (676 functions), tracecheck 0 gaps /
  10 exceptions. Its drift step reports DRIFT only against HEAD for the two
  regenerated test files; regenerating reproduced all four generated files
  byte-for-byte.
- `python tools/mutate.py`: 221 mutants, 201 killed, 20 equivalent, every
  one accounted for. Each of the nine new ones was also built against the
  suite at `ea54309` and passed it.
- `bash tools/misra.sh` (cppcheck 2.21.0): controls pass, 0 findings
  outside deviations, 0 manifest differences, 359 occurrences, all inside
  the rationale by expression. Negative control on a copy of the tree: a
  `( present == TRUE ) && ( driver->counter >= 0 )` line in `sfault` was
  refused at column 51, by expression.
- `bash tools/coverage.sh gcc` with `GITHUB_ACTIONS=true`: table below in
  `CLAUDE.md`, notice line printed. doxygen 1.17.0: 0 warnings.
- CI run `42344832905` of `4bbfa37`: every job success. The MISRA job ran
  the image's cppcheck through `--cli` and the expression check (a parse
  failure would have left 359 stale records and failed it). The coverage
  notice reads `coverage by gcc-14 and gcov-14`, with every module's
  numbers identical to the local MinGW gcc 14.2 run. Both upload steps
  still warned about Node.js 20 under v5, hence v7.

- CI run `42344985604` of `38e2dea` (upload-artifact v7, notice escaping):
  every job success, no annotation but the coverage notice, which now
  shows its percent signs. The Node.js 20 warning is gone.

## Remaining work

1. Target (ARM) execution remains a permanent limit per `CLAUDE.md`.

No other open item is recorded. Candidates if the owner wants more:
flipping the coverage job to blocking at the current numbers, or a
target-side measurement plan for an integrator (not something this
repository can run).
