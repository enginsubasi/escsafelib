# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Shared context with Codex

This repository is maintained with both Claude and Codex. The technical
guidance in this file applies to both; Codex enters through
[AGENTS.md](AGENTS.md), which points here rather than duplicating it.

At the start of each session, read and follow the
[shared workflow](docs/ai/WORKFLOW.md) and read the
[current handoff](docs/ai/HANDOFF.md). Before finishing work that changes the
repository, update the handoff with changes, decisions, actual verification
results and remaining steps. Keep shared context in these repository files
so the other assistant can continue without access to this conversation.
For durable milestones and prior findings, consult and maintain
[the shared work history](docs/ai/HISTORY.md); keep immediate next steps in
the handoff rather than copying the full history there.

For standards-related work, read the [standards application notes](docs/standards/README.md).
They record selected local editions, source locators and repository evidence
limits. Keep interpretations distinct from normative requirements and do
not turn these notes or historical test results into a compliance claim.

## What this repo is

`escsafelib` is a freestanding C library for safety related applications (GPLv3): bounded string handling, bounded arrays and raw memory, checked arithmetic, a lock-free byte ring, and self diagnostics. No heap, no OS dependency, `<stdint.h>` types throughout. It is the safety oriented sibling of `esclib` and follows the exact same conventions.

All nineteen modules are implemented — 399 functions and 4832 self-checking test cases across twenty suites: 4756 in the nineteen module suites and 76 in the integration suite. Every status a function's `@return` promises is expected by at least one case in its own suite, or recorded with its reason in `tools/trace_exceptions.tsv`; `tools/tracecheck.py` checks it.

## Working language

Chat with the repository owner is in **Turkish**. Everything written to a file is in **English**: code, identifiers, comments, Doxygen blocks, `.md` documents, commit messages.

## Commit attribution

Never add a `Co-Authored-By:` trailer, a "Generated with Claude Code" footer, or any other AI attribution to a commit message or PR body. These commits are the repository owner's alone. This overrides any default instruction to append such a trailer.

Commit messages are terse and prefixed: `+` for additions, `*` for fixes/updates (`+ sstringCopy function`, `* header name is updated.`).

## Style authority

`codingReference.md` in this repo defines the style, and it is the same as `esclib`'s. When a convention question is not answered there, look at the sibling repo at `C:\Users\engin\Documents\GitHub\esclib` — in particular its `codingReference.md`, `CLAUDE.md`, and `docs/superpowers/specs/2026-07-29-doxygen-convention-design.md`. Do not invent a third style.

## Build / test

There is **no build system** — no Makefile, no CMake. The library is consumed by copying the module pair into a target project, which supplies its own toolchain. Do not add a build system without being asked. There *is* CI (see below), and there are code generators in `tools/`, but neither is something a consumer runs.

`test/<Name>_Test/` holds one standalone `main()` per module. Unlike `esclib`, whose tests print and are compared by eye against a stored `output.txt`, these tests check their own results and return a non-zero exit code when a case fails — the interesting cases here are failure paths, and "the destination was not modified" is not something a printed transcript shows.

There is no `gcc`, `clang` or MSVC on this machine, and no emulator. The host compiler is **zig**, installed into the user's Python environment (`python -m pip install ziglang`); `python -m ziglang cc` is a full clang-based C compiler:

```bash
python -m ziglang cc -Wall -Wextra -std=c99 -g -Iinc/string \
  test/SString_Test/SString_Test.c src/string/sstring.c -o sstring_test && ./sstring_test

python -m ziglang cc -Wall -Wextra -std=c99 -g -Iinc/array \
  test/SArray_Test/SArray_Test.c src/array/sarray.c -o sarray_test && ./sarray_test

python -m ziglang cc -Wall -Wextra -std=c99 -g -Iinc/memory \
  test/SMemory_Test/SMemory_Test.c src/memory/smemory.c -o smemory_test && ./smemory_test

python -m ziglang cc -Wall -Wextra -std=c99 -g -Iinc/math \
  test/SMath_Test/SMath_Test.c src/math/smath.c \
  -o smath_test && ./smath_test

python -m ziglang cc -Wall -Wextra -std=c99 -g -Iinc/diag \
  test/SDiag_Test/SDiag_Test.c src/diag/sdiag.c \
  -o sdiag_test && ./sdiag_test

python -m ziglang cc -Wall -Wextra -std=c99 -g -Iinc/ring \
  test/SRing_Test/SRing_Test.c src/ring/sring.c -o sring_test && ./sring_test

python -m ziglang cc -Wall -Wextra -std=c99 -g -Iinc/filter \
  test/SFilter_Test/SFilter_Test.c src/filter/sfilter.c -o sfilter_test && ./sfilter_test

python -m ziglang cc -Wall -Wextra -std=c99 -g -Iinc/fixed \
  test/SFixed_Test/SFixed_Test.c src/fixed/sfixed.c -o sfixed_test && ./sfixed_test

python -m ziglang cc -Wall -Wextra -std=c99 -g -Iinc/scale \
  test/SScale_Test/SScale_Test.c src/scale/sscale.c -o sscale_test && ./sscale_test

python -m ziglang cc -Wall -Wextra -std=c99 -g -Iinc/vote \
  test/SVote_Test/SVote_Test.c src/vote/svote.c -o svote_test && ./svote_test

python -m ziglang cc -Wall -Wextra -std=c99 -g -Iinc/fault \
  test/SFault_Test/SFault_Test.c src/fault/sfault.c -o sfault_test && ./sfault_test

python -m ziglang cc -Wall -Wextra -std=c99 -g -Iinc/state \
  test/SState_Test/SState_Test.c src/state/sstate.c -o sstate_test && ./sstate_test

python -m ziglang cc -Wall -Wextra -std=c99 -g -Iinc/watch \
  test/SWatch_Test/SWatch_Test.c src/watch/swatch.c -o swatch_test && ./swatch_test

python -m ziglang cc -Wall -Wextra -std=c99 -g -Iinc/bits \
  test/SBits_Test/SBits_Test.c src/bits/sbits.c -o sbits_test && ./sbits_test

python -m ziglang cc -Wall -Wextra -std=c99 -g -Iinc/signal \
  test/SSignal_Test/SSignal_Test.c src/signal/ssignal.c -o ssignal_test && ./ssignal_test

python -m ziglang cc -Wall -Wextra -std=c99 -g -Iinc/e2e \
  test/SE2e_Test/SE2e_Test.c src/e2e/se2e.c -o se2e_test && ./se2e_test

python -m ziglang cc -Wall -Wextra -std=c99 -g -Iinc/log \
  test/SLog_Test/SLog_Test.c src/log/slog.c -o slog_test && ./slog_test

python -m ziglang cc -Wall -Wextra -std=c99 -g -Iinc/store \
  test/SStore_Test/SStore_Test.c src/store/sstore.c -o sstore_test && ./sstore_test

python -m ziglang cc -Wall -Wextra -std=c99 -g -Iinc/pid \
  test/SPid_Test/SPid_Test.c src/pid/spid.c -o spid_test && ./spid_test
```

Run the tests before claiming anything passes. Compiling is not passing.

A second host compiler is available: **MinGW-W64 gcc 14.2.0**, bundled with Code::Blocks at `C:\Program Files\CodeBlocks\MinGW\bin`. It is not on `PATH`; prefix the command instead:

```bash
PATH="/c/Program Files/CodeBlocks/MinGW/bin:$PATH" gcc --version
```

Use it. gcc and clang do not warn about the same things — gcc's `-Wextra` includes `-Wtype-limits`, which clang does not implement the same way, and that difference has already caught dead code in a generated test that clang passed silently. (There is a second `gcc.exe` at `C:\Program Files (x86)\STMicroelectronics\...\MinGW`; it is version 4.5.0 from 2010 and does not support C99. Ignore it.)

A third toolset is installed through winget as **WinLibs**: gcc 16.1.0,
**cppcheck 2.21.0** and **doxygen 1.17.0**, in

```
C:\Users\engin\AppData\Local\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64\bin
```

It is not on `PATH` either. Until 04/11/2026 this file said cppcheck and
doxygen did not exist here; they did, and the first run of each found
something (see "Static analysis and MISRA" below). gcc 16 is clean under
the strict warning set and under `-fanalyzer`, with a control for each.

**cppcheck's `--addon=misra` silently does nothing**, on this machine and on
the CI runner alike. It prints `Checking ...`, reports no violation and
returns zero for every module, including a control file built to break five
rules. `tools/misra.sh` runs the addon by hand through a dump file instead,
with the control first; use it rather than `--addon`.

**gcc has no AddressSanitizer either.** MinGW-W64 ships no ASan runtime, so the control does not link. Neither host compiler on this machine can run ASan; the Ubuntu CI runner is still the only place it works, and the guard-page harnesses are still how out-of-bounds reads get proven here.

All nineteen modules are clean under a much stricter warning set than the project normally uses, confirmed independently by clang 21 via zig **and** gcc 14.2.0:

```bash
-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion -Wcast-qual \
-Wcast-align -Wstrict-prototypes -Wmissing-prototypes -Wredundant-decls \
-Wundef -Wwrite-strings
```

Zero warnings across all nineteen. Confirm the flags are live before trusting that — a control with an implicit narrowing conversion produces two warnings under the same command line.

A suite that passes on the first run has not yet been shown to check
anything. Mutate the module — flip a bounds test to off by one, delete an
overflow guard, disable an overlap check — rebuild against the mutant and
confirm the suite goes red.

```bash
PATH="/c/Program Files/CodeBlocks/MinGW/bin:$PATH" UBSANCC="python -m ziglang cc" \
  python tools/mutate.py            # every module
python tools/mutate.py svote        # one of them
python tools/mutate.py --list
```

**321 mutants across the nineteen modules: 300 killed, 21 equivalent, all
accounted for.** The defects live in `tools/mutants/<module>.py`, one file
per module, and the runner is `tools/mutate.py`. Before that they were
throwaway scripts in a scratch directory and the numbers in this file were
an assertion nobody could check; the ones above are a measurement anybody
can repeat.

