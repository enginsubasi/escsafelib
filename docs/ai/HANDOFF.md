# Current handoff

Historical milestones and prior findings are recorded in
[HISTORY.md](HISTORY.md). This file is the current state and next action.

## Session

- Last updated: 2026-11-07.
- Assistant: Claude.
- Branch: `main`.
- Objective: new function blocks and new modules, agreed with the owner in
  chat on 2026-11-07 (no plan document, per `CLAUDE.md`). The owner has no
  ARM hardware, so every piece is pure integer logic verified on the host;
  ARM is cross-compile, `-fanalyzer` and `size.sh` only, as before.
- Agreed order, one commit per piece:
  1. `sdiag` CRC-8 (SAE J1850 and polynomial 0x2F). **Done.**
  2. `sbits` byte order (owner approved; bit ordering stays Intel only).
     **Done.**
  3. `ssignal` (prefix `SG_`): range, gradient, stuck-at and two-sensor
     plausibility. Observation only; qualification stays in `sfault`.
     **Done.**
  4. `se2e` (prefix `SE_`): counter, data ID and injected CRC on send; a
     receiver state machine telling repetition, loss, wrong order,
     corruption apart (timeout is `swatch`'s, by decision). No
     compatibility claim with any specification. **Done.**
  5. `slog` (prefix `SL_`): fault event memory in a caller-owned table,
     saturating counters, full-table policy chosen at Init. **Done.**
  6. `sstore` (prefix `SO_`): record header, version, length and CRC over a
     byte buffer; choose the valid and newer of two copies. **Done.**
  7. `spid` (prefix `SP_`): Q16.16 PID with anti-windup and output limits,
     its own arithmetic. **Done.**
  Optional, only if the owner asks: `sscale` 2D map, `sstate` time in state.

## Piece 1: `sdiag` CRC-8

- `sdiagCrc8Sae`, `sdiagCrc8SaeUpdate`, `sdiagCrc8H2F`,
  `sdiagCrc8H2FUpdate`, sharing static `crc8Byte` and `crc8Block`;
  `SD_CRC8_SEED` is 0x00 (register starts at its inverse, result inverted,
  so chaining is exact, as for CRC-32). The polynomial is not public.
- Check values 0x4B and 0xDF over "123456789" and residues 0x3B and 0xBD,
  confirmed by an independent Python computation before the C was written.
- Suite: catalogue answers, residues, chaining split at every position,
  every one and two byte message against a bit serial oracle that shares
  no code, every single bit error in a frame, refusals with the output
  untouched. `sdiag` M9-M12 all killed.
- Verified: zig and gcc 14 strict, 126 cases 0 failed; doxcheck,
  apicheck, tracecheck 0; MISRA `sdiag` clean (one more 11.5 hit inside
  the deviation, 26 in total); ARM M0+ `-fanalyzer` clean; `size.sh`
  `.data`/`.bss` 0.

## Piece 2: `sbits` byte order

- `sbitsReadLE`, `sbitsReadBE`, `sbitsWriteLE`, `sbitsWriteBE`: unsigned
  integers of one to four bytes (`SBITS_MAXBYTES`) at a byte offset, one
  byte at a time, with static `orderFits` checking the count and the end in
  64 bits. Writers refuse a value too big for its slot (`SB_OVERFLOW`). A
  signed reading is documented as the unsigned read plus `sbitsGeti32`;
  no separate signed API, by decision.
- Suite: known frames, every count at every offset against `sbitsGetBytes`
  and the reversed frame, range edges round trip, sign extension of a BE
  reading, every refusal with frame and output untouched. `sbits` M18-M24
  all killed.
- Verified: zig with UBSan and gcc 14 strict, 192 cases 0 failed;
  `run_all.sh` ALL GREEN; coverage `sbits` still 100% lines, branches and
  MC/DC; ARM M0+ `-fanalyzer` clean; MISRA full tree 0 differences after
  four reviewed `retVal == SB_OK`-style records were added (363 in all).

## Piece 3: `ssignal`

- New module `inc/signal/ssignal.h`, `src/signal/ssignal.c`, from
  `template/`: `ssignalCheckRange`, `ssignalCheckGradient`,
  `ssignalCorrelate` (stateless), `ssignalStuckInit`/`Update`/`Reset`/
  `Count` on `ssignalstuck_t`. Status enum `SSIGNALSTATUS` (`SG_`), verdict
  enum `SSIGNALVERDICT` (`SG_VERDICT_BELOW`/`INSIDE`/`ABOVE`).
- Decisions: verdicts rather than flags (below and above are different
  faults); 64-bit differences; correlation by cross multiplication with the
  gain limited to `SSIGNAL_MAXGAIN` (65535) so products stay under 2^47;
  stuck runs measured against the run's first sample, saturating, limit at
  least two; zeroed driver refused with `SG_NULLPTR`.
- Registered in `run_all.sh`, `coverage.sh`, the CI test matrix, the CI
  sanitizer job and the CI cppcheck include list. MISRA, size, the ARM
  steps and the header check discover modules on their own.
- Verified: 115 cases on zig with UBSan and gcc 14 strict; M1-M13 all
  killed; coverage 100% lines, branches and MC/DC; tracecheck, doxcheck,
  apicheck 0; MISRA full tree 0 differences after three reviewed
  `isReady ( driver ) == FALSE` records (366 in all); cppcheck general 0;
  gcc 16 `-fanalyzer -Werror` and ARM M0+/M4 `-fanalyzer` clean; header
  coexistence clean; `.data`/`.bss` 0, 460 B on M0+; `run_all.sh` ALL
  GREEN.

## Piece 4: `se2e`

- New module `inc/e2e/se2e.h`, `src/e2e/se2e.c`: `se2eTxInit`,
  `se2eProtect`, `se2eRxInit`, `se2eCheck`, `se2eRxResync`. Frame: byte 0
  CRC, byte 1 counter, payload after (`SE2E_HEADERSIZE` 2). CRC over the
  16-bit data ID (low byte first, never sent), the counter and the payload,
  through an injected `se2ecrc_t` shaped like `sdiagCrc8SaeUpdate`.
- Decisions: status says whether the check ran, `SE2ERESULT` says what the
  frame is (OK, OKSOMELOST, INITIAL, REPEATED, WRONGSEQUENCE, ERROR); no
  timeout (no frame means no call; `swatch` and `sfault` own that); every
  good non-repeated frame becomes the reference, a wrong sequence
  included; `maxDelta` 1-127; zeroed driver refused by `crc == NULL`.
- The integration suite gained `testProtectedLink`: `sdiagCrc8SaeUpdate`
  passed to se2e with no wrapper, `sbitsWriteBE`/`ReadBE`, `ssignal` range
  and correlation, and `sfault` confirming three corrupted frames. 38 to
  57 cases; its build lines in `run_all.sh`, `coverage.sh`, CI and the
  README now name eleven modules.
- Verified: 75 cases on zig with UBSan and gcc 14 strict; M1-M12 all
  killed; coverage 100% lines, branches, MC/DC; checkers 0; MISRA full
  tree 0 differences after three reviewed records (369 in all); cppcheck,
  gcc 16 analyzer, ARM M0+/M4 analyzer clean; `.data`/`.bss` 0;
  `run_all.sh` ALL GREEN. CI of the `ssignal` commit: every job success.

## Piece 5: `slog`

- New module `inc/log/slog.h`, `src/log/slog.c`: `slogInit`, `slogRecord`,
  `slogFind`, `slogGet`, `slogCount`, `slogLost`, `slogRemove`, `slogClear`
  over a caller-owned `slogentry_t` table. Policies `SL_POLICY_KEEP`
  (refuse with `SL_FULL`) and `SL_POLICY_REPLACE` (evict the entry with the
  largest unsigned `cycle - lastCycle`, first on a tie); every refused or
  displaced event counts in `lost`, saturating. Remove fills the hole with
  the last entry. `isReady` also refuses `used > capacity`.
- Verified: 127 cases on zig with UBSan and gcc 14 strict; tracecheck
  caught two unasserted `SL_OK`s on the first run (fixed); M1-M14 all
  killed; coverage 100% lines, branches and MC/DC; MISRA 0 differences
  after 15 reviewed records (384 in all); cppcheck, analyzers, ARM, header
  check clean; `.data`/`.bss` 0; `run_all.sh` ALL GREEN.
- Registration is now one script per module (scratchpad, not committed):
  `run_all.sh` and `coverage.sh` lists, the CI matrix and sanitizer steps
  and the CI cppcheck include list.

## Piece 6: `sstore`, with `slogAdopt` and an `apicheck` fix

- New module `inc/store/sstore.h`, `src/store/sstore.c`: `sstoreSeal`,
  `sstoreCheck`, `sstoreSelect`; stateless, CRC-32 passed as a
  `sstorecrc_t` shaped like `sdiagCrc32Update`. Header: version u16, length
  u16, sequence u32, CRC u32, all little endian; payload written in place.
  Check verifies CRC before version; Select picks the valid copy, the newer
  across the sequence wrap when both are, A on a tie, and reports
  `SO_WRONGVERSION` when nothing is valid but something is intact.
- The integration suite's new `testStoredFaultMemory` (slog, sbits LE,
  sstore with `sdiagCrc32Update`, a torn copy) found that a saved slog table
  could not be restored: `slogAdopt` added, refusing duplicate identifiers
  and zero-occurrence entries with the new `SL_CORRUPT`.
