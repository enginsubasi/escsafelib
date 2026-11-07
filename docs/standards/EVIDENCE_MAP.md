# Repository evidence and integration questions

Snapshot: base `0348352`, reviewed 2026-11-05. This is a reading-based map,
not a verification report. No test suite, analyzer or target execution was
run for this task. Historical numbers in `CLAUDE.md` were not remeasured.

## Evidence already represented in the repository

| Subject | Inspected repository evidence | What remains outside that evidence |
|---|---|---|
| Coding and interface contracts | [codingReference.md](../../codingReference.md), source Doxygen, [apicheck.py](../../tools/apicheck.py), [doxcheck.py](../../tools/doxcheck.py) | The checkers enforce selected rules; a complete safety-requirements or MISRA guideline matrix was not established. |
| Functional behavior and robustness | [Module suites](../../test), [integration example](../../test/Integration_Test/Integration_Test.c), [tracecheck.py](../../tools/tracecheck.py) | `tracecheck.py` links every status a function's documentation promises to a case in its own suite that expects it, or to a recorded exception. That is traceability to the documented interface, not to project requirements: a claim still needs the link from those requirements and the configuration being claimed. Test count alone does not supply it. |
| Test sensitivity | [Mutation runner](../../tools/mutate.py), [mutants](../../tools/mutants), [Windows harnesses](../../test/harness/README.md) | Curated mutants sample defect classes. Killed mutants do not measure software failure probability or hardware diagnostic coverage. |
| Structural coverage | [coverage.sh](../../tools/coverage.sh) | It collects line and branch results, and gcc's condition coverage (MC/DC) when the compiler is gcc 14 or later. Every MC/DC gap has been classified as a missing case (added) or an unreachable condition (listed in `CLAUDE.md`); the measurement is host-side and not tool-qualified, and nothing here is an architecture-level call-coverage argument. |
| Generated module consistency | [Generator instructions](../../tools/README.md), [CI workflow](../../.github/workflows/ci.yml) | Regeneration consistency is different from correctness of the generator and independence of the expected results. |
| Static analysis and deviations | [misra.sh](../../tools/misra.sh), source banners, CI analyzer jobs | Tool controls, rule coverage, individual filtered findings and target model must be evaluated in the actual use. See [MISRA notes](MISRA_C_2012.md). |
| Target build and resource information | CI ARM build, [size.sh](../../tools/size.sh) | Object size and zero module `.data`/`.bss` do not bound caller-owned RAM, stack, execution time or target behavior. |
| Configuration and communication | Git history, CI definitions, [shared workflow](../ai/WORKFLOW.md) | They support retention and handoff; they do not establish a complete release evidence package, competent approval or independent assessment. |

The gaps above are **not established by the inspected repository evidence**,
not assertions that the owner has no additional records elsewhere. Relevant
source discussions are in [IEC 61508](IEC_61508.md) and [ISO 26262](ISO_26262.md).

## Module-level questions for an integrator

The following are project interpretations of the cited standards, combined
with the actual module contracts. They do not introduce new API behavior.

