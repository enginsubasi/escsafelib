# Current handoff

Historical milestones and prior findings are recorded in
[HISTORY.md](HISTORY.md). This file is the current state and next action.

## Session

- Last updated: 2026-11-07.
- Assistant: Claude.
- Branch: `main`.
- Base commit: `54e59d7` (`* safety notes and occurrence-scoped MISRA checks`).
- Objective: review the Codex session, then carry out the open work list
  agreed with the owner in chat (items 1-7 below).
- Status: completed and verified; committed and pushed at the owner's
  request in one commit on top of `54e59d7`. The first CI run of it is the
  next thing to read.

## Changes

1. **Mutant anchors repaired.** `54e59d7` had broken five (`sfault` M11,
   M15, M16, `sfilter` M10, `svote` M16); 25 more were updated after the 10.4
   remediation.
2. **Stale records.** Empty `docs/ai/DEVELOPMENT_PLAN.md` deleted (`CLAUDE.md`
   forbids plan documents). `HISTORY.md`, `docs/standards/MISRA_C_2012.md`
   and the `sdiag` mutant paragraph in `CLAUDE.md` corrected.
3. **Rule 10.4 manifest keyed without line numbers**: `module, function,
   sha256(line), count`. `tools/misra_occurrences.py` rewritten with a coarse
   rationale guard (`TRUE`, `FALSE` or an `S?_` enum member on the line);
   its `--self-test` runs as a control in `tools/misra.sh` and refuses to run
   under `python -O`.
4. **Rule 10.4 remediated**: 621 to 359. Unsigned operands meet only
   `u`-suffixed constants in all fourteen modules, both generators and
   `template/src/generic.c`; every banner's deviation paragraph narrowed to
   enum and `TRUE`/`FALSE` comparisons. New safety rule in `CLAUDE.md`.
5. **Traceability**: `tools/tracecheck.py` plus `tools/trace_exceptions.tsv`
   (10 reasoned exceptions, stale ones fail). Blocking in CI and in
   `run_all.sh`. Its 106 first-run gaps closed by new cases in nine suites,
   the `sfixedFloor`/`sfixedRound` contract fix (no `SX_UNDERFLOW` is
   possible; `test/harness/sfixed_rounding.c` sweeps all 2^32 inputs with
   exact counts), and sentinel checks that refused calls leave every output
   alone.
6. **MC/DC** column in `tools/coverage.sh` (gcc 14+, matching `gcov-NN`);
   CI coverage job installs gcc-14. MC/DC gaps of `sbits`, `svote`,
   `sfault`, `sstate` classified; six real cases added.
7. **CI**: every job on `ubuntu-24.04`; `actions/checkout@v5`.
8. **Mutants**: seven new (`sstring` M19, `sfilter` M14, `sscale` M18,
   `sarray` M20-M22, `smemory` M8), each backing new cases. 212 in total.

## Decisions and reasons

- `TRUE`/`FALSE` stay plain `1`/`0` per the shared header contract; their
  comparisons remain the recorded 10.4 deviation.
- Signed operands keep plain constants; only unsigned ones get `u`.
- `sarraySumu8`/`u16` overflow tested with 16 MB / 128 KB static arrays
  (host only) rather than listed as unreachable.
- A table changed after `sscaleInit` is a contract violation; the suite pins
  that `Apply` and `ApplyClamped` refuse it.

## Verification (final tree, this session)

- `bash tools/run_all.sh` with zig clang and with MinGW gcc 14.2: all 29
  suite lines `0 failed` (3803 module cases + 38 integration), strict
  warnings 0, UBSan clean, apicheck 0, doxcheck 0 (669 functions),
  tracecheck 0 gaps / 10 exceptions. Its drift step reports DRIFT only
  against HEAD; regenerating reproduced all six generated files
  byte-for-byte (hashes compared before and after).
- `python tools/mutate.py`: 212 mutants, 192 killed, 20 equivalent, every
  one accounted for.
- `bash tools/misra.sh` (cppcheck 2.21.0): both controls pass, 0 findings
  outside deviations, 0 manifest differences, 359 occurrences.
- cppcheck general checks with the CI options: 0 findings. doxygen 1.17.0:
  0 warnings.
- `bash tools/coverage.sh gcc`: `sbits`, `svote`, `swatch` at 100% branches
  and MC/DC; table in `CLAUDE.md`.
- `test/harness/sfixed_rounding.c`: exact counts, exit 0.
- Independent verifier agent (before the last round of test additions):
  additionally confirmed gcc 16 `-fanalyzer -Werror`, ARM cross build of
  all 14 modules with `-fanalyzer`, the header coexistence check and
  `tools/size.sh` (`.data`/`.bss` 0, 23638 B M0+, 21206 B M4). The later
  changes touched tests, tools, docs and comments only.
- Independent safety-reviewer agent: 0 defects; its six contract items and
  one question were all addressed.
- Not verifiable here: the CI coverage job's gcc-14/gcov-14 selection and
  the CI MISRA job with apt cppcheck on ubuntu-24.04 against the new
  manifest.

## Remaining work

1. After commit, read the first CI run's job conclusions, especially
   `cppcheck and MISRA C 2012` and the coverage job.
2. Classify the MC/DC gaps of the other ten modules
   (`<module>.conditions.txt`).
3. Lines holding more than one comparison among the 359 remaining 10.4
   occurrences pass the coarse rationale test by line; a per-expression
   review is still a review, not a proof.
4. Target (ARM) execution remains a permanent limit per `CLAUDE.md`.