- `tools/apicheck.py` skipped every prototype wrapped onto a second line
  (pattern stopped at a line end): `sstringToken` was never checked. Fixed
  (390 declarations, 0 findings) and a control with a wrapped prototype
  missing its capacity fires.
- Verified: sstore 73 cases, slog 155, integration 76, zig with UBSan and
  gcc 14 strict; sstore M1-M12 and slog M15-M18 killed; coverage 100% in
  both; MISRA 0 differences after 13 reviewed records (402 in all);
  cppcheck, analyzers, ARM, headers clean; `run_all.sh` ALL GREEN.

## Piece 7: `spid`

- New module `inc/pid/spid.h`, `src/pid/spid.c`: `spidInit`, `spidUpdate`,
  `spidReset`, `spidPreset`. Q16.16 throughout; products in int64 divided by
  65536 (no signed shift); derivative on measurement; integrator clamped to
  the output limits (anti-windup) and started at the nearest limit; Preset
  for bumpless transfer; zeroed driver refused through `ready`.
- Verified: 72 cases on zig with UBSan and gcc 14 strict, every expected
  value worked by hand; M1-M15 killed (M14 survived the first suite and a
  case was added); coverage 100% lines, branches, MC/DC; MISRA 0
  differences after four reviewed records (406 in all); cppcheck,
  analyzers, ARM, headers clean; `.data`/`.bss` 0; `run_all.sh` ALL GREEN.

## Remaining work

CI run `42352223713` of `8093f17` (`spid`): every job success; so were the
runs of every earlier piece except `ca747f9`, fixed by `647b066`.

1. Optional, only if the owner asks: `sscale` 2D map, `sstate` time in
   state.
2. Target (ARM) execution remains a permanent limit per `CLAUDE.md`.
