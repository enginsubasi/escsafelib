# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this repo is

`escsafelib` is a freestanding C library for safety related applications (GPLv3): bounded string handling, bounded arrays and raw memory, checked arithmetic, a lock-free byte ring, and self diagnostics. No heap, no OS dependency, `<stdint.h>` types throughout. It is the safety oriented sibling of `esclib` and follows the exact same conventions.

All nine modules are implemented — 296 functions and 2929 self-checking test cases.

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
```

Run the tests before claiming anything passes. Compiling is not passing.

A second host compiler is available: **MinGW-W64 gcc 14.2.0**, bundled with Code::Blocks at `C:\Program Files\CodeBlocks\MinGW\bin`. It is not on `PATH`; prefix the command instead:

```bash
PATH="/c/Program Files/CodeBlocks/MinGW/bin:$PATH" gcc --version
```

Use it. gcc and clang do not warn about the same things — gcc's `-Wextra` includes `-Wtype-limits`, which clang does not implement the same way, and that difference has already caught dead code in a generated test that clang passed silently. (There is a second `gcc.exe` at `C:\Program Files (x86)\STMicroelectronics\...\MinGW`; it is version 4.5.0 from 2010 and does not support C99. Ignore it.)

**gcc has no AddressSanitizer either.** MinGW-W64 ships no ASan runtime, so the control does not link. Neither host compiler on this machine can run ASan; the Ubuntu CI runner is still the only place it works, and the guard-page harnesses are still how out-of-bounds reads get proven here.

All nine modules are clean under a much stricter warning set than the project normally uses, confirmed independently by clang 21 via zig **and** gcc 14.2.0:

```bash
-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion -Wcast-qual \
-Wcast-align -Wstrict-prototypes -Wmissing-prototypes -Wredundant-decls \
-Wundef -Wwrite-strings
```

Zero warnings across all nine. Confirm the flags are live before trusting that — a control with an implicit narrowing conversion produces two warnings under the same command line.

A suite that passes on the first run has not yet been shown to check anything. Mutate the module — flip a bounds test to off by one, delete an overflow guard, disable an overlap check — rebuild against the mutant and confirm the suite goes red. `sarray` was cleared against 6 mutants, `smemory` against 7, `smath` against 11, `sring` against 11, `sfilter` against 11, `sfixed` against 12, `sscale` against 16 of 17, `sdiag` against 10 of 12.

`sring` counts 12 because the unready-driver guard was added after the first eleven. Only three of those eleven were re-run against the new source, and that is enough: the guard adds one early branch that returns TRUE for every driver a successful `Init` produced, so for the cases those mutants exercise the control flow is bit-identical.

The `sring` run is the one that shows mutation testing paying for itself directly. A mutant that made `sringPutBlocku8` compute one byte too much free space passed the whole suite, because no case put *exactly* the usable size through the block form. That mutant is a real bug: it fills the buffer completely, which makes the two indices equal, which is the encoding for empty — so the ring would report holding nothing straight after being handed a full load. The boundary cases that kill it were written because the mutant survived, not the other way round.

Two results from those runs are worth keeping in mind:

- Masking `smemoryEqualSecure` down to the low bit of each difference still passes every naive equality case. Only the deliberate high-bit-only case catches it.
- **One `sscale` mutant survives and is provably equivalent.** Removing
  the up-front refusal of a flat first pair and widening the direction
  test from `>` to `>=` changes nothing observable: the only table it
  affects is one where `x[1] == x[0]`, and the validation loop's first
  iteration refuses that in either direction, leaving the same status
  and an equally untouched driver. That is a proof, not a hole, and it
  is the reason the explicit `==` check is in the source anyway — the
  code should say what it means rather than be right by accident.
- **Two `sdiag` mutants survive and are known to.** Disabling the zero-read checks of March element two, or the address uniqueness pass, changes nothing that any harness on this machine can observe: healthy host memory always reads back what was written, and the aliasing harness's fault is caught redundantly by a later March element. See "Untestable here" below.

`tools/coverage.sh` measures statement and branch coverage with gcc's
`--coverage` and gcov. Mutation testing samples; coverage counts, and the two
answer different questions. The first run said so plainly: `sscale`, at the
time the most heavily tested module in the library — 223 cases and 17 mutants
— had never taken ten percent of its own branches.

```bash
PATH="/c/Program Files/CodeBlocks/MinGW/bin:$PATH" bash tools/coverage.sh gcc
```

What that first run found, and what closing it cost:

| module | branches taken, before | after |
|---|---|---|
| `sarray` | 79.4% | 97.2% |
| `smemory` | 81.7% | 90.4% |
| `sdiag` | 82.2% | 84.2% |
| `sstring` | 83.0% | 88.3% |
| `sfixed` | 86.0% | 94.7% |
| `sscale` | 89.9% | 90.8% |
| `smath` | 91.3% | 97.0% |
| `sring` | 91.5% | 91.5% |
| `sfilter` | 92.3% | 94.6% |

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
- `sarray`: the sum overflow in the `u8` and `u16` families. The accumulator is
  a `uint32_t`, so `u8` needs 16.8 million elements and `u16` needs 65537. The
  identical guard is exercised in the `u32` and `i32` families.
- `smath`: the unsigned saturating forms saturating toward the end they cannot
  reach — the same impossibility recorded in their `@return` blocks.
- `sring`, `sscale`: the redundant half of `isReady`. Either condition alone
  catches a zeroed driver, which is why both survive mutation.
- `sscale`: `SC_INVALIDTABLE` out of `Apply` and `FindSegment`, and the
  degenerate-segment guard in `interpolate`. A validated table cannot produce
  any of them.
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

- **Stuck-at memory faults.** Nothing portable can produce a cell that accepts a write and returns something else, so the read checks inside the March elements are covered by review only. This is why two `sdiag` mutants survive.
- **Execution on ARM. This one is permanent, not pending.** There is no target and no emulator, and there will not be one. Every module cross-compiles for `arm-none-eabi` and passes `-fanalyzer`, which says the code builds for the target, not that it behaves there. Everything the test suites prove, they prove on x86-64 hosts.

  What that costs is narrow but real, and it should be stated rather than glossed. The suites cannot see: unaligned access faults (`sarray` and `sdiag` take typed pointers partly to make this the compiler's problem, but a caller can still hand over a misaligned buffer); anything that depends on the target's actual word size or padding; the memory-ordering assumption in `sring`, whose whole barrier argument is about a processor this code has never run on; and the real timing of `smemoryEqualSecure`, which is constant in comparison count but was never measured on a core with a cache.

  Do not write "verified on ARM" anywhere. An integrator putting this on a target owns that validation, and the honest claim is that the logic is verified on a host and the code builds clean for the target.
- **cppcheck, MISRA and doxygen.** None of the three is installed, and there is no `gh` to read the CI results back. Two MISRA deviations are recorded by hand (Rule 11.4 in `sstring`/`sarray`/`smemory`, Rule 11.5 in `smemory`/`sdiag`); no checker has confirmed there are only two.

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
- **Reporting** (`continue-on-error: true`) — `cppcheck` including the MISRA addon, `host-analyzer` (a newer gcc analyzer than the local arm one), and `docs` (doxygen). None of these tools exist on the development machine, so their first run is a discovery, not a gate. **Flip a reporting job to blocking once a run comes back clean**; until then do not claim the repo is cppcheck-clean, MISRA-checked, or doxygen-warning-free.

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

Doxygen is not installed here, so `Doxyfile` ships unverified; `WARN_NO_PARAMDOC = YES` is what turns the comment convention into a tool enforced rule when the owner runs it.

`tools/doxcheck.py` covers the gap in the meantime, and covers more than doxygen would. It checks every `.c` in the tree — 488 functions, statics and test helpers included — for banner completeness, a `/**` block on every function, `@param` lists that match the signature *in both directions*, `[in]` on every read-only parameter, `@return` present exactly when the return type is not void, and unknown tags. It is a blocking CI job because it runs clean here.

**Its most useful rule is one doxygen has no concept of: a `@return` must name every status the body can set, and no status it cannot.** That is what found the only real defect the first full audit turned up. The four `smath` families share one template, so the unsigned ones had inherited the signed text and promised an `SH_UNDERFLOW` that an unsigned addition cannot produce and an `SH_OVERFLOW` that an unsigned subtraction cannot — a caller testing for either was writing a branch that never runs.

Two patterns it accepts on purpose, because both are better documentation than the alternative:

- **`"otherwise the status sarrayCopyNu8 reports"`** documents the delegate's statuses. Copying the list would drift.
- **A status the body can form but can never produce may be explained in a `@note`** rather than listed in `@return`. `sfixedFloor` calls a range check that can say `SX_OVERFLOW`, and rounding down cannot reach it. The audit added those notes rather than removing the checks.

The same rule applies to it as to the test suites: a checker that passes on its first run has been shown to check nothing. It was developed against a control file carrying one deliberate defect per rule, and every rule fired. Do that again before trusting a change to it.

## Layout and module contract

```
inc/<domain>/<name>.h   ←→   src/<domain>/<name>.c    strict 1:1 pair
test/<Name>_Test/<Name>_Test.c                        self-checking test main
template/inc/generic.h, template/src/generic.c        copy these to start a new module
```

Domains: `array`, `diag`, `filter`, `fixed`, `math`, `memory`, `ring`, `scale`, `string`.

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
| `SM_` | `smemory` | | |

Two prefixes could not be the obvious abbreviation. `sfixed` takes the `X` of
fi**x**ed because `sfilter` already held `SF_`, and `smath` takes the `H` of
mat**h** because `smemory` already held `SM_`. `SM_` was deliberately left
with `smemory` rather than moved: renaming a shipped module's prefix to free
a letter is churn paid by every existing call site.

All nine modules are implemented. `sstring` is the reference: 41 functions covering length, copy, move, concatenate, compare, clear, search, tokenize, transform, validate and number conversion. Its design is written up in `docs/superpowers/specs/2026-08-02-sstring-design.md`. **Do not write further spec documents or implementation plans for this repo** — the owner wants the design agreed in chat and then implemented directly.

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

`sscale` is 12 functions of piecewise linear scaling — a breakpoint table from a datasheet, and everything between the breakpoints taken to be a straight line. Thermistor counts to degrees, load cell counts to newtons.

It is the second module built around the driver struct, and unlike `sring` the reason is safety rather than state. **`sscaleInit` is the only function that validates the table, and no other function can be reached without it having succeeded.** A caller cannot interpolate through an unchecked table, cannot swap the two arrays by accident, and cannot pass a count that disagrees with the one the table was validated against. That is worth more here than the parameter list it saves.

Four things in it are easy to get wrong:

- **Both directions of table are accepted.** A thermistor's resistance falls as its temperature rises, so its table descends. Forcing the caller to reverse the array by hand would put the one error this module exists to prevent back in the caller's code. Direction is taken from the first pair and every remaining pair must agree, so a table that turns round half way is refused rather than searched.
- **Interpolation rounds to nearest, halves away from zero**, where `sfixed` truncates. The difference is deliberate: truncation biases every reading toward zero by up to one count, always in the same direction, and on a calibration curve a bias is a systematic error rather than noise.
- **The overflow check lives in `Init`, not in `Apply`.** One segment's spans can multiply past `int64_t` — the boundary is an input span of 2^32-1 against an output span of 2^31, which fits, and one count more, which does not. Checking it once at startup makes a bad table a configuration error rather than a runtime failure found by whichever reading first happened to reach it, and it makes the arithmetic in `Apply` total.
- **The rounding is written with a remainder, not by adding half the denominator.** That addition would itself overflow when the numerator is near the top of the type. Twice a remainder cannot.

The `y` array is deliberately *not* required to be monotonic — a calibration curve may fold back and still be a function of its input. That is also why `sscaleInvert`, which builds the reverse map by exchanging the two arrays, can fail: the inverse of a folded curve is not a function. It is one call into `sscaleInit`, so the requirement is enforced by the same code that enforces it for a forward table.

Its test suite contains **no floating point**, for the same reason `sfixed`'s does not. The rounding rule is verified by cross multiplication in `int64_t`: a result is correct to nearest when twice its error has a magnitude no greater than the denominator.

`smemory` is 17 functions, the untyped half of the library: bounded replacements for the `mem` family of `<string.h>`. **The line between it and `sarray` is whether the operation has to know what the bytes mean.** Copy, move, set, compare and search do not, so they take a `void*` and live here. A sum, a minimum or an ordering by magnitude does, so it lives in `sarray`. When adding a function, that question decides the module — do not add a typed operation to `smemory` or a byte-blind one to `sarray`.

Two `smemory` functions have no `<string.h>` counterpart and exist for reasons that are easy to undo by accident:

- `smemoryEqualSecure` reads all `count` bytes with no early exit, because `memcmp` on a MAC leaks the length of the matching prefix through its timing. It reports equal or not equal only — producing an ordering means finding the first difference, and finding the first difference *is* the leak. Its operands are read through `const volatile unsigned char*` so the compiler cannot restore the early exit. Do not "optimise" that loop.
- `smemoryClearSecure` writes through `volatile`, which `smemoryClear` does not. `smemoryClear` is an ordinary store and a dead-store eliminator may delete it entirely. Both exist on purpose; the Doxygen on each says which to use.

`smemory` carries a second MISRA deviation `sstring` and `sarray` do not: Rule 11.5, `void*` to object pointer. It is confined to `unsigned char*`, the one object type the standard always permits for examining an object's bytes.

`smath` is 68 functions across the same four numeric families as `sarray`. Its single rule: **every check happens before the operation, never after.** There is no place in it where a result wraps and is then inspected — for a signed type that inspection is already undefined behaviour, and for an unsigned one the wrapped value carries no evidence that it wrapped. Overflow is detected by division rather than by a wider intermediate, so only `Scale` and `Average` need 64-bit arithmetic.

The checked and saturating forms of add, subtract and multiply share one status helper each, so `smathAddSat` saturates exactly where `smathAdd` reports `SH_OVERFLOW`. Keep it that way: two independent boundary tests will disagree eventually.

`sdiag` is 16 functions. Two of its features hold state, in caller-owned structs (`sdiagflow_t`, `sdiagshadow_t`) so the functions stay reentrant; the rest are stateless. `sring` is the module actually built around the driver-struct pattern. Its banner has claimed "without hardware dependencies" since 2022 and that is the scope: CRC and checksum, March memory tests, stack usage measurement, control-flow signatures, redundant storage. **A CPU register test, a program counter test and an instruction set test are deliberately absent** — they cannot be written in C at all, and a complete IEC 61508 / ISO 26262 self test needs assembly for them. Do not add a HAL or a linker symbol to this module to close that gap; it belongs in a separate, non-portable one.

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

## Safety rules

Beyond the shared esclib style, this library adds:

- **Every loop bound comes from a parameter.** No `while ( *p )`, no `for ( ; *p != '\0'; ++p )`. This is what makes an unterminated input a bounded read and a status code instead of a runaway scan.
- **Every pointer parameter is immediately followed by the capacity of the buffer it points at**, and a bound is never inferred from a different buffer. `sstringCopy` takes `srcSize` as well as `destSize` for exactly this reason — deriving the source scan bound from the destination size let a short unterminated source be read past its end, and a guard-page test reproduced it as a fault. Do not add a function that reads through a pointer without its own capacity.
- **Validate, then commit.** Every check completes before the first byte is written; on any failing status the destination is bit-for-bit unchanged. Build a result in a local scratch array when the final size is not known up front — see `sstringFromI32`.
- Every pointer parameter is NULL checked before use.
- No dynamic allocation, ever. The caller owns all storage.
- No `<string.h>`, no `<ctype.h>`, no `<stdlib.h>`. Only `<stdint.h>` and `<stddef.h>`.
- Failure is reported through the return value. No abort, no assert, no logging.
- No module state, so every function is reentrant.

## The driver-struct pattern

For stateful modules (from esclib, applies here as new modules gain state):

- One `typedef struct { ... } <prefix>_t;` holding all state.
- First parameter of every function is `<prefix>_t* driver`; buffers are passed into `Init` as pointers.
- Names are `<prefix>` + verb: `xxxInit`, then `xxxUpdate`/`xxxReceive`/`xxxAdd`, then `xxxGetValue`/`xxxGetStatus`.
- Type-suffixed names when a module is width-specific (`xxxAddu32`).
- Hardware and I/O are injected as function pointers stored in the struct at `Init`. Never call a HAL directly from library code.

`sring` is the worked example of all of it: `sringu8_t` holds every field, `driver` is the first parameter of all twelve functions, the buffer is handed to `sringInitu8`, the names are `sringInitu8` / `sringPutu8` / `sringGetu8` / `sringCountu8`, the type suffix is there because the module is width-specific, and the memory barrier — the one piece of processor behaviour it needs — is a function pointer stored at `Init` instead of a call into a HAL. Copy its shape when adding a stateful module.

The other five modules are stateless and take their buffers and capacities directly instead.

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
