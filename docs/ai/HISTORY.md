# Work history

This file keeps durable project work notes for Codex and Claude. The current
state and next action belong in [HANDOFF.md](HANDOFF.md); this file records
the work that led to that state. Chat transcripts and private agent memory are
not required to resume the project.

## 2026-11-05 — Shared assistant context and standards references

- Added repository-wide [AGENTS.md](../../AGENTS.md), shared
  [WORKFLOW.md](WORKFLOW.md), and this [handoff protocol](HANDOFF.md). Added a
  short shared-context section to [CLAUDE.md](../../CLAUDE.md), keeping its
  existing technical guidance as the authority. The intent is a file-based
  handoff for the same checkout; it is not background synchronization between
  separate clones.
- Inspected the owner's `G:\My Drive\Standards` collection and selected ten
  PDFs relevant to a portable safety-related C library. Created
  [standards index](../standards/README.md), [source register](../standards/SOURCES.md),
  [IEC 61508](../standards/IEC_61508.md), [ISO 26262](../standards/ISO_26262.md),
  [MISRA C:2012](../standards/MISRA_C_2012.md), and
  [repository evidence map](../standards/EVIDENCE_MAP.md).
- Source notes identify local editions, clauses, PDF page positions and
  SHA-256 hashes. The MISRA file is the March 2013 publication, not the
  2019 first revision. The notes distinguish standard text, project
  interpretation and evidence limits; they do not claim SIL/ASIL or
  compliance. Standards PDFs and extracted text were not added to the repo.
- Added links to the standards reference from the repository README,
  CLAUDE.md and shared workflow. Original standards files were read only.
- Verification recorded at that time: 11 Markdown files and 100 local
  links/anchors audited; hashes and selected clause/page locators checked;
  rendered pages of ISO 26262-6, IEC 61508-2 and MISRA visually inspected;
  `git diff --check` passed. Library tests were not run for that
  documentation-only work.

## 2026-11-05 — Development roadmap and MISRA baseline

- Added `DEVELOPMENT_PLAN.md` covering repeatable baseline
  checks, MISRA deviation handling, API/test traceability, targeted tests,
  target integration evidence, and release/maintenance evidence.
- Ran the repository MISRA script with cppcheck 2.21.0 and its bundled addon.
  The checker control reported expected Rules 7.1 and 15.1. The baseline
  scan reported zero findings outside its then-current exception filters:
  Rule 10.4 matched 636 occurrences, Rule 11.5 25, Rule 11.6 10, and
  Rule 11.4 10.
- Confirmed that the local/downloaded MISRA PDF copy has 236 pages, title-page
  date March 2013, and the same SHA-256 as the source register.
- The previous shell invocation initially appeared incomplete because the
  command returned a session ID after 30 seconds; polling that session was
  needed to obtain its final output. The control itself passed.

## 2026-11-06 — Rule 10.4 occurrence filter

- Replaced the broad Rule 10.4 rule/module wildcard with
  [tools/misra_10_4.tsv](../../tools/misra_10_4.tsv), keyed by module, line,
  SHA-256 of exact source-line bytes, and expected occurrence count. The
  baseline had 556 keys for 636 diagnostics because multiple diagnostics can
  share one line.
- Added [tools/misra_occurrences.py](../../tools/misra_occurrences.py).
  It compares current analyzer locations against the manifest, detects
  extra, missing, changed and duplicate occurrences, and supports selected
  module scans. Its self-test covers those comparisons.
- Updated [tools/misra.sh](../../tools/misra.sh) and
  [tools/README.md](../../tools/README.md), plus CLAUDE.md and the MISRA
  application notes. Output calls the entries matched Rule 10.4 allowlist
  occurrences. No automatic baseline-regeneration path exists. A source
  fingerprint is a drift control; it does not provide the required technical
  rationale, risk analysis, authorization or proof of guideline compliance.
- Two independent read-only agent reviews contributed findings. One identified
  the danger of automatic manifest rewriting and the missing per-finding
  rationale. Another audited representative module groups and found
  out-of-rationale numeric, arithmetic and Boolean-control findings. Those
  findings were not represented as formally approved deviations.