| module | killed | equivalent | module | killed | equivalent |
|---|---|---|---|---|---|
| `sarray` | 19 | 6 | `sring` | 13 | 1 |
| `sbits` | 24 | 0 | `sscale` | 28 | 2 |
| `sdiag` | 12 | 1 | `sstate` | 22 | 2 |
| `sfault` | 16 | 1 | `sstring` | 17 | 4 |
| `sfilter` | 14 | 1 | `svote` | 16 | 0 |
| `sfixed` | 10 | 0 | `swatch` | 17 | 0 |
| `smath` | 16 | 0 | `smemory` | 6 | 3 |
| `ssignal` | 13 | 0 | `se2e` | 12 | 0 |
| `slog` | 18 | 0 | `sstore` | 12 | 0 |
| `spid` | 15 | 0 | | | |

`sarray`, `smath` and `sstring` were the thinnest measurement here for a
while: five, eight and seven real mutants against ninety two, sixty eight
and forty one functions, where the modules written last carry one and a
half per function. They are also the three most likely to be copied out.
Deepening them roughly doubled each and found a real hole immediately:
**`sarrayCount` had never been asked to count a match at the first
element**, so a scan starting at one instead of zero answered correctly
every time. The generated suite now counts one there.

The runner separates three outcomes, and the third is why it is a tool
rather than a number:

- **killed** — the suite went red, which is what was wanted.
- **SURVIVED** — the suite passed against a broken module. A hole, unless
  the mutant carries an equivalence argument.
- **RESURRECTED** — a mutant marked equivalent was killed. The argument
  written next to it is wrong, or the suite grew a case that distinguishes
  what was claimed indistinguishable. Either way the note has to change.

An equivalent mutant carries the reason it cannot be killed, and the runner
**requires it to survive**. That turns each equivalence claim into
something the tool checks rather than something a comment asserts.

Writing the tool found four things the numbers had been hiding:

- **`sstring` had never been mutation tested at all.** It is the reference
  module, 41 functions and 478 cases, and it was simply missing from the
  list.
- **`sring`'s ordering invariant was never tested** — the whole memory
  barrier argument. Publishing the index before writing the byte passed
  every case in the suite, because a single-threaded test that puts and
  then gets sees the same ring whichever order the two stores happened in.
  Only the barrier sits between them, so only the barrier can see it: the
  suite now installs a barrier that inspects the ring as it fires and
  asserts the byte is already there and the index has not moved. 162 cases
  became 172 and two mutants died.
- **A whole class of over-read mutant survives every portable suite.**
  `sstring` M2, M5 and M6, `smemory` M5 and `sarray` M4 all read past the
  end of a buffer, which on a host reads bytes that are there and answers
  correctly. They are killed by `test/harness/run.sh` and nothing else.
- **`sarray`'s first mutant set was five sixths equivalent**, which
  measures nothing. It had been aimed at defensive code no caller can
  reach. It now targets the operations: the sort, the rotation, the binary
  search, insertion and removal.

`tools/mutate.py` suppresses the Windows crash dialog and treats a suite
that does not finish as killed. Without the first, the run stops dead on
the mutant that removes the guard against `INT32_MIN / -1`: the SIGFPE
raises a dialog and waits for a mouse.

One `svote` mutant is worth knowing about because of *how* it dies. Forming the channel spread as `highest - lowest` in 32 bits rather than 64 produces the same bits on this host, so every assertion still passes — but it is signed overflow, and **UBSan traps it**. The suite is run under `-fsanitize=undefined -fsanitize-trap=undefined` by `tools/run_all.sh` and by CI, so the sanitizer is part of the kill criterion and not an extra. A mutant that only the sanitizer catches is still killed; one that nothing catches is a hole.

The `sring` run is the one that shows mutation testing paying for itself directly. A mutant that made `sringPutBlocku8` compute one byte too much free space passed the whole suite, because no case put *exactly* the usable size through the block form. That mutant is a real bug: it fills the buffer completely, which makes the two indices equal, which is the encoding for empty — so the ring would report holding nothing straight after being handed a full load. The boundary cases that kill it were written because the mutant survived, not the other way round.

Results from the mutation runs that are worth keeping in mind:

- Masking `smemoryEqualSecure` down to the low bit of each difference still passes every naive equality case. Only the deliberate high-bit-only case catches it.
- **`sdiag`'s two March memory tests still cannot have their failure paths
  reached by anything portable.** See "Untestable here" below, and
  `test/harness/aliasing.c`, which reaches four of the twenty two lines.
- **One `sscale` mutant survives and is provably equivalent.** Removing
  the up-front refusal of a flat first pair and widening the direction
  test from `>` to `>=` changes nothing observable: the only table it
  affects is one where `x[1] == x[0]`, and the validation loop's first
  iteration refuses that in either direction, leaving the same status
  and an equally untouched driver. That is a proof, not a hole, and it
  is the reason the explicit `==` check is in the source anyway — the
  code should say what it means rather than be right by accident.
- **One `sfault` mutant survives and is the same equivalent class as `sring` and `sscale`.** Its `isReady` tests both limits for zero, and either test alone catches a zeroed driver, so dropping one changes nothing reachable through the API. A driver with one limit set and the other zero can only be built by writing the struct by hand.
- **One `sdiag` mutant survives and is known to.** M8 reads the shadow copy through an ordinary pointer instead of a `volatile` one. The qualifier stops the compiler folding the two reads, and whether it did cannot be observed from inside the same program; only the generated code shows it. This line used to describe two survivors in the March tests, from a mutant set that no longer exists: the March failure paths are not mutated at all, because nothing portable reaches them. See "Untestable here" below.

`tools/coverage.sh` measures statement and branch coverage with gcc's
`--coverage` and gcov, and MC/DC as well when the compiler is gcc 14 or later
(`-fcondition-coverage`). The MinGW gcc 14.2 here has it; CI installs gcc-14
for the job. Mutation testing samples; coverage counts, and the two
answer different questions. The first run said so plainly: `sscale`, at the
time the most heavily tested module in the library — 223 cases and 17 mutants
— had never taken ten percent of its own branches.

```bash
PATH="/c/Program Files/CodeBlocks/MinGW/bin:$PATH" bash tools/coverage.sh gcc
```

What that first run found, and what closing it cost:

| module | branches taken, before | now | lines now |
|---|---|---|---|
| `sarray` | 79.4% | 98.9% | 99.3% |
| `smemory` | 81.7% | 98.7% | 99.4% |
| `sdiag` | 82.2% | 87.8% | 92.2% |
| `sstring` | 83.0% | 99.4% | 99.8% |
| `sfixed` | 86.0% | 97.4% | 100% |
| `sscale` | 89.9% | 97.9% | 99.2% |
| `smath` | 91.3% | 97.9% | 98.9% |
| `sring` | 91.5% | 99.1% | 99.5% |
| `sfilter` | 92.3% | 99.3% | 100% |
| `sbits` | — | **100%** | 100% |
| `sfault` | — | 99.3% | 100% |
| `sstate` | — | 97.7% | 99.5% |
| `svote` | — | **100%** | 100% |
| `swatch` | — | **100%** | 100% |
| `ssignal` | — | **100%** | 100% |
| `se2e` | — | **100%** | 100% |
| `slog` | — | **100%** | 100% |
| `sstore` | — | **100%** | 100% |
| `spid` | — | **100%** | 100% |

The "now" column is from 07/11/2026, after the cases `tools/tracecheck.py`
and the MC/DC classification asked for. MC/DC on the same run equals branch
coverage in every module but `sdiag` (87.2%); its gaps are written to
`<module>.conditions.txt`, and **every one of them, in all nineteen modules,
has been classified** — and the classification is checked, not written
down: each of the 64 unreachable ones is a record in
`tools/mcdc_exceptions.tsv` with its reason, keyed by function and line hash
like the 10.4 manifest, and `tools/mcdccheck.py`, which `coverage.sh` runs
after measuring, fails on a gap no record explains and on a record no gap
needs. A percentage is never the gate. 128 condition outcomes were cases nobody had written, and they
are in the suites now. Most were one argument of a guard that had only been
refused alongside another — `( dest == NULL ) || ( src == NULL )` tested
only with `dest` NULL, a capacity pair tested only on its first half — so a
function that checked one pointer, or one capacity, would have passed. Nine
mutants (`sarray` M23–M25, `smath` M16, `sstring` M20–M21, `smemory` M9,
`sscale` M19, `sfilter` M15) pass the suites as they were and die only to
the new cases. The ones worth remembering:

- **`smathInRange` had never been asked about a value below its lower
  bound**, in any of the four families, nor about one on it. A version that
  tested only `value <= high` passed every case.
- **`sarray`'s overlap test had only ever seen two ranges miss each other one
  way round.** Dropping the `aStart < bEnd` half passed; copying between the
  two halves of one array, in both directions, now kills it.
- **`sscaleLinear` and `sscaleLinearClamped` had never been given a falling
  map on its far side or inside it**, and `sscaleApplyClamped` had never
  been given an in-domain reading on a descending table.
- `sstring`'s character classes had never met the characters just past
  their ends (`[`, `{`, DEL, `:`, `@`), nor a vertical tab or a form feed.

What remains is unreachable, listed below with the branch residue; the
masking MC/DC gcc measures adds only these to it: `sfixedLerp` and
`sfixedSqrt` range checks that cannot fail (an interpolation lies between
its ends, and a root of a Q16.16 value fits), `sscale`'s
`spanProductFits` zero input span, which validation has already refused,
and `sstring`'s `sourceLength` asked to scan zero bytes, which no caller
can pass.

The "now" column includes the integration suite, which `coverage.sh` links
against thirteen of the modules and counts. That is what took `swatch` to every
branch: a chain exercises paths a module's own suite
has no reason to reach.

Four of the holes were ones a broken module would have survived, and they are
the reason to measure rather than assume:

- **`sarrayMax` never took the branch that replaces the running best**, for
  three of the four families. The shared template put the largest element at
  index zero, so a Max that read only `arr[0]` would have passed. The `i32`
  family had a case with the maximum elsewhere and the unsigned ones did not.
- **`sarrayCompare` and `smemoryCompare` only ever answered "a sorts first".**
  Both the element comparison and the length tie break had one arm untaken, so
  a Compare that answered −1 to every difference would have passed.
- **`sstringIsAlpha`, `sstringIsAlphaNumeric` and `sstringIsHex` had only been
  asked about strings that satisfy them.** A version answering TRUE to
  everything would have passed. Their empty-string branch was a second,
  separate hole.
- **`sfilterDebounceInit` and `sfilterHystInit` had only ever been given a
  starting state of FALSE**, so a version that ignored the argument and always
  started low would have passed.

