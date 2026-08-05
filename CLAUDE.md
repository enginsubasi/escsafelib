# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this repo is

`escsafelib` is a freestanding C library for safety related applications (GPLv3): bounded string handling, bounded arrays and raw memory, checked arithmetic, a lock-free byte ring, and self diagnostics. No heap, no OS dependency, `<stdint.h>` types throughout. It is the safety oriented sibling of `esclib` and follows the exact same conventions.

All six modules are implemented — 246 functions and 1936 self-checking test cases.

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
  test/BasicMathSafe_Test/BasicMathSafe_Test.c src/math/basicmathsafe.c \
  -o basicmathsafe_test && ./basicmathsafe_test

python -m ziglang cc -Wall -Wextra -std=c99 -g -Iinc/selfdiag \
  test/SelfDiagSafe_Test/SelfDiagSafe_Test.c src/selfdiag/selfdiagsafe.c \
  -o selfdiagsafe_test && ./selfdiagsafe_test

python -m ziglang cc -Wall -Wextra -std=c99 -g -Iinc/ring   test/SRing_Test/SRing_Test.c src/ring/sring.c -o sring_test && ./sring_test
```

Run the tests before claiming anything passes. Compiling is not passing.

A second host compiler is available: **MinGW-W64 gcc 14.2.0**, bundled with Code::Blocks at `C:\Program Files\CodeBlocks\MinGW\bin`. It is not on `PATH`; prefix the command instead:

```bash
PATH="/c/Program Files/CodeBlocks/MinGW/bin:$PATH" gcc --version
```

Use it. gcc and clang do not warn about the same things — gcc's `-Wextra` includes `-Wtype-limits`, which clang does not implement the same way, and that difference has already caught dead code in a generated test that clang passed silently. (There is a second `gcc.exe` at `C:\Program Files (x86)\STMicroelectronics\...\MinGW`; it is version 4.5.0 from 2010 and does not support C99. Ignore it.)

**gcc has no AddressSanitizer either.** MinGW-W64 ships no ASan runtime, so the control does not link. Neither host compiler on this machine can run ASan; the Ubuntu CI runner is still the only place it works, and the guard-page harnesses are still how out-of-bounds reads get proven here.

All six modules are clean under a much stricter warning set than the project normally uses, confirmed independently by clang 21 via zig **and** gcc 14.2.0:

```bash
-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion -Wcast-qual \
-Wcast-align -Wstrict-prototypes -Wmissing-prototypes -Wredundant-decls \
-Wundef -Wwrite-strings
```

Zero warnings across all six. Confirm the flags are live before trusting that — a control with an implicit narrowing conversion produces two warnings under the same command line.

A suite that passes on the first run has not yet been shown to check anything. Mutate the module — flip a bounds test to off by one, delete an overflow guard, disable an overlap check — rebuild against the mutant and confirm the suite goes red. `sarray` was cleared against 6 mutants, `smemory` against 7, `basicmathsafe` against 11, `sring` against 11, `selfdiagsafe` against 10 of 12.

The `sring` run is the one that shows mutation testing paying for itself directly. A mutant that made `sringPutBlocku8` compute one byte too much free space passed the whole suite, because no case put *exactly* the usable size through the block form. That mutant is a real bug: it fills the buffer completely, which makes the two indices equal, which is the encoding for empty — so the ring would report holding nothing straight after being handed a full load. The boundary cases that kill it were written because the mutant survived, not the other way round.

Two results from those runs are worth keeping in mind:

- Masking `smemoryEqualSecure` down to the low bit of each difference still passes every naive equality case. Only the deliberate high-bit-only case catches it.
- **Two `selfdiagsafe` mutants survive and are known to.** Disabling the zero-read checks of March element two, or the address uniqueness pass, changes nothing that any harness on this machine can observe: healthy host memory always reads back what was written, and the aliasing harness's fault is caught redundantly by a later March element. See "Untestable here" below.

When writing a sweep against a wider-type oracle, **the oracle is where the bug will be.** The first `basicmathsafe` run reported 32640 failures in unsigned subtract; the module was right and the oracle wrong, because `(uint32_t) a - b` wraps inside the oracle's own type and reads as an overflow when the truth is an underflow. 32640 is exactly the number of pairs with `a < b`. A failure count that equals a recognisable combinatorial quantity is a sign the oracle is broken, not the module.

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

The same idea covers `selfdiagsafe`'s memory tests, which a portable test cannot reach at all: map one 64 KiB section at two adjacent addresses with `CreateFileMapping` plus two `MapViewOfFileEx` calls, and the second half of the region genuinely *is* the first half. That is a real address decoder fault. `selfdiagsafeRamTestDestructive` reports `SD_FAILED` at the first word of the second view, `selfdiagsafeRamTestNonDestructive` returns `SD_OK` exactly as its documentation says it will, and a healthy region of the same size passes both. Prove the aliasing is real before trusting the result — write two different values and check the first read back as the second.

## Untestable here

Three things cannot be verified on this machine, and no claim should be made about them until they are:

- **Stuck-at memory faults.** Nothing portable can produce a cell that accepts a write and returns something else, so the read checks inside the March elements are covered by review only. This is why two `selfdiagsafe` mutants survive.
- **Execution on ARM.** Every module cross-compiles and passes `-fanalyzer`; none has ever run on a target or under an emulator. There is no `qemu` here.
- **cppcheck, MISRA and doxygen.** None of the three is installed, and there is no `gh` to read the CI results back. Two MISRA deviations are recorded by hand (Rule 11.4 in `sstring`/`sarray`/`smemory`, Rule 11.5 in `smemory`/`selfdiagsafe`); no checker has confirmed there are only two.

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

## Layout and module contract

```
inc/<domain>/<name>.h   ←→   src/<domain>/<name>.c    strict 1:1 pair
test/<Name>_Test/<Name>_Test.c                        self-checking test main
template/inc/generic.h, template/src/generic.c        copy these to start a new module
```

Domains: `array` (`sarray`), `math` (`basicmathsafe`), `memory` (`smemory`), `ring` (`sring`), `selfdiag` (`selfdiagsafe`), `string` (`sstring`).

All six modules are implemented. `sstring` is the reference: 41 functions covering length, copy, move, concatenate, compare, clear, search, tokenize, transform, validate and number conversion. Its design is written up in `docs/superpowers/specs/2026-08-02-sstring-design.md`. **Do not write further spec documents or implementation plans for this repo** — the owner wants the design agreed in chat and then implemented directly.

`sarray` is 92 functions: twenty three operations repeated across four element families, `uint8_t`, `uint16_t`, `uint32_t` and `int32_t`. Two things about it differ from `sstring` and will bite if forgotten:

- **Every size and count in `sarray` is an element count, not a byte count.** `arrSize` of 8 on a `uint32_t` array is 32 bytes. Only the two static helpers that feed `isOverlapping` deal in bytes, and `spanBytes` guards that multiply against wrapping.
- **The four families are mechanically identical modulo the element type.** They are emitted from `tools/gen_sarray.py`, not typed four times. Edit the generator and re-run it; **never edit `src/array/sarray.c` or `inc/array/sarray.h` directly**, because the next generator run silently reverts the change. The same applies to `basicmathsafe` and to both generated test suites. CI runs all four generators and fails on a non-empty `git diff`, so drift is caught rather than discovered later. See `tools/README.md`.

`sarray` has no `Get`/`Set` equivalent in `sstring` because C already has `arr[i]`; `sarrayGet` exists to be the bounds checked form of it. `sarrayBinarySearch` requires a sorted array and does not verify it, because verifying costs the scan the search exists to avoid — `sarrayIsSorted` is the separate precondition check.

`smemory` is 17 functions, the untyped half of the library: bounded replacements for the `mem` family of `<string.h>`. **The line between it and `sarray` is whether the operation has to know what the bytes mean.** Copy, move, set, compare and search do not, so they take a `void*` and live here. A sum, a minimum or an ordering by magnitude does, so it lives in `sarray`. When adding a function, that question decides the module — do not add a typed operation to `smemory` or a byte-blind one to `sarray`.

Two `smemory` functions have no `<string.h>` counterpart and exist for reasons that are easy to undo by accident:

- `smemoryEqualSecure` reads all `count` bytes with no early exit, because `memcmp` on a MAC leaks the length of the matching prefix through its timing. It reports equal or not equal only — producing an ordering means finding the first difference, and finding the first difference *is* the leak. Its operands are read through `const volatile unsigned char*` so the compiler cannot restore the early exit. Do not "optimise" that loop.
- `smemoryClearSecure` writes through `volatile`, which `smemoryClear` does not. `smemoryClear` is an ordinary store and a dead-store eliminator may delete it entirely. Both exist on purpose; the Doxygen on each says which to use.

`smemory` carries a second MISRA deviation `sstring` and `sarray` do not: Rule 11.5, `void*` to object pointer. It is confined to `unsigned char*`, the one object type the standard always permits for examining an object's bytes.

`basicmathsafe` is 68 functions across the same four numeric families as `sarray`. Its single rule: **every check happens before the operation, never after.** There is no place in it where a result wraps and is then inspected — for a signed type that inspection is already undefined behaviour, and for an unsigned one the wrapped value carries no evidence that it wrapped. Overflow is detected by division rather than by a wider intermediate, so only `Scale` and `Average` need 64-bit arithmetic.

The checked and saturating forms of add, subtract and multiply share one status helper each, so `basicmathsafeAddSat` saturates exactly where `basicmathsafeAdd` reports `BM_OVERFLOW`. Keep it that way: two independent boundary tests will disagree eventually.

`selfdiagsafe` is 16 functions. Two of its features hold state, in caller-owned structs (`selfdiagsafeflow_t`, `selfdiagsafeshadow_t`) so the functions stay reentrant; the rest are stateless. `sring` is the module actually built around the driver-struct pattern. Its banner has claimed "without hardware dependencies" since 2022 and that is the scope: CRC and checksum, March memory tests, stack usage measurement, control-flow signatures, redundant storage. **A CPU register test, a program counter test and an instruction set test are deliberately absent** — they cannot be written in C at all, and a complete IEC 61508 / ISO 26262 self test needs assembly for them. Do not add a HAL or a linker symbol to this module to close that gap; it belongs in a separate, non-portable one.

`selfdiagsafe` has one deliberate inversion of the library-wide rule that outputs are written only on success: the `failIndex` of both memory tests is written **only** on `SD_FAILED`. On a memory test the failing address is the entire result and there is nothing to report when nothing failed.

`sring` is 12 functions and is the **reference for the driver-struct pattern** — the first module built around it rather than merely holding a struct. A single-producer single-consumer byte ring, lock-free for the case it exists for: an interrupt filling it while the main loop drains it.

Its whole safety argument rests on one property, and any change has to preserve it: **the producer writes only `writeIndex`, the consumer writes only `readIndex`, and neither index is ever written by both sides.** `sringPut`/`sringPutBlock` are the producer; `sringGet`/`sringGetBlock`/`sringClear` are the consumer. `sringClear` moves `readIndex` up to `writeIndex` rather than zeroing both, precisely so it stays on the consumer side. The test suite checks this structurally, because every functional case still passes when it is broken.

Two things follow from that design and are not negotiable:

- **One byte of the buffer is never used.** Full and empty are otherwise the same state, and telling them apart with a count needs a field both sides write — which is exactly what makes a ring need a lock. A 64-byte buffer holds 63. Do not "fix" this.
- **`volatile` is not a memory barrier.** It stops the compiler caching an index; it does not stop the processor completing the two stores out of order, and the byte must land before the index that publishes it. Enough on Cortex-M0/M0+/M3/M4, not enough on M7 or anything multi-core. The barrier cannot be issued from library code (it is an intrinsic, and this library includes no vendor header), so it is **injected at `Init` as a function pointer** — the driver-struct rule applied literally. It fires after the data moves and before the index does, and never on a refused operation.

Two producers, or two consumers, are unsafe and documented as such.

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
