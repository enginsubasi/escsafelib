# Current handoff

Historical milestones and prior findings are recorded in
[HISTORY.md](HISTORY.md). This file is the current state and next action.

## Session

- Last updated: 2026-11-06.
- Assistant: Codex.
- Branch: `main`.
- Base commit: `0348352` (`* cppcheck, MISRA and doxygen are blocking CI jobs`).
- Objective: establish a repository-grounded development roadmap and begin
  its first checkpoint.
- Status: occurrence-level MISRA filter implemented and tested; source audit
  identified further 10.4 findings requiring remediation.

## Changes

- Created [docs/standards/README.md](../standards/README.md), `SOURCES.md`,
  `IEC_61508.md`, `ISO_26262.md`, `MISRA_C_2012.md` and `EVIDENCE_MAP.md`.
  They cover selected material from ten local PDFs with edition, clause/page
  locators, source hashes and repository evidence boundaries.
- Linked the standards notes from [README.md](../../README.md),
  [CLAUDE.md](../../CLAUDE.md) and [WORKFLOW.md](WORKFLOW.md).
- Added [DEVELOPMENT_PLAN.md](DEVELOPMENT_PLAN.md), an ordered roadmap covering
  baseline evidence, MISRA deviations, test traceability, targeted tests,
  target integration and release evidence.
- Added [HISTORY.md](HISTORY.md) with the durable record of the shared-assistant
  setup, standards research, roadmap, MISRA implementation, independent
  reviews, test outcomes and open findings. `CLAUDE.md` and `WORKFLOW.md` now
  direct Claude to use it alongside this current-state handoff.
- Verified the owner's Downloads copy of MISRA C:2012 against the registered
  source: 236 pages, March 2013 title page, matching SHA-256.
- Replaced the Rule 10.4 wildcard with [an occurrence manifest](../../tools/misra_10_4.tsv).
  Its original baseline had 556 keys for 636 findings. Keys include module,
  line, SHA-256 of line content and duplicate count. Full and selected-module
  checks reject new, changed and stale occurrences.
- Added `tools/misra_occurrences.py` comparison and self-tests; updated
  `tools/misra.sh`, `tools/README.md` and the MISRA guidance in `CLAUDE.md`.
  The report calls these matched allowlist occurrences; it explicitly does
  not infer formal deviation approval or justify each occurrence.
- Two independent review agents checked design and standards boundaries.
  Their findings were incorporated: no automatic manifest regeneration path;
  duplicate-aware counts; explicit warning that source fingerprints are not
  technical rationale; checker control and submodule matching retained.
- Source review found at least 321 of the baseline 636 findings outside the
  recorded status/Boolean rationale in the inspected modules. Fifteen
  unsigned zero comparisons in `sfault`, `sfilter` and `svote` were changed
  to `0u`, and their manifest entries removed. Numeric, arithmetic and
  helper-Boolean findings elsewhere remain under review.
- Preserved the earlier uncommitted shared-context setup: `AGENTS.md`, the
  shared-context section in `CLAUDE.md`, and `docs/ai/`.
- Three C source modules now use unsigned zero constants for unsigned
  threshold/count comparisons; behavior is unchanged. No commit or push has
  been made. Changes remain local and uncommitted.

## Decisions and reasons

- Keep the notes as source-grounded engineering interpretations, not as a
  SIL/ASIL assignment, qualification report or compliance declaration.
- The local MISRA PDF is the March 2013 publication; its filename does not
  establish that it contains the 2019 revision or later amendments.
- Preserve the existing technical/style rules and deviation policy. The
  notes identify evidence limits, including rule/module-wide deviation
  filtering, missing MC/DC evidence in the current coverage script and
  the host/target validation boundary; they do not silently change policy.
- Reuse the single source of technical guidance in `CLAUDE.md`; both
  assistants reach the standards index through the shared workflow.
- Follow the existing language convention: Turkish conversation, English
  repository files.

## Verification

- Read selected full PDF pages and relevant repository contracts/tools.
- Rendered and visually checked ISO 26262-6 PDF p. 33, IEC 61508-2 PDF p. 56
  and MISRA PDF p. 3 to verify the coverage tables and edition identity.
- `tools/misra.sh` with cppcheck 2.21.0 and bundled `misra.py`: control passed;
  14 modules scanned; 0 findings outside recorded scopes; 621 Rule 10.4
  occurrences matched after the 15 source fixes. The original baseline was
  636 matches across 556 keys. `tools/misra.sh sstate` passed earlier.
- `python tools/misra_occurrences.py --self-test`: passed matching, extra,
  missing, changed-fingerprint and duplicate-count cases.
- `bash tools/run_all.sh 'python -m ziglang cc'`: all 14 suites, integration
  (38 cases), strict-warning checks, UBSan, API check, Doxygen check and
  generator consistency passed. ASan was correctly skipped because its
  control was inert on this host.
- `git diff --check`: passed. Removed cppcheck dump artifacts created during
  source-format inspection.
- Downloaded MISRA PDF identity and SHA-256 verified against `SOURCES.md`.
- `git diff --check`: passed before this handoff/source-register update.
- `python -X utf8 -` (inline documentation/source audit): passed for
  11 Markdown files and 100 local links/anchors before this handoff update.
  All ten source hashes/filenames match the inspected copies; ten key
  clause/page locators were checked against extracted source pages.
- Removed the temporary PDF, extracted-text, image and helper cache;
  the original standards files were accessed read-only.
- Library builds, tests and CI: not run; this task changes Markdown only.
  Historical results in `CLAUDE.md` were not revalidated in this session.

## Remaining work and next step

- Continue checkpoint 2, item 7 of [DEVELOPMENT_PLAN.md](DEVELOPMENT_PLAN.md):
  remediate the remaining numeric-zero, arithmetic and helper-Boolean 10.4
  findings outside the documented rationale. Then proceed to API-to-test
  traceability. Keep hardware-specific validation gaps explicit.
- The prior standards-notes task is complete; its evidence gaps remain
  review findings rather than automatic coding authorization.