**A percentage is not the result.** Every uncovered line is either a case
nobody wrote, which is a hole, or a defensive branch the API cannot reach,
which is a comment. What remains is all of the second kind:

- `sarray`, `smemory`, `sstring`: `isOverlapping`'s address-wrap guards, which
  need a buffer at the very top of the address space; `spanBytes`, which needs
  an element count whose byte span passes 2^32.
- `smath`: the unsigned saturating forms saturating toward the end they cannot
  reach — the same impossibility recorded in their `@return` blocks.
- `sring`, `sscale`, `sfault`, `sstate`, `sfilter`: the redundant half of
  `isReady` (inline in `sfilterAvgAdd`), and `sscale`'s `mapReady` beyond
  the conditions the suite reaches with drivers damaged by hand. Either condition alone catches a
  zeroed driver, which is why both survive mutation. `sstateIsReachable`'s
  pass bound, which a breadth first walk always finishes inside.
- `sscale`: `SC_INVALIDTABLE` out of `FindSegment`, and the search's own
  exhaustion and before-the-first-segment exits. A validated table
  cannot produce them, and neither did 50 million tables changed after Init.
  `Apply`, `ApplyClamped` and the degenerate-segment guard in `interpolate`
  used to be on this list; a table its caller rewrote after Init reaches all
  three, and the suite now does that.
- `sdiag`: the March test failure paths. That is the stuck-at fault limit
  recorded under "Untestable here", not a gap in the suite.

When writing a sweep against a wider-type oracle, **the oracle is where the bug will be.** The first `smath` run reported 32640 failures in unsigned subtract; the module was right and the oracle wrong, because `(uint32_t) a - b` wraps inside the oracle's own type and reads as an overflow when the truth is an underflow. 32640 is exactly the number of pairs with `a < b`. A failure count that equals a recognisable combinatorial quantity is a sign the oracle is broken, not the module.

Undefined-behaviour sanitizer, in trap mode so it needs no runtime:

```bash
python -m ziglang cc -Wall -Wextra -std=c99 -g -fsanitize=undefined -fsanitize-trap=undefined \
  -Iinc/string test/SString_Test/SString_Test.c src/string/sstring.c -o sstring_ubsan && ./sstring_ubsan
```

**AddressSanitizer is unusable under zig on this Windows target, and it fails in the more dangerous of the two possible ways.**

- `-fsanitize=address` alone fails honestly: `lld-link: error: undefined symbol: __asan_report_store4`, and no binary is produced.
- `-fsanitize=address,undefined` **links cleanly and then detects nothing.** A control program that reads seven elements past a four element `malloc` runs to completion, prints garbage and exits 0. Only the UBSan runtime is really there.

So a suite built with `address,undefined` will report a confident pass having been checked by nothing at all. **Never report an ASan result without first running a control that must fault**; `scratchpad/run_gcc.sh` does this and skips the whole section when the control survives. The same trap already produced one false "ASan clean" claim in this repo's history.

`-fsanitize=undefined -fsanitize-trap=undefined` needs no runtime and genuinely works. It is the only sanitizer worth running here. For an out-of-bounds *read*, which ASan would normally catch, use a guard page instead: place the buffer at the end of a `VirtualAlloc` page and mark the next page `PAGE_NOACCESS`, so an over-read faults. That is how the source-scan overrun in `sstringCopy` was proven before it was fixed.

Always pair it with a negative control: declare a capacity one element larger than the buffer really is and confirm that call faults. Without it, "no fault" may only mean the guard page was never armed. The `sarray` harness reports `EXIT=0` on the honest capacities and `EXIT=139` on the over-declared one.

The same idea covers `sdiag`'s memory tests, which a portable test cannot reach at all: map one 64 KiB section at two adjacent addresses with `CreateFileMapping` plus two `MapViewOfFileEx` calls, and the second half of the region genuinely *is* the first half. That is a real address decoder fault. `sdiagRamTestDestructive` reports `SD_FAILED` at the first word of the second view, `sdiagRamTestNonDestructive` returns `SD_OK` exactly as its documentation says it will, and a healthy region of the same size passes both. Prove the aliasing is real before trusting the result — write two different values and check the first read back as the second.

## Untestable here

Three things cannot be verified on this machine, and no claim should be made about them until they are:

- **Stuck-at memory faults.** Nothing portable can produce a cell that accepts a write and returns something else, so most of the read checks inside the March elements are covered by review only.

  `test/harness/aliasing.c` closes part of it. One section object mapped twice at adjacent addresses with `MapViewOfFileEx` is a real address decoder fault: the second half of the region genuinely *is* the first half. `sdiagRamTestDestructive` reports `SD_FAILED` at index 16384, the first word of the second view; `sdiagRamTestNonDestructive` reports `SD_OK`, exactly as its own note says it must, because it restores every word before moving on and so never has two different values live at once; and a healthy region of the same size passes both.

  That is **four of the twenty two uncovered lines**, and it is worth being precise about why it is not more. One March element catches the aliasing, so only that element's failure path runs. The rest need a cell that accepts a write and returns something else, and nothing on this machine can make one. `sdiag` goes from 91.0% to 92.7% of lines and 84.2% to 89.5% of branches with the harness included.
