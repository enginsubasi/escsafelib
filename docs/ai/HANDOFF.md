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
  2. `sbits` byte order: u16/u32 read and write with explicit `LE`/`BE`
     names (owner approved; bit ordering stays Intel only).
  3. `ssignal` (prefix `SG_`): range, gradient, stuck-at and two-sensor
     plausibility. Observation only; qualification stays in `sfault`.
  4. `se2e` (prefix `SE_`): counter, data ID and injected CRC on send; a
     receiver state machine telling repetition, loss, wrong order,
     corruption and timeout apart. No compatibility claim with any
     specification.
  5. `slog` (prefix `SL_`): fault event memory in a caller-owned table,
     saturating counters, full-table policy chosen at Init.
  6. `sstore` (prefix `SO_`): record header, version, length and CRC over a
     byte buffer; choose the valid and newer of two copies.
  7. `spid` (prefix `SP_`): Q16.16 PID with anti-windup and output limits,
     its own arithmetic.
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

## Remaining work

1. Pieces 2-7 above, in order.
2. Target (ARM) execution remains a permanent limit per `CLAUDE.md`.