| Modules and source | Useful property | Assumption or question to carry into the integrating project |
|---|---|---|
| [sstring](../../src/string/sstring.c), [sarray](../../src/array/sarray.c), [smemory](../../src/memory/smemory.c) | Bounded access and explicit status reporting | Do pointers designate live accessible objects of the declared size, with suitable alignment where typed? Does each size use the right unit? Check overlap and integer address-order assumptions. A NULL check cannot establish object validity. |
| [smath](../../src/math/smath.c), [sfixed](../../src/fixed/sfixed.c), [sscale](../../src/scale/sscale.c) | Checked numeric operations, fixed-point arithmetic and table-based scaling | Are units, rounding, saturation/error policy and calibration ranges suitable? Who owns calibration storage, and can it change after validation? |
| [sfilter](../../src/filter/sfilter.c), [sfault](../../src/fault/sfault.c) | Signal conditioning and fault qualification | Do filter lag, the update period and confirmation/healing settings meet the system's reaction-time budget? Who serializes access to each driver? A cycle count is not a measured duration. |
| [se2e](../../src/e2e/se2e.c) | Counter and CRC over an implicit data identifier; receiver classification of each frame | Which CRC and identifier does each message use, is an eight bit CRC's Hamming distance enough at the frame length, who supervises the arrival deadline (`swatch`) and who qualifies the results (`sfault`)? No compatibility with any specified end to end profile is claimed. |
| [slog](../../src/log/slog.c) | Bounded fault event memory with counted losses | How large must the table be for the worst incident, which full table policy suits the diagnosis, and who reads the memory out and when? The table lives in RAM; keeping it across a reset is the integrator's. |
| [ssignal](../../src/signal/ssignal.c) | Range, gradient, two sensor correlation and stuck signal observations | Which limits, gains and tolerances come from the sensor's specification, and who qualifies an implausible reading into a fault (`sfault`) and reacts to it? A stuck limit counts samples, so its meaning depends on the caller's sample rate. |
| [swatch](../../src/watch/swatch.c) | Deadline and minimum-period supervision | Who supplies the clock, polls the watch and performs the reaction? Can that path fail with the supervised task? Preserve the true elapsed-time bound for modular tick arithmetic. |
| [sdiag](../../src/diag/sdiag.c) | Integrity checks, RAM tests, flow signatures and redundant storage | Which concrete fault model, memory region and test interval are covered? Can the test run without corrupting live data? What faults remain invisible to the implementation or the available harness? |
| [sstate](../../src/state/sstate.c) | Validated transition tables and refusal diagnostics | Does the application table encode the required safe behavior, including reset and forced transitions? A legal transition in a table is not necessarily safe for the physical system. |
| [svote](../../src/vote/svote.c) | Agreement tests and selection among redundant inputs | Are the inputs independent and fresh, and is the tolerance justified? What detects shared calibration, source or time-base failures? Voting does not create independent channels. |
| [sring](../../src/ring/sring.c) | Single-producer/single-consumer exchange | Are producer/consumer ownership, aligned atomic access and the compiler/hardware memory model satisfied? Is the injected barrier correct for the full target configuration? No portable multicore guarantee is inferred here. |
| [sbits](../../src/bits/sbits.c) | Bounded field encoding and decoding | Do sender and receiver agree on numbering, widths and signed interpretation? Packing and CRC helpers alone do not constitute a complete safety communication protocol. |

The requirement to identify use constraints follows from IEC 61508-3
7.2.2 and Annex D ([IEC3](SOURCES.md#iec3), PDF pp. 24-26, 99-100) and
ISO 26262-8 12.4.2 ([ISO8](SOURCES.md#iso8), PDF pp. 45-46).
The independence questions relate to ISO 26262-9 7.4
([ISO9](SOURCES.md#iso9), PDF pp. 22-24).

## Important distinctions to preserve in reviews

1. **Detection and reaction.** A diagnostic status needs an application
   response. Preserving an output on failure can preserve a stale value;
   the caller decides whether that is usable. Diagnostic outputs and driver
   state can also change on non-success statuses according to each API.
2. **Code coverage and diagnostic coverage.** Executed branches describe
   tests of software; detected dangerous hardware failures describe a system
   diagnostic argument. The percentages are not interchangeable.
3. **Independent modules and independent safety paths.** No cross-module
   includes helps copy-out and testing. It does not remove shared clocks,
   hardware, input data, compiler defects or common design assumptions.
4. **Reproducibility and correctness.** Generating the same C twice is useful
   but can repeat the same defect. A separately justified oracle or other
   independent checks can address that risk.
5. **Host and target results.** Record which environment each result covers.
   The existing project explicitly leaves target execution validation to
   the integrator; do not upgrade a cross-build result into target evidence.

These distinctions are supported by IEC 61508-2 Annex C.1
([IEC2](SOURCES.md#iec2), PDF p. 74), IEC 61508-3 7.4.4 and 7.5
([IEC3](SOURCES.md#iec3), PDF pp. 32-35, 38), ISO 26262-6 9.4.5
([ISO6](SOURCES.md#iso6), PDF p. 33), ISO 26262-8 11.4.5
([ISO8](SOURCES.md#iso8), PDF pp. 40-41), and the linked module contracts.

## What a reusable evidence record should answer

For a selected application and revision, a useful record connects:

- the module/functions used and the requirements or explicit assumptions
  allocated to them;
- the source identity, local modifications, generated inputs and build
  configuration;
- the runtime constraints, fault reaction, timing budget and ownership of
  each integration responsibility;
- the checks performed, their expected and actual outcomes, tool versions,
  controls and retained results;
- uncovered behavior, known anomalies, deviation rationale and the scope
  of any competent review or acceptance.

This is an evidence-reading aid, not a claim that these records are complete
or an instruction to start a separate implementation project. No SIL/ASIL
target, tool qualification route, new feature or code change was selected
by this documentation task.
