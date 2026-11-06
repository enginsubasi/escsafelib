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

- Added [DEVELOPMENT_PLAN.md](DEVELOPMENT_PLAN.md) covering repeatable baseline
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
- Changes remain uncommitted and unpushed. No CMake/Make build system, target
  validation or product safety-integrity claim was introduced.

## Open work carried forward

See [HANDOFF.md](HANDOFF.md) and the open item in
[DEVELOPMENT_PLAN.md](DEVELOPMENT_PLAN.md). The next technical priority is to
remediate or precisely justify the remaining out-of-rationale Rule 10.4
findings before proceeding to API/test traceability. Target-specific timing,
fault injection, memory model and safety reactions need a defined target and
fault model.