- Audits covered:
  - `sdiag`, `sring`, `sstate`, `swatch`: 84 occurrences reviewed; no
    clearly out-of-scope findings spotted, while retaining the limitation
    that line-only analyzer output can conflate expressions on one line.
  - `sfixed`, `sscale`, `sfault`, `sfilter`, `svote`: at least 15
    numeric count/limit findings in `sfault`, `sfilter` and `svote` fell
    outside the status/Boolean rationale.
  - `sbits`, `sarray`, `sstring`, `smemory`, `smath`: review identified
    many numeric-bound, helper-Boolean and arithmetic findings outside or
    ambiguous against the rationale. At least 321 of the original 636 were
    classified outside the documented status/Boolean rationale across the
    reviewed groups. A full per-expression rationale audit is still required.
- Fixed fifteen unsigned zero comparisons in `sfault.c`, `sfilter.c` and
  `svote.c` by using `0u`; this removed 15 analyzer occurrences. Updated
  each file's version, date and Doxygen history. Behavior is unchanged.
- The manifest now checks 621 remaining Rule 10.4 occurrences. It originally
  had 556 unique keys; the removed findings' keys were deleted. Since source
  line numbers are part of the key, inserting banner history lines required
  updating manifest line numbers for the affected modules; hashes still
  validate exact source lines.
- Updated `docs/standards/MISRA_C_2012.md` to reflect the current occurrence
  checker and distinguish it from the former wildcard at base `0348352`.
  Updated the CLAUDE.md summary count to distinguish 621 current matches from
  the 636 baseline.
- Latest completed verification:
  - `tools/misra.sh`, cppcheck 2.21.0 + bundled `misra.py`: control passed,
    14 modules, zero findings outside configured scopes, 621 10.4 allowlist
    matches.
  - `tools/misra.sh sstate`: earlier partial scan passed with 20 10.4 matches.
  - `python tools/misra_occurrences.py --self-test`: passed.
  - `bash tools/run_all.sh 'python -m ziglang cc'`: all 14 module suites,
    integration suite (38 cases), strict warnings, UBSan, API check, Doxygen
    check, and generated-source consistency passed. ASan was skipped because
    the allocation-overread control was inert on this host.
  - `git diff --check` and Bash syntax check passed.
- Committed and pushed as `54e59d7`. No CMake/Make build system, target
  validation or product safety-integrity claim was introduced.

## 2026-11-07 — Review of the Codex session (Claude)

- `54e59d7` broke five mutant anchors (`sfault` M11, M15, M16, `sfilter` M10,
  `svote` M16): the `0` to `0u` edits changed text the mutants match, and
  `tools/mutate.py` was not run. CI's reporting mutation job concluded
  `failure` on that commit. The anchors were updated.
- `DEVELOPMENT_PLAN.md` had been committed as a one-byte file while the
  handoff pointed at items inside it. `CLAUDE.md` asks for no plan documents
  in this repository, so it was deleted; the ordered open work lives in
  [HANDOFF.md](HANDOFF.md).
- The Rule 10.4 manifest was keyed by source line number, so adding a single
  `@par History` line to a module invalidated every later key in it.

## 2026-11-07 — Open work items 1-7 (Claude)

- Rule 10.4 manifest re-keyed by module, enclosing function, line hash and
  count, with a coarse rationale guard and a self-test that `misra.sh` runs
  as a control. Then every unsigned operand meeting an unsuffixed constant
  was fixed (621 to 359 occurrences); the deviation text in every banner
  and the template now covers only enum and `TRUE`/`FALSE` comparisons.
  The rationale guard caught one line the remediation script had skipped
  (`*required = srcLen + 1;`, a code line starting with `*`).