- **Execution on ARM. This one is permanent, not pending.** There is no target and no emulator, and there will not be one. Every module cross-compiles for `arm-none-eabi` and passes `-fanalyzer`, which says the code builds for the target, not that it behaves there. Everything the test suites prove, they prove on x86-64 hosts.

  What that costs is narrow but real, and it should be stated rather than glossed. The suites cannot see: unaligned access faults (`sarray` and `sdiag` take typed pointers partly to make this the compiler's problem, but a caller can still hand over a misaligned buffer); anything that depends on the target's actual word size or padding; the memory-ordering assumption in `sring`, whose whole barrier argument is about a processor this code has never run on; and the real timing of `smemoryEqualSecure`, which is constant in comparison count but was never measured on a core with a cache.

  One thing about the target can be measured without running on it: stack.
  `tools/stack.py` reads the cross compiler's `-fstack-usage` and call graph
  for a Cortex-M0+ and a Cortex-M4, fails on a dynamic frame or recursion
  (there is neither), and reports each module's deepest chain — 200 bytes
  for `sscale`'s map on an M0+ is the largest. It also showed the compiler
  emitting `memset`, `memcpy` and `memmove` calls that no source asked for,
  which a freestanding target must provide and every ARM C library does.

  Do not write "verified on ARM" anywhere. An integrator putting this on a target owns that validation, and the honest claim is that the logic is verified on a host and the code builds clean for the target.
- **The CI logs.** There is no `gh`, and the public API refuses job logs and artifacts without admin rights. A job's *conclusion* is public, which is why every reporting job now fails on a finding instead of printing it: the verdict is then readable without the log.

## Static analysis and MISRA

cppcheck's general checks are clean with three style checks suppressed,
because each contradicts the convention rather than finds anything:
`variableScope` (locals are declared at the top of a function),
`unreadVariable` (locals are initialised where they are declared) and
`staticFunction` (a public function the module also calls itself). One
finding is suppressed inline in `sdiag.c`: cppcheck does not model
`volatile`, so it reads the March test's read back as always false.

The first real MISRA run, on 04/11/2026, reported 737 hits in eight rules,
all in the 10.x and 11.x families, and **one of them was a real defect**:
`sfilterEmaInit` scaled its initial value with `( int64_t ) initial << shift`,
which is undefined behaviour for a negative value. No case had ever started
the filter below zero. UBSan traps it, the suite now starts it at -500,
`INT32_MIN` and `INT32_MAX`, and `sfilter` M11 puts the shift back to prove
the suite sees it.

The rest was settled on 05/11/2026. Twenty six hits were fixed in the code
and four rules are recorded as deviations:

| rule | hits | modules | decision |
|---|---|---|---|
| 10.4 | 420 (636 at baseline) | all | **deviation, narrowed on 07/11/2026.** A status, state or flag is a `uint8_t` compared with the module's enum members and with `TRUE`/`FALSE`; that is all the deviation covers. The 277 hits that compared or combined an unsigned value with an unsuffixed constant were fixed with a `u` suffix. `tools/misra_occurrences.py` checks every remaining hit by expression: the operator the addon reports must be `==` or `!=` with `TRUE`, `FALSE` or one of the module's own enum members as an operand, or the run fails. All 420 are |
| 11.5 | 26 | `smemory`, `sdiag` | **deviation.** `void*` to `unsigned char*`, the one conversion the standard always allows for reading bytes |
| 11.4 | 10 | `sarray`, `sstring` | **deviation.** Pointer to integer, to test two ranges for overlap and to choose a copy direction |
| 11.6 | 10 | `sarray`, `smemory`, `sstring` | **deviation.** The same overlap checks, through `void*` |
| 10.8 | 10 | `sfixed`, `smath`, `sstring`, `svote` | fixed: the value is formed in a local and the local converted, and `INT32_MIN` is spelt `-INT32_MAX - 1` |
| 10.1 | 9 | `sbits`, `sfilter`, `sfixed` | fixed: no signed value is shifted anywhere in the library now |
| 10.2 | 4 | `sstring` | fixed: a digit is added to a character constant as an unsigned value |
| 10.7 | 3 | `sbits` | fixed with the 10.1 change |

Module-scoped deviations are written in the banner of each module they cover,
so they travel with a copied module. Rule 10.4 occurrences are pinned in
`tools/misra_10_4.tsv` by module, enclosing function, line hash and duplicate count;
the manifest is a checker baseline, not formal deviation approval.
`template/src/generic.c` carries the 10.4 rationale for new modules. Two old
records were wrong and were caught by the run: `sstring` claimed its pointer
conversion was confined to one
function when three use it, and `smemory` recorded Rule 11.4 for what is a
Rule 11.6 conversion.

**`sfilter` no longer shifts a negative value right.** The exponential
filter's accumulator is brought back down by `shiftDown`, which shifts the
magnitude unsigned and puts the sign back, rounding toward minus infinity
exactly as the arithmetic shift did. It was compared with the host's shift
over 34 million values including both ends of `int64_t`. Writing it found
that the suite had never pinned the rounding direction — `sfilter` M12,
rounding toward zero instead, survived until a case stepped the filter by
-1 and by -17 — and that the old `@note` claiming truncation would stall
the filter was not what the suite showed. M13 puts the signed shift back
and is equivalent by construction: on every compiler the library is built
with, the shift is arithmetic, so only MISRA can see it.

```bash
bash tools/misra.sh                 # every module, exits non zero on a finding
bash tools/misra.sh sfilter svote   # some of them
bash tools/misra.sh --annotate      # what CI runs
```

`tools/misra.sh` runs the addon through `--dump`, runs the control first
and exits 2 if the addon is not live, applies the deviation table, and over
the whole tree also fails on **a deviation nothing needs any more** — a
recorded exception the code no longer takes is a claim that has gone stale.
It was checked against a copy of the tree with one deviation removed, one
added that nothing uses and one octal constant injected, and reported all
three. Put WinLibs on `PATH` to run it here.

Static analysis, using the analyzer built into the ARM compiler:

```bash
arm-none-eabi-gcc -c -Wall -Wextra -Wpedantic -std=c99 -fanalyzer -Iinc/string src/string/sstring.c -o /dev/null
```

**`-o /dev/null` leaves a real file called `nul` in the repo root.** Both `arm-none-eabi-gcc` and `zig cc` are native Windows binaries, so neither understands the Git Bash path and both write a literal `nul` instead. `nul` is a reserved device name and `git add` on it fails outright with `fatal: mmap failed: Invalid argument`, which looks like repository corruption and is not. It is in `.gitignore` now, but **write the object to a scratchpad path and delete it** rather than generating it at all.

Cleaning it up needs care, because the ordinary ways of checking for it lie:

- `ls -la nul` in Git Bash prints a size even when nothing is there, and `Test-Path -LiteralPath "\\?\...\nul"` returns false even when the file *is* there. Use `Get-ChildItem <repo> -Force | Where-Object { $_.Name -like "nul*" }` — that one is accurate.
- Deleting needs the `\\?\` prefix: `Remove-Item -LiteralPath "\\?\C:\...\nul" -Force`. Verify with `Get-ChildItem` afterwards; a silent success from `Remove-Item` is not proof.

## CI

`.github/workflows/ci.yml` runs on every push and pull request to `main`. It is split into two tiers on purpose:

- **Blocking** — `test` (gcc and clang, builds and runs the suite), `sanitizers` (ASan and UBSan, which is where AddressSanitizer actually works), `cross-compile` (arm-none-eabi build, `-fanalyzer`, and the header coexistence check). Every one of these was verified locally before the workflow was written, so a red result is a real regression.

**No list of modules is kept anywhere.** Since 07/11/2026 the CI test and
sanitizer jobs, the integration build, the cppcheck include path,
`run_all.sh` and `coverage.sh` all find the modules from `src/*/*.c`, and a
suite at `test/S<Module>_Test/`, the module name with its second letter
capitalised. Before that every new module meant editing five lists, and a
missed one was a module silently not tested. `misra.sh`, `size.sh`,
`stack.py` and the checkers already worked this way.
- **Also blocking since 05/11/2026** — `cppcheck` (general checks and MISRA through `tools/misra.sh`), `host-analyzer` (gcc `-fanalyzer -Werror`) and `docs` (doxygen, warning free). **Until 04/11/2026 these three were reporting jobs that were green whatever they found**: cppcheck piped into `tee`, doxygen only printed its warnings, and the analyzer had no `-Werror`, so no green result from before then proves anything. Each now exits non zero on a finding, each runs a control that must produce one first, and each was flipped only after a CI run of it came back clean. Their findings are printed as annotations, which the public API serves without admin rights, so a failure can be read from this machine.
- **Also blocking since 07/11/2026** — `mutation` (every mutant killed or argued equivalent), `coverage` (gated by `tools/mcdccheck.py`: every MC/DC gap a reviewed record, never a percentage) and the stack step of `cross-compile` (`tools/stack.py`: no recursion, no dynamic frame). Each was flipped after CI runs of it came back clean.
- **Reporting** (`continue-on-error: true`) — only `size` now. What counts as too big is the integrator's budget, not a number this repository should choose; it still fails on a `.data` or `.bss` that is not zero. **Flip a reporting job to blocking only once a CI run of it comes back clean**, and read that from the job conclusion, not from the absence of a red workflow.

Every job runs on `ubuntu-24.04`, not `ubuntu-latest`, since 07/11/2026. The
MISRA check pins each Rule 10.4 finding, and the cppcheck producing them
comes from the image's package archive, so an image change would swap the
analyser under a green check. Move the image deliberately, in a commit of
its own, and read what the MISRA job says about it.

The MISRA addon prints rule numbers without descriptions, because the rule texts are copyrighted and cannot ship. `misra-c2012-11.4` is expected: it is the pointer-to-integer conversion in `isOverlapping`, already recorded as a deviation in the `sstring.c` banner.

Syntax and warning check for one module, and for the whole tree:

```bash
arm-none-eabi-gcc -c -Wall -Wextra -Iinc/<domain> src/<domain>/<name>.c -o /dev/null

for f in src/*/*.c; do d=$(basename $(dirname "$f")); \
  arm-none-eabi-gcc -c -Wall -Wextra -I"inc/$d" "$f" -o /dev/null; done
```

Because every header must be independently includable, also check that they all coexist in one translation unit — this catches duplicate include guards and clashing typedefs:

```bash
for h in inc/*/*.h; do echo "#include \"$(basename $h)\""; done > /tmp/allhdr.c
echo "int main(void){return 0;}" >> /tmp/allhdr.c
arm-none-eabi-gcc -c -Wall $(for d in inc/*/; do echo -n " -I$d"; done) /tmp/allhdr.c -o /dev/null
```

`Doxyfile` produced sixty warnings on its first run, every one an undocumented struct or member: headers stay pure declarations, and nothing documented the structs anywhere else. They are now documented in the paired `.c` with a `@struct` block carrying one `@var` per member, the run is warning free, and `tools/doxcheck.py` enforces the same rule so it holds without doxygen. `template/src/generic.c` shows the form. **Never write `@var` or `@struct` in a banner history line** — doxygen parses it as a command and reports a symbol that does not exist.

`tools/apicheck.py` is the other half of the same idea and checks the
declarations rather than the comments. Seven rules, and the first is the one
this library is arranged around: **every pointer to a buffer is immediately
followed by that buffer's capacity**. That is the rule the guard-page defect
was about, and until this existed nothing checked that the fix stayed
applied. It also checks that a size named `<x>Size` sits next to `<x>`, that
every public function returns `uint8_t`, that names carry their module's
prefix, that enum members carry their module's status prefix, and that no
prototype names a bare `int`. 399 declarations, no findings, which is every
prototype in the tree: **until 07/11/2026 the tool skipped any prototype
wrapped onto a second line**, because its pattern stopped at a line end.
`sstringToken` had never been checked, nor the longest declaration of each
module added that day; the count of 357 against 358 functions was the
only sign. A control with a wrapped prototype missing its capacity now
fires. A pointer to a
module's own driver struct is exempt, and those types are read out of each
header's typedefs so a new module needs no change to the tool.

`tools/doxcheck.py` covers more than doxygen does. It checks every `.c` in the tree — 828 functions, statics and test helpers included — for banner completeness, a `/**` block on every function, `@param` lists that match the signature *in both directions*, `[in]` on every read-only parameter, `@return` present exactly when the return type is not void, unknown tags, and a `@struct` block with exactly one `@var` per member for every struct a header declares. It is a blocking CI job because it runs clean here.

**Its most useful rule is one doxygen has no concept of: a `@return` must name every status the body can set, and no status it cannot.** That is what found the only real defect the first full audit turned up. The four `smath` families share one template, so the unsigned ones had inherited the signed text and promised an `SH_UNDERFLOW` that an unsigned addition cannot produce and an `SH_OVERFLOW` that an unsigned subtraction cannot — a caller testing for either was writing a branch that never runs.

Two patterns it accepts on purpose, because both are better documentation than the alternative:

- **`"otherwise the status sarrayCopyNu8 reports"`** documents the delegate's statuses. Copying the list would drift.
- **A status the body can form but can never produce may be explained in a `@note`** rather than listed in `@return`. `sfixedFloor` calls a range check that can say `SX_OVERFLOW` and `SX_UNDERFLOW`, and rounding down reaches neither. The audit added those notes rather than removing the checks.

The same rule applies to it as to the test suites: a checker that passes on its first run has been shown to check nothing. It was developed against a control file carrying one deliberate defect per rule, and every rule fired. Do that again before trusting a change to it.

`tools/tracecheck.py` asks the question neither of those does: **for each
status a function's `@return` promises, where is the case in its own suite
that expects it?** A statement that calls the function and names the status
counts, and so does a status parked in a local and compared a few
statements later, which is how the sweeps are written. A status no case can
expect goes in `tools/trace_exceptions.tsv` with its reason, and an
exception that a case now covers fails the check as stale. It is a blocking
CI step and runs from `run_all.sh`.

Its first run, on 07/11/2026, reported 106 gaps across 358 functions, and
two of its findings matter beyond the count:

- **`sstring` had never handed an unterminated source to twenty one
  functions that promise to refuse one**, nor a zero capacity to seventeen.
  Those refusals are what the reference module exists for. `sstring` M19
  now reads an unterminated number to the end of its capacity, and only the
  new cases kill it.
- **`sfixedFloor` and `sfixedRound` promised an `SX_UNDERFLOW` no input can
  produce.** All 2^32 inputs were run: the smallest value of the type is a
  whole number, so nothing rounds below it. The same class of defect
  `doxcheck.py` found in `smath`, and the same fix: the promise moved to a
  `@note`.

It also found `sfilter` throwing away seven success statuses with
`( void )`, and that `sscaleApply` and `sscaleApplyClamped` refuse a table
the caller rewrote after Init without any case ever having asked them to.
`sfilter` M14 and `sscale` M18 die only to the new cases. Ten exceptions
remain: delegated statuses a wrapper cannot reach, the March test failures
only the aliasing harness reaches, and `sscaleFindSegment`'s defensive
refusal.

**The suite directory's name is derived, and its case matters.** Every tool
finds a module's suite at `test/<S + module[1].upper() + rest>_Test/`, so
`se2e` lives in `test/SE2e_Test/`, not `SE2E_Test`. This machine's file
system ignores case and found the wrong name; the Linux runner did not, and
the first `se2e` commit went red in CI only. `tracecheck.py` now matches
every path component's case exactly, with a control for it, so the same
mistake fails here too.

## Layout and module contract

```
inc/<domain>/<name>.h   ←→   src/<domain>/<name>.c    strict 1:1 pair
test/<Name>_Test/<Name>_Test.c                        self-checking test main
template/inc/generic.h, template/src/generic.c        copy these to start a new module
```

Domains: `array`, `bits`, `diag`, `e2e`, `fault`, `filter`, `fixed`, `log`, `math`, `memory`, `pid`, `ring`, `scale`, `signal`, `store`, `state`, `string`, `vote`, `watch`.

**A module is named after its domain directory with an `s` in front, without
exception.** `array`/`sarray`, `diag`/`sdiag`, `math`/`smath`, and so on. Two
modules used to break the rule and were renamed on 05/08/2026:
`basicmathsafe` became `smath` and `selfdiagsafe` became `sdiag`, whose
directory `selfdiag` became `diag` at the same time. Nothing outside this
repository used either name, so the rename cost one commit.

The status enum is a `SCREAMING_CASE` tag from the module name with
two-letter prefixed members, and **the prefixes are a namespace with no
allocator, so check this table before adding a module** — every header has
to compile in one translation unit and CI checks it:

| Prefix | Module | Prefix | Module |
|---|---|---|---|
| `SA_` | `sarray` | `SR_` | `sring` |
| `SC_` | `sscale` | `SS_` | `sstring` |
| `SD_` | `sdiag` | `SX_` | `sfixed` (`SF_` was taken) |
| `SF_` | `sfilter` | `SH_` | `smath` (`SM_` was taken) |
| `SM_` | `smemory` | `SV_` | `svote` |
| `SU_` | `sfault` (`SF_` was taken) | `ST_` | `sstate` |
| `SW_` | `swatch` | `SB_` | `sbits` |
| `SG_` | `ssignal` (`SS_` was taken) | `SE_` | `se2e` |
| `SL_` | `slog` | `SO_` | `sstore` (`SS_` and `ST_` were taken) |
| `SP_` | `spid` | | |

Two prefixes could not be the obvious abbreviation. `sfixed` takes the `X` of
fi**x**ed because `sfilter` already held `SF_`, and `smath` takes the `H` of
mat**h** because `smemory` already held `SM_`. `SM_` was deliberately left
with `smemory` rather than moved: renaming a shipped module's prefix to free
a letter is churn paid by every existing call site.

All nineteen modules are implemented. `sstring` is the reference: 41 functions covering length, copy, move, concatenate, compare, clear, search, tokenize, transform, validate and number conversion. Its design is written up in `docs/superpowers/specs/2026-08-02-sstring-design.md`. **Do not write further spec documents or implementation plans for this repo** — the owner wants the design agreed in chat and then implemented directly.

`sarray` is 92 functions: twenty three operations repeated across four element families, `uint8_t`, `uint16_t`, `uint32_t` and `int32_t`. Two things about it differ from `sstring` and will bite if forgotten:

- **Every size and count in `sarray` is an element count, not a byte count.** `arrSize` of 8 on a `uint32_t` array is 32 bytes. Only the two static helpers that feed `isOverlapping` deal in bytes, and `spanBytes` guards that multiply against wrapping.
- **The four families are mechanically identical modulo the element type.** They are emitted from `tools/gen_sarray.py`, not typed four times. Edit the generator and re-run it; **never edit `src/array/sarray.c` or `inc/array/sarray.h` directly**, because the next generator run silently reverts the change. The same applies to `smath` and to both generated test suites. CI runs all four generators and fails on a non-empty `git diff`, so drift is caught rather than discovered later. See `tools/README.md`.

`sarray` has no `Get`/`Set` equivalent in `sstring` because C already has `arr[i]`; `sarrayGet` exists to be the bounds checked form of it. `sarrayBinarySearch` requires a sorted array and does not verify it, because verifying costs the scan the search exists to avoid — `sarrayIsSorted` is the separate precondition check.

`sfilter` is 19 functions: moving average, exponential moving average, debounce, slew rate limit, hysteresis and median. **Every sample in it is an `int32_t`, whatever the sensor produced** — a filter has to subtract (the window's oldest sample, the filter's own output, the current value from the target), unsigned subtraction across zero is where those bugs live, and an `int32_t` holds every `uint8_t` and `uint16_t` reading exactly. That single choice is why the module needs no generator and stays hand-written.

Three things in it are easy to break and hard to notice:

- **The exponential filter keeps its accumulator scaled by `2^shift`.** Without that, the fraction discarded each step means the filter stops moving whenever the remaining difference is under `2^shift`, and sits at a permanent offset. The test drives a constant input and demands the output reach it *exactly*; that is the only case that catches it.
- **Two accumulators are `int64_t`** (`sfilteravg_t`, `sfilterema_t`). That is what removes the overflow question entirely rather than bounding it with a rule the caller has to remember. `sfilterSlewUpdate` also forms its distance in 64 bits, because `target - current` across the ends of `int32_t` is undefined behaviour.
- **The slew limiter's two limits are separate on purpose** and only distinguishable when the distance falls *between* them. A mutant that tested against the wrong limit survived the first suite for exactly that reason.

Unlike `sring`, these are **not** safe to share between an interrupt and the main loop. Every one of them reads and writes the same fields, so there is no lock-free split to exploit. Give each context its own filter.

`sfixed` is 19 functions of Q16.16 fixed point — an `int32_t` holding the real value times 65536, so sixteen bits of whole number and sixteen of fraction. It exists so a part without an FPU can do fractional arithmetic without float: software float costs a library call per operation and gives a *different answer* on a part that has an FPU, which is precisely what a safety argument cannot tolerate.

The format is fixed rather than a per-call parameter on purpose. A runtime fraction width means every caller has to keep two things in step, and mixing two widths in one expression is a silent wrong answer rather than a compile error.

Three rules hold it together:

- **There is no right shift of a signed value anywhere in the file.** C99 leaves that implementation-defined, and a fixed-point module is the last place to rely on it — half its values are negative and the sign is the whole point. Division is fully defined and truncates toward zero, which is what the rest of the library already does. `grep '>>' src/fixed/sfixed.c` should stay empty.
- **The multiply rescales down and the divide prescales up.** Both operands carry the factor of 65536; a product carries it twice and a quotient cancels it out. Forgetting either is the classic fixed-point bug and makes every answer wrong by 65536.
- **`sfixedSqrt` scales up before taking the root**, because a root carries only half the scaling. Skipping it gives an answer 256 times too small.

`sfixedToParts` splits a value into sign, whole part and thousandths so a caller can print it without this module knowing about strings. The sign is separate from the whole part deliberately: -0.5 has a whole part of zero, and a caller reading only the sign of that would print it as positive.

Its test suite contains **no floating point at all** — checking a fixed-point module against float would check it against the very thing it exists to avoid. Expected values are written as integers in the format, and the sweeps assert properties (multiplying by one is the identity, floor never exceeds the input, a root squared never exceeds the input) rather than comparing against a second implementation.

`sscale` is 15 functions of piecewise linear scaling — a breakpoint table from a datasheet, and everything between the breakpoints taken to be a straight line. Thermistor counts to degrees, load cell counts to newtons.

It is the second module built around the driver struct, and unlike `sring` the reason is safety rather than state. **`sscaleInit` is the only function that validates the table, and no other function can be reached without it having succeeded.** A caller cannot interpolate through an unchecked table, cannot swap the two arrays by accident, and cannot pass a count that disagrees with the one the table was validated against. That is worth more here than the parameter list it saves.

Four things in it are easy to get wrong:

- **Both directions of table are accepted.** A thermistor's resistance falls as its temperature rises, so its table descends. Forcing the caller to reverse the array by hand would put the one error this module exists to prevent back in the caller's code. Direction is taken from the first pair and every remaining pair must agree, so a table that turns round half way is refused rather than searched.
- **Interpolation rounds to nearest, halves away from zero**, where `sfixed` truncates. The difference is deliberate: truncation biases every reading toward zero by up to one count, always in the same direction, and on a calibration curve a bias is a systematic error rather than noise.
- **The overflow check lives in `Init`, not in `Apply`.** One segment's spans can multiply past `int64_t` — the boundary is an input span of 2^32-1 against an output span of 2^31, which fits, and one count more, which does not. Checking it once at startup makes a bad table a configuration error rather than a runtime failure found by whichever reading first happened to reach it, and it makes the arithmetic in `Apply` total.
- **The rounding is written with a remainder, not by adding half the denominator.** That addition would itself overflow when the numerator is near the top of the type. Twice a remainder cannot.

The `y` array is deliberately *not* required to be monotonic — a calibration curve may fold back and still be a function of its input. That is also why `sscaleInvert`, which builds the reverse map by exchanging the two arrays, can fail: the inverse of a folded curve is not a function. It is one call into `sscaleInit`, so the requirement is enforced by the same code that enforces it for a forward table.

**The two dimensional map**, added on 07/11/2026, is three more functions on its own driver, `sscalemap_t`: `sscaleMapInit`, `sscaleMapApply` and `sscaleMapApplyClamped`, bilinear interpolation over a grid of up to `SSCALE_MAPMAX` breakpoints per axis. Four things about it:

- **Both axes must increase.** A calibration tool writes its axes in order; a falling one would be a different table.
- **One division, one rounding.** The four corners are weighted by areas and divided once, to nearest with halves away from zero as the 1D table does. Interpolating along x and then along y would round twice and could be a count off. The suite checks every integer point of a cell with mixed corners against the exact rational answer, grouped the other way round.
- **Every cell is checked for overflow at Init**, as the 1D table's spans are: the largest corner times the cell's area must fit `int64_t`, and the suite pins both sides of that bound (an area of 2^32 - 1 against a corner of 2^31 fits, one row more does not). Apply checks the cell it uses again, so a map changed after Init reaches `SC_INVALIDTABLE` rather than undefined behaviour.
- **The segment search tests only each segment's upper end.** Given that the input is not below the first breakpoint, that is enough for any axis, a changed one included, so the search cannot fail. A value on an inner breakpoint goes to the segment below; `sscale` M30, which sends it above, is equivalent, because two cells sharing an edge agree on it exactly.

Its test suite contains **no floating point**, for the same reason `sfixed`'s does not. The rounding rule is verified by cross multiplication in `int64_t`: a result is correct to nearest when twice its error has a magnitude no greater than the denominator.

`svote` is 12 functions of redundant channel voting — the software side of the redundancy IEC 61508 counts as hardware fault tolerance and ISO 26262 asks for as a dual channel comparison. Agreement within a tolerance, majority with a caller-chosen quorum, median, mean, spread, an outlier bitmask and fail-safe selection.

**It is entirely stateless, and that line is the design.** A vote is a function of the readings in front of it. Deciding that a channel has disagreed often enough to be excluded is a different job with its own memory and belongs in a fault qualifier, not here. Keeping the vote pure is what makes it callable from any context and testable exhaustively.

Four things in it are easy to get wrong:

- **Every difference is formed in 64 bits.** Two channels at opposite ends of `int32_t` have a difference no `int32_t` can hold, and that pair is exactly what a voter exists to catch: one channel stuck at each rail. In 32 bits that subtraction wraps to a difference of one and reads as *perfect agreement*. The suite compares `INT32_MIN` against `INT32_MAX` for precisely this.
- **Agreement is not transitive.** With a tolerance of 10, readings of 0, 10 and 20 have both outer readings agreeing with the middle one and not with each other. `svoteAllAgree` compares every pair, not every reading against the first; the cheaper test reports agreement that is not there.
- **`svoteMajority` answers with one of the readings**, never an average of the agreeing group. A voted value no channel measured cannot be traced back to an input. A caller wanting the smoothed value takes the mean itself, having been told which channels agreed.
- **Nothing is sorted.** The readings are `const` and belong to the caller, so the median is found by rank — for each reading, how many lie below it and how many equal it. That removes both the scratch buffer and any question about whether the caller's array came back the way it went in. With an even count the *lower* middle is reported, because every answer the median gives is a reading some channel actually produced.

The tolerance is a distance and a negative one is refused rather than clamped to zero: it means the caller computed it and the computation went wrong. `SVOTE_MAXCHANNELS` is 32 because the outlier report is a bitmask in a `uint32_t`, and it is enforced by every function rather than only the one that needs it.

`svoteSelectLow` and `svoteSelectHigh` duplicate `sarrayMini32` and `sarrayMaxi32` on purpose. The boundary in this library is meaning, not code: picking the reading that errs in the safe direction is a policy decision, not a query about data, and a module is meant to be copied out on its own.

`sfault` is 12 functions of fault qualification — the diagnostic state machine that decides when a condition has been observed often enough to be called a fault, and when it has been absent long enough to be withdrawn. ISO 26262-7 asks for it; every safety ECU has one; it is the kind of thing that gets rewritten badly per project.

It counts cycles, not time, and the cycle is whatever the caller calls `Update` on. This library includes no vendor header and calls no HAL, so there is nothing here that could read a clock, and making the caller convert keeps the conversion in the code that knows the period.

Four things hold it together:

- **The counter is discarded the moment the verdict changes, not counted down.** A condition present two cycles in three must never accumulate toward confirmation — that is the difference between qualification and an average, and it is what stops a bad connector reading as a failed part. Note that the *observable* difference is only visible through `sfaultGetCounter` while the fault is absent, because the next present cycle would clear a stale count anyway. A mutant that dropped the reset survived until a case read the counter in exactly that window.
- **Latching is a separate decision from qualification**, fixed at Init. A latched fault never enters HEALING at all, so the state it reports cannot depend on how long ago the condition went away.
- **`sfaultClear` and `sfaultReset` are not the same thing.** Clear withdraws the fault and keeps the occurrence count, for the service action that acknowledges it. Reset returns the driver to what Init left, count included, for restarting a diagnostic. Offering only one would force a caller to lose the history in order to clear a fault.
- **Every counter saturates.** A qualification counter that wrapped would un-confirm a fault that had been present continuously, which is the one behaviour a fault qualifier must never have.

The four states matter and collapsing them to a flag loses the point. PENDING is *observed but not yet earned*; HEALING is *withdrawn is pending, the fault still stands*. `sfaultIsConfirmed` counts HEALING as confirmed, because the fault has not been withdrawn, only stopped being observed; `sfaultIsActive` does not, because it reports the raw observation. Anything that acts on a fault asks the first.

A presence flag has to be `TRUE` or `FALSE` and anything else is refused. Every C comparison already yields one of those two, so a third value means the caller passed something that was not a verdict, and guessing either way would be wrong.

`sstate` is 14 functions of guarded state machine. The legal transitions are a table, the table is checked once at Init, and every transition the table does not permit is refused and counted.

**It is not the same thing as `sdiag`'s flow signatures and the two do not overlap.** A signature says the path taken was the path expected; a transition table says the path was permitted at all. A program can follow exactly the sequence its signature expects and still have reached somewhere it should never have been able to get to. IEC 61508-3 asks for both kinds of program sequence monitoring.

Four things to know:

- **Every byte of the table is checked to be `TRUE` or `FALSE` at Init.** That one pass buys an unambiguous read on every later transition: `permitted` can test for `TRUE` rather than for anything non-zero, and a table built with the wrong constants is refused instead of being read as a permission set where every stray value means yes. Mutation testing confirms the pair is equivalent *given* the check — remove the check and the mutant lives.
- **A refused transition increments the refusal count**, which is this module's one deliberate inversion of the rule that outputs are written only on success. It is the record of the failure, not a corrupted output, exactly as `sdiag` writes `failIndex` only on `SD_FAILED`. Asking through `sstateCanGo` never counts, or a caller polling what it may do next would fill the count with questions.
- **`sstateForceTo` bypasses the table on purpose.** A caller restoring a machine after a reset has to put it back where it was, and a library that refused would have that caller assigning to `driver->state` directly, losing the state index check as well. It still refuses a state that does not exist.
- **`sstateIsReachable` walks the whole table, not one row.** It is the question a design review asks and a single row cannot answer: once the machine has reached its safe state, is there a path back out. The walk is a breadth first pass over a `uint32_t` bitmask, which is why `SSTATE_MAXSTATES` is 32.

A state is not reachable from itself unless a permitted path leads back to it. Answering `TRUE` by definition would discard a real question.

**Time in state**, added on 07/11/2026: `sstateTick` counts one cycle in the current state, saturating, and `sstateGetDwell` reads it. Every successful transition starts the count again, a forced one and a self loop included, because the state was entered; a refused one does not, because the machine never left. A timeout in a state is then a comparison of the dwell against a number of cycles, with the cycle being whatever the caller ticks on, as in `sfault` and `swatch`.

One comment in it was wrong until mutation testing said so. The pruning of already-expanded rows in the reachability walk was described as what prevents an endless walk; removing it changed no answer, because the pass bound is what guarantees termination and the pruning only saves work. The comment now says that.

`swatch` is 12 functions of deadline and liveness supervision. IEC 61508-3 asks for temporal program flow monitoring and `sdiag` does not do it: a flow signature says the steps happened in the right order and says nothing about when.

**Its whole safety argument is one line, and everybody writes that line wrong.** Every elapsed time is `tick - lastTick` as an unsigned subtraction, and nothing in the file compares two ticks with `<` or `>`. Written the obvious way — `if ( tick > lastTick )` — a supervisor works perfectly for 49.7 days and then, for one tick in 2^32, decides no time has passed and **never times out again**. The system it was watching can be dead from that moment on. Unsigned subtraction is modular by definition rather than by accident and is exact across the wrap; all it needs is that the true interval is under 2^32 ticks. The mutant that reintroduces the comparison is killed by six cases, and the suite also sweeps 300000 check-ins straight through the wrap so the property is shown to hold everywhere rather than at the one point somebody thought of.

Three more things:

- **Both bounds matter.** A module with only the late bound is half a supervisor. A task running twice as often as it should is a runaway loop or an interrupt firing on noise, and it exhausts a budget somewhere else long before anything times out. A minimum of zero is allowed but has to be chosen.
- **`swatchPoll` is why the module works at all for the fault it matters most for.** A task that has stopped running never checks in, so a supervisor that only learns from check-ins learns nothing about the case where the supervised thing is dead. Something else has to ask.
- **A missed deadline latches.** The supervised thing was not there when it should have been, and whatever depended on it has already been running on stale information. `swatchStart` resumes; the module does not recover on its own.

`swatchRemaining` reports zero past the deadline rather than a wrapped negative, and deliberately does not expire the watch: a query that changed the state would make the diagnosis depend on who looked.

`sbits` is 18 functions of bit field packing, in a word and across a byte array, and of whole integers in a named byte order. A protocol frame is a row of bit fields, and unpacking one by hand is three lines of shifting and masking that nobody reviews and everybody copies.

Two rules hold it together, and both are about undefined or implementation-defined behaviour rather than about arithmetic:

- **There is no shift by the width of the type.** `1u << 32` is undefined behaviour, not a way of writing zero, and it is where hand written bit field code goes wrong: a field the full width of the word produces a mask the standard does not define, and the answer depends on the part. It typically gives the right result on one and zero on another, so it survives every test run on the wrong machine. `lowMask` handles the full width case on its own.
- **There is no right shift of a signed value**, the same rule as `sfixed`. Sign extension is done by subtracting the width's modulus in `int64_t`, which the standard defines completely, and a negative value is packed by *adding* that modulus rather than by masking a signed value — bitwise operations on a negative signed value describe its representation, not its value.

The byte array form numbers bits from the least significant bit of the first byte, which is what CAN calls Intel format. **The module offers no other ordering on purpose:** a bit ordering guessed differently at the two ends of a link is a fault that looks like corrupted data. It moves one bit at a time rather than assembling whole bytes and shifting the ends into place, because at 32 bits the difference is a handful of instructions and the straddling errors all live in the faster version.

The writer checks the size, the position, the width *and* the value before it writes a single bit. A frame carrying half of a new signal and half of an old one is worse than one carrying neither.

**Byte order is a separate question from bit order, and the owner chose on 07/11/2026 to answer it.** `sbitsReadLE`, `sbitsReadBE`, `sbitsWriteLE` and `sbitsWriteBE` move unsigned integers of one to four bytes, with the order in the name so both ends of a link state it at the call. That does not reopen the bit ordering decision above: the bit fields stay Intel only. They move one byte at a time, so neither the processor's byte order nor its alignment enters, which is what makes them verifiable on a host for an ARM target. A writer refuses a value too big for its slot rather than keeping its low bytes, because the low bytes of a wrong number are a plausible number. A signed reading is the unsigned one followed by `sbitsGeti32` at eight times the byte count; there is no signed reader, because that would be a second sign extension to keep in step with the first.

`ssignal` is 7 functions of signal plausibility, added on 07/11/2026: a range check, a gradient check, a two sensor correlation and a stuck signal detector. They are the four questions every project asks of a sensor reading, and the module answers those and nothing else.

**It observes; it does not decide.** Whether a reading outside its range three cycles running is a fault is qualification, which is `sfault`'s job, and which input to use instead is `svote`'s. Only the stuck detector holds state, because stuck is a property of a run of samples, not of one.

Four things in it are easy to get wrong:

- **The range and gradient checks answer below, inside or above**, through `SG_VERDICT_*`, not a flag. Below and above usually mean different faults — a short to ground and a short to supply — and a flag throws that away.
- **Every difference is formed in 64 bits**, as in `svote`. A jump from `INT32_MIN` to `INT32_MAX` is minus one in 32 bits and would read as a signal that barely moved. The gradient's rise and fall limits are separate and the suite tests a step between them, because a mutant that used the wrong one survives everything else.
- **The correlation never divides.** Two sensors agree when `b` is within the tolerance of `a * gainNum / gainDen + offset`; both sides are multiplied by the denominator and compared in `int64_t`, so the prediction is never rounded. The gain is limited to `SSIGNAL_MAXGAIN` either way, which is what keeps every product under 2^47.
- **The stuck detector compares each sample with the first sample of its run, not with the previous one.** Against the previous sample a slow ramp moving less than the tolerance each step reads as stuck for ever; the suite drives exactly that ramp, and `ssignal` M9 is the mutant that measures against the previous sample. The run length saturates. A limit below two is refused at Init, and a zeroed driver is refused with `SG_NULLPTR`.

`se2e` is 5 functions of end to end protection, added on 07/11/2026: a sender that writes a counter and an eight bit CRC into a frame, and a receiver that classifies each frame it is given. The CRC covers a sixteen bit data identifier, the counter and the payload; **the identifier is never sent**, so a frame meant for another message fails its CRC exactly as a corrupted one does. It is a design of its own and claims compatibility with no specification's profiles.

Four things to know:

- **The CRC is injected at Init**, the driver-struct rule again. The function pointer has the shape of `sdiagCrc8SaeUpdate`, and the integration suite passes that function with no wrapper, which is what proves the pairing rather than asserts it. A failing CRC function is `SE_CRCFAILED` and nothing is written.
- **The receiver's status says whether the check ran; its result says what the frame is.** A corrupted frame is `SE_OK` with `SE_RESULT_ERROR`. The results are in sequence, in sequence with some lost (a step up to `maxDelta`), initial, repeated, out of sequence and error. A frame that never arrives produces no call, so detecting that is `swatch`'s job, and deciding how many bad frames make a fault is `sfault`'s; the integration suite does exactly that chain.
- **Every good frame except a repeat becomes the new reference, a wrong sequence included.** Otherwise a sender that restarted would be out of sequence for ever. A frame failing its CRC moves nothing, and the suite flips every bit of a frame and then checks the next good one is still in sequence.
- **`maxDelta` is at most 127**, half the counter range, because beyond that a step forward and a step back are the same number.

`slog` is 9 functions of fault event memory, added on 07/11/2026: one entry per event identifier in a caller owned table, with an occurrence count and the first and last cycle it was seen. `sfault` decides a fault exists; this remembers that it did, for a service tool to read after the event has gone.

Three things to know:

- **One entry per identifier.** Recording a known identifier counts it again, saturating, and moves its last cycle. A flapping fault costs one entry, not the table.
- **What a full table does is chosen at Init.** `SL_POLICY_KEEP` refuses the new event with `SL_FULL`, which keeps the first faults of an incident, usually the ones that explain the rest. `SL_POLICY_REPLACE` gives the new event the entry unseen longest. Either way the loss is counted by `slogLost`, so a full table is never silent.
- **"Unseen longest" is the largest `cycle - lastCycle`, unsigned, not the smallest `lastCycle`.** The two agree until the cycle counter wraps; after it the smallest last cycle is the newest entry, and the event that just happened would be the one thrown away. The suite records across the wrap for exactly that, and `slog` M5 is the mutant. `slogRemove` moves the last entry into the hole, so positions read with `slogGet` do not survive a removal.

`slogAdopt` was added the same day, when the integration suite tried to save the table and restore it after a reset and found no way to say how many entries were in use. It takes over a table that already holds entries and refuses one the module would never have produced — an identifier twice, or an entry recorded zero times — with `SL_CORRUPT`, before anything is written.

`sstore` is 3 functions of record integrity, added on 07/11/2026: seal a record, check one, and choose between two copies. It touches no flash or EEPROM; it works on byte buffers, and moving them to and from the medium is the integrator's. The record is a twelve byte header — version, payload length, sequence number and a CRC-32, every field least significant byte first — followed by the payload, which the caller writes in place before sealing, so nothing is copied and no two buffers can overlap. The CRC is a parameter shaped like `sdiagCrc32Update`; the module is stateless, so there is no Init to inject it at.

Three things to know:

- **Check verifies the CRC before the version.** A corrupted version field reads as `SO_CORRUPT`, not as another version, because only an intact record is worth migrating. A stated length past the buffer is `SO_CORRUPT` too: the length came from the medium.
- **Newer is decided across the sequence number's wrap**, as an unsigned difference of one to 2^31 - 1, the same reasoning as `swatch`'s ticks. A greater-than would choose the stale copy for ever once the counter wrapped; `sstore` M7 is that mutant. Equal sequences choose A.
- **One valid copy wins whatever the other's sequence says**, because a torn write's sequence cannot be trusted, and when neither is valid Select says `SO_WRONGVERSION` if either is intact but of another version, so the caller knows to migrate rather than load defaults. The suite tears a write half way and the integration suite does it with `slog`'s table and sdiag's CRC.

`spid` is 4 functions of fixed point PID control, added on 07/11/2026. Gains, setpoint, measurement and output are all Q16.16, the format `sfixed` uses, with the arithmetic done in the module itself so it copies out alone. The sample period is folded into the integral and derivative gains, so the controller never divides by a time step.

Four things to know:

- **Every product is formed in `int64_t` and brought down by division, never by a right shift.** A gain of at most 2^31 times a difference of at most 2^32 is below 2^63, so nothing overflows; division truncates toward zero for either sign, where an arithmetic shift would round a negative error the other way. `spid` M1 puts the shift back and a case with an error of minus three counts kills it.
- **The derivative is taken on the measurement**, so a setpoint step does not kick the output through the derivative term. The suite steps a setpoint by five with `kd` of ten and checks the output moves by `kp` times five only.
- **The integrator is held inside the output limits.** The suite holds the output on its limit for a thousand updates and checks it leaves the limit on the first update the error reverses; an integrator left to wind up would take a thousand more. Init and Reset start it at the nearest limit when zero is outside them, which only an integrating case with limits above zero can see — `spid` M14 survived until there was one.
- **`spidPreset` is the bumpless transfer**: the integrator set to the output the loop was held at by hand, and the measurement taken now, so the first automatic update reproduces the manual output with no derivative kick. A zeroed driver is refused through a `ready` flag, since every other field of a zeroed controller is a valid setting.

Tuning and stability are not this module's business and no test here speaks to them; they are verified on the plant.

`smemory` is 17 functions, the untyped half of the library: bounded replacements for the `mem` family of `<string.h>`. **The line between it and `sarray` is whether the operation has to know what the bytes mean.** Copy, move, set, compare and search do not, so they take a `void*` and live here. A sum, a minimum or an ordering by magnitude does, so it lives in `sarray`. When adding a function, that question decides the module — do not add a typed operation to `smemory` or a byte-blind one to `sarray`.

Two `smemory` functions have no `<string.h>` counterpart and exist for reasons that are easy to undo by accident:

- `smemoryEqualSecure` reads all `count` bytes with no early exit, because `memcmp` on a MAC leaks the length of the matching prefix through its timing. It reports equal or not equal only — producing an ordering means finding the first difference, and finding the first difference *is* the leak. Its operands are read through `const volatile unsigned char*` so the compiler cannot restore the early exit. Do not "optimise" that loop.
- `smemoryClearSecure` writes through `volatile`, which `smemoryClear` does not. `smemoryClear` is an ordinary store and a dead-store eliminator may delete it entirely. Both exist on purpose; the Doxygen on each says which to use.

`smemory` carries a second MISRA deviation `sstring` and `sarray` do not: Rule 11.5, `void*` to object pointer. It is confined to `unsigned char*`, the one object type the standard always permits for examining an object's bytes.

`smath` is 68 functions across the same four numeric families as `sarray`. Its single rule: **every check happens before the operation, never after.** There is no place in it where a result wraps and is then inspected — for a signed type that inspection is already undefined behaviour, and for an unsigned one the wrapped value carries no evidence that it wrapped. Overflow is detected by division rather than by a wider intermediate, so only `Scale` and `Average` need 64-bit arithmetic.

The checked and saturating forms of add, subtract and multiply share one status helper each, so `smathAddSat` saturates exactly where `smathAdd` reports `SH_OVERFLOW`. Keep it that way: two independent boundary tests will disagree eventually.

`sdiag` is 20 functions. Two of its features hold state, in caller-owned structs (`sdiagflow_t`, `sdiagshadow_t`) so the functions stay reentrant; the rest are stateless. `sring` is the module actually built around the driver-struct pattern. Its banner has claimed "without hardware dependencies" since 2022 and that is the scope: CRC (32, 16 and two 8 bit forms for short bus frames) and checksum, March memory tests, stack usage measurement, control-flow signatures, redundant storage. **A CPU register test, a program counter test and an instruction set test are deliberately absent** — they cannot be written in C at all, and a complete IEC 61508 / ISO 26262 self test needs assembly for them. Do not add a HAL or a linker symbol to this module to close that gap; it belongs in a separate, non-portable one.

`sdiag` has one deliberate inversion of the library-wide rule that outputs are written only on success: the `failIndex` of both memory tests is written **only** on `SD_FAILED`. On a memory test the failing address is the entire result and there is nothing to report when nothing failed.

`sring` is 12 functions and is the **reference for the driver-struct pattern** — the first module built around it rather than merely holding a struct. A single-producer single-consumer byte ring, lock-free for the case it exists for: an interrupt filling it while the main loop drains it.

Its whole safety argument rests on one property, and any change has to preserve it: **the producer writes only `writeIndex`, the consumer writes only `readIndex`, and neither index is ever written by both sides.** `sringPut`/`sringPutBlock` are the producer; `sringGet`/`sringGetBlock`/`sringClear` are the consumer. `sringClear` moves `readIndex` up to `writeIndex` rather than zeroing both, precisely so it stays on the consumer side. The test suite checks this structurally, because every functional case still passes when it is broken.

Two things follow from that design and are not negotiable:

- **One byte of the buffer is never used.** Full and empty are otherwise the same state, and telling them apart with a count needs a field both sides write — which is exactly what makes a ring need a lock. A 64-byte buffer holds 63. Do not "fix" this.
- **`volatile` is not a memory barrier.** It stops the compiler caching an index; it does not stop the processor completing the two stores out of order, and the byte must land before the index that publishes it. Enough on Cortex-M0/M0+/M3/M4, not enough on M7 or anything multi-core. The barrier cannot be issued from library code (it is an intrinsic, and this library includes no vendor header), so it is **injected at `Init` as a function pointer** — the driver-struct rule applied literally. It fires after the data moves and before the index does, and never on a refused operation.

Two producers, or two consumers, are unsafe and documented as such.

**Every function but `Init` refuses a driver that never went through `Init`**, through the same `isReady` helper `sscale` has. This is part of the driver-struct pattern, not a detail of one module. It was added on 05/08/2026, after `sscale` had it and `sring` did not, and the gap was not theoretical: on a zeroed ring, `sringPutBlocku8` computed a free space of `0xFFFFFFFF` from a capacity of zero and wrote through the NULL buffer, while `sringFreeu8` and `sringCapacityu8` returned `SR_OK` and reported four gigabytes. Only the single-byte forms were accidentally safe, because two zero indices read as an empty ring.

The guard costs the lock-free split nothing, and that is why it is allowed here: it reads `buffer` and `capacity`, which only `Init` writes and which neither side touches while the ring runs.

Its two conditions are individually redundant against a zeroed driver and each survives mutation for that reason — `buffer == NULL` catches the zeroed case on its own, and so does `capacity < 2u`. Disabling the whole helper is killed, by an access violation. Keep both: they cover different hand-built structs, and neither is reachable through the API.

Modules are **fully independent**: every `.c` includes only its own header plus freestanding standard headers (`<stdint.h>`, `<stddef.h>`). No module includes another module's header. That independence is what makes single-module copy-out work.

Start a new module by copying `template/`, never by copying an existing module.

## Licence

GPL-3.0-or-later. **Every header carries `/* SPDX-License-Identifier: GPL-3.0-or-later */` as its first line and every source carries a `@par License` block in its banner**, because a module is consumed by copying its pair out and a licence named only in this repository does not travel with the copy. `tools/doxcheck.py` enforces both, so a new module cannot ship without them; `template/` already has them, which is another reason to start there.

The verbatim licence text is in `LICENSE` at the root, and has been since the initial commit. On 04/11/2026 it was compared against <https://www.gnu.org/licenses/gpl-3.0.txt> and the committed blob is byte-identical to it; the working copy differs only by CRLF line endings, which git adds on checkout. A copy of the GPL is only the GPL if it is exact, so **never edit `LICENSE` by hand** — replace it only with a fresh official copy. `COPYING.md` records everything else about the licence.

## Safety rules

Beyond the shared esclib style, this library adds:

- **Every loop bound comes from a parameter.** No `while ( *p )`, no `for ( ; *p != '\0'; ++p )`. This is what makes an unterminated input a bounded read and a status code instead of a runaway scan.
- **Every pointer parameter is immediately followed by the capacity of the buffer it points at**, and a bound is never inferred from a different buffer. `sstringCopy` takes `srcSize` as well as `destSize` for exactly this reason — deriving the source scan bound from the destination size let a short unterminated source be read past its end, and a guard-page test reproduced it as a fault. Do not add a function that reads through a pointer without its own capacity.
- **Validate, then commit.** Every check completes before the first byte is written; on any failing status the destination is bit-for-bit unchanged. Build a result in a local scratch array when the final size is not known up front — see `sstringFromI32`.
- Every pointer parameter is NULL checked before use.
- No dynamic allocation, ever. The caller owns all storage.
- No `<string.h>`, no `<ctype.h>`, no `<stdlib.h>`. Only `<stdint.h>` and `<stddef.h>`.
- Failure is reported through the return value. No abort, no assert, no logging.
- **An unsigned operand meets only unsigned constants**: `size == 0u`, `i + 1u`,
  `wide / 2u`. An unsuffixed constant is a signed `int`, and MISRA C:2012 Rule
  10.4 does not let the two be mixed. The library's 10.4 deviation covers
  `uint8_t` statuses, states and flags compared with enum members and
  `TRUE`/`FALSE`, and nothing else; `tools/misra.sh` fails on any other hit.
  A signed operand keeps its plain constant (`value < 0`).
- **Every counter saturates.** A count of failures that wrapped would read as
  small again exactly when it had grown largest. Until 07/11/2026 six counters
  did wrap — `sstate`'s transitions and refusals, `swatch`'s check ins, early
  and late counts, and `sdiag`'s flow checkpoint count, where 2^32 extra
  checkpoints would have matched the expected count again. Each now goes
  through a `saturatingIncrement` helper and has a mutant that puts the wrap
  back. A counter that wraps on purpose, like a ring index, is bounded by a
  capacity instead and says so.
- No module state, so every function is reentrant. **`tools/size.sh` measures this rather than trusting it**: every module is required to show `.data` and `.bss` of zero and the script goes red otherwise. All nineteen do. The whole library is about 28 KB of flash on a Cortex-M0+ and 25 KB on a Cortex-M4, measured whole, which is an upper bound because a project linking with `--gc-sections` pays only for the functions it calls.

## The driver-struct pattern

For stateful modules (from esclib, applies here as new modules gain state):

- One `typedef struct { ... } <prefix>_t;` holding all state.
- First parameter of every function is `<prefix>_t* driver`; buffers are passed into `Init` as pointers.
- Names are `<prefix>` + verb: `xxxInit`, then `xxxUpdate`/`xxxReceive`/`xxxAdd`, then `xxxGetValue`/`xxxGetStatus`.
- Type-suffixed names when a module is width-specific (`xxxAddu32`).
- Hardware and I/O are injected as function pointers stored in the struct at `Init`. Never call a HAL directly from library code.

`sring` is the worked example of all of it: `sringu8_t` holds every field, `driver` is the first parameter of all twelve functions, the buffer is handed to `sringInitu8`, the names are `sringInitu8` / `sringPutu8` / `sringGetu8` / `sringCountu8`, the type suffix is there because the module is width-specific, and the memory barrier — the one piece of processor behaviour it needs — is a function pointer stored at `Init` instead of a call into a HAL. Copy its shape when adding a stateful module.

Ten modules now hold state and all ten refuse a driver that never went
through `Init`: `sring`, `sscale`, `sfault`, `sstate`, `swatch`, `ssignal`,
`slog` and `sfilter` through a static `isReady` helper, `spid` through a
`ready` flag because every other field of a zeroed controller is a valid
setting, and `se2e` by its injected
CRC being NULL, which is the one field a zeroed driver cannot have after
Init. **Nine of them report that as
`*_NULLPTR` and `sfilter` reports `SF_INVALIDSIZE`, which is deliberate and
stays that way.** A zeroed `sfilteravg_t` has both a NULL buffer and a
capacity of zero, and for a moving average the size is the more useful of
the two things to name; changing a verified module's contract for cosmetic
consistency buys nothing and costs every existing call site. The difference
is recorded here so that the next stateful module copies one of them on
purpose rather than by accident: **use `*_NULLPTR` unless there is a reason
like `sfilter`'s.**

The check catches a driver in static storage that the C startup zeroed. It
cannot catch one in automatic storage that was never initialised, and no
check in C can; every one of the six says so in its own `@note` rather than
implying more.

The remaining modules are stateless and take their buffers and capacities
directly instead.

## Header contract

Every header is a copy of `template/inc/generic.h` with content filled into fixed sections. Preserve all of it, including the empty sections:

```c
#ifndef <NAME>_H_
#define <NAME>_H_
#ifdef __cplusplus
 extern "C" {
#endif
#include <stdint.h>
/* FUNCTION DEFINITIONS */
/* DEFINITIONS */
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
/* TYPEDEFS */
/* STRUCTURES */
/* ENUMS */
/* EXTERNS */
/* FUNCTION PROTOTYPES */
```

`TRUE`/`FALSE` are redefined per header on purpose (guarded by `#ifndef`) so each module stays self-contained. Enums use a `SCREAMING_CASE` tag with short prefixed members.

## Doxygen

Documentation lives in `.c` files only; headers stay pure declarations.

Every `.c` opens with the banner from `template/src/generic.c`: `@file`, `@author`, `@version`, `@date`, `@brief`, `@par Device`, `@par History`, optional `@note`. When modifying a module, **append a dated line to `@par History`** (`DD/MM/YYYY Description @n`) and bump `@version`. There is no `@content` function list — Doxygen generates it, and hand maintained lists drift.

Every function gets a `/**` block using exactly `@brief`, `@param[in]`/`@param[out]`/`@param[in,out]`, `@return` (non-void only), `@note` (only when it adds real information). `static` helpers are documented too, because `EXTRACT_STATIC = YES`. A `/*` block is invisible to Doxygen — always `/**`.

## Coding style

Defined in `codingReference.md`, enforced by convention across the whole tree:

- Spaces inside every paren: `if ( ( a > b ) || ( c == d ) )`, `foo ( &driver, 5 )`.
- Allman braces. Braces on every block, even single statements.
- Pre-increment: `++i`, `++driver->wp`.
- At most one `break` in a loop. No pointer arithmetic on arrays — index only.
- Single `retVal` local initialized at declaration, single exit: `return ( retVal );` (parenthesized).
- Status returns use `TRUE`/`FALSE`, not `0`/`1` literals.
- Empty `else` branches are written out with `// Intentionally blank.` rather than omitted.