- `tools/tracecheck.py` added: every status a `@return` promises must be
  expected by the module's own suite or recorded with a reason. First run
  106 gaps. Real findings: `sstring` had never been given an unterminated
  source by 21 functions or a zero capacity by 17; `sfixedFloor` and
  `sfixedRound` promised an `SX_UNDERFLOW` that none of the 2^32 inputs
  produces (contract corrected); `sfilter` discarded seven success statuses;
  `sscaleApply`/`ApplyClamped` refusal of a table changed after Init had
  never been asked for (and 50 million fuzzed tables never reached
  `sscaleFindSegment`'s refusal). New mutants `sstring` M19, `sfilter` M14,
  `sscale` M18 are killed only by the new cases.
- `coverage.sh` gained an MC/DC column (gcc 14+); CI pinned to
  `ubuntu-24.04` and `actions/checkout@v5`.
- Follow-up from independent review: the template's own 10.4 hit, sentinel
  checks on refused outputs in `sstring` and `svote`, Floor/Round pinned at
  the bottom of the type plus a portable exhaustive harness, four mutants
  backing the sum-overflow and CopyN-overlap cases, `-O`-proof checker
  controls. MC/DC classification of four modules found six more real cases.
  Final: 3841 cases, 212 mutants all accounted for.
- Process note: shell heredocs in this environment collapse backslash
  escapes (`\n`, `\b`, line continuations). Two edits were damaged that
  way and repaired; write scripts with the file tool instead.
- Committed as `ea54309`. Its first CI run: every job green, including the
  MISRA job with the image's cppcheck against the new manifest and the
  coverage job with gcc-14.

## 2026-11-07 — MC/DC classified everywhere, 10.4 checked by expression (Claude)

- Every MC/DC gap in the ten unclassified modules classified. 128 condition
  outcomes were missing cases, mostly one pointer or one capacity of a
  guard never refused on its own; all added. Real holes:
  `smathInRange` never asked below or on its lower bound in any family;
  `sarray`'s overlap test only ever saw ranges miss one way round;
  `sscaleLinear`/`LinearClamped`/`ApplyClamped` never exercised on the far
  side or inside of a falling map; `sstring` character classes never met
  the characters past their ends. Nine new mutants die only to the new
  cases. Final: 4064 cases, 221 mutants (201 killed, 20 equivalent).
- `misra_occurrences.py` now checks each Rule 10.4 finding by expression:
  `misra.sh` runs the addon with `--cli` for the column, and the dump's
  token at that column must be `==`/`!=` with a `TRUE`/`FALSE` expansion or
  the module's own enum member as an operand. All 359 pass; a control line
  holding a status comparison beside an unsuffixed unsigned one is refused
  at the second comparison's column, where the old line test passed it.
- `coverage.sh` prints its table as one GitHub notice, so CI's numbers can
  be read without log access; the first such notice confirmed the runner
  measured with gcc-14 and gcov-14 and matched the local numbers exactly.
  `upload-artifact` moved to v7 (v4 and v5 still run on Node.js 20).

## 2026-11-07 — New function blocks and five new modules (Claude)

Agreed in chat, one commit per piece, all verified on the host only (no
ARM hardware; cross-compile, `-fanalyzer` and `size.sh` as before).

- `sdiag` CRC-8 (SAE J1850 and polynomial 0x2F), `sbits` byte order
  readers and writers (owner approved; bit order stays Intel only).
- New modules `ssignal` (plausibility), `se2e` (end to end protection with
  an injected CRC), `slog` (fault event memory), `sstore` (record
  integrity, two copies), `spid` (Q16.16 PID). Each from `template/`, each
  at 100% lines, branches and MC/DC with every mutant killed.
- The integration suite gained two chains that pass sdiag's CRCs to `se2e`
  and `sstore` with no wrapper; the second found that a saved `slog` table
  could not be restored, and `slogAdopt` was added.
- Two tool holes found on the way: `tracecheck.py` accepted a suite
  directory whose name matched only case-insensitively (the first `se2e`
  commit went red on the Linux runner only; renamed to `SE2e_Test` and the
  tool now matches case), and `apicheck.py` skipped every prototype wrapped
  onto a second line (`sstringToken` had never been checked).
- `spid` M14 survived the first suite: the Init clamp of the integrator is
  only visible through an integrating case with limits above zero.
- Final: 19 modules, 394 functions, 4664 cases, 302 mutants (282 killed,
  20 equivalent), 406 Rule 10.4 occurrences all inside the rationale by
  expression.

## 2026-11-07 — sstate time in state and sscale 2D map (Claude)

- `sstateTick`/`sstateGetDwell`, and a bilinear `sscalemap_t` with one
  rounding and per-cell overflow checks at Init and Apply. MC/DC showed the
  map's segment search lower-bound test was unreachable; it was removed
  rather than classified. Final: 399 functions, 4800 cases, 318 mutants
  (297 killed, 21 equivalent), 420 Rule 10.4 occurrences.

## Open work carried forward

See [HANDOFF.md](HANDOFF.md).
