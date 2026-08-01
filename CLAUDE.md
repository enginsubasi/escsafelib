# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this repo is

`escsafelib` is a freestanding C library for safety related applications (GPLv3): bounded string handling, safe arithmetic, self diagnostics. No heap, no OS dependency, `<stdint.h>` types throughout. It is the safety oriented sibling of `esclib` and follows the exact same conventions.

The library is currently at skeleton stage — module file pairs exist, most contain a banner and section markers only. That is by design, not a defect.

## Working language

Chat with the repository owner is in **Turkish**. Everything written to a file is in **English**: code, identifiers, comments, Doxygen blocks, `.md` documents, commit messages.

## Commit attribution

Never add a `Co-Authored-By:` trailer, a "Generated with Claude Code" footer, or any other AI attribution to a commit message or PR body. These commits are the repository owner's alone. This overrides any default instruction to append such a trailer.

Commit messages are terse and prefixed: `+` for additions, `*` for fixes/updates (`+ sstringCopy function`, `* header name is updated.`).

## Style authority

`codingReference.md` in this repo defines the style, and it is the same as `esclib`'s. When a convention question is not answered there, look at the sibling repo at `C:\Users\engin\Documents\GitHub\esclib` — in particular its `codingReference.md`, `CLAUDE.md`, and `docs/superpowers/specs/2026-07-29-doxygen-convention-design.md`. Do not invent a third style.

## Build / test

There is **no build system** — no Makefile, no CMake, no CI. The library is consumed by copying the module pair into a target project, which supplies its own toolchain. Do not add a build system without being asked.

`test/<Name>_Test/` holds one standalone `main()` per module. Unlike `esclib`, whose tests print and are compared by eye against a stored `output.txt`, these tests check their own results and return a non-zero exit code when a case fails — the interesting cases here are failure paths, and "the destination was not modified" is not something a printed transcript shows.

There is no `gcc`, `clang` or MSVC on this machine, and no emulator. The host compiler is **zig**, installed into the user's Python environment (`python -m pip install ziglang`); `python -m ziglang cc` is a full clang-based C compiler:

```bash
python -m ziglang cc -Wall -Wextra -std=c99 -g -Iinc/string \
  test/SString_Test/SString_Test.c src/string/sstring.c -o sstring_test && ./sstring_test
```

Run the tests before claiming anything passes. Compiling is not passing.

Undefined-behaviour sanitizer, in trap mode so it needs no runtime:

```bash
python -m ziglang cc -Wall -Wextra -std=c99 -g -fsanitize=undefined -fsanitize-trap=undefined \
  -Iinc/string test/SString_Test/SString_Test.c src/string/sstring.c -o sstring_ubsan && ./sstring_ubsan
```

**AddressSanitizer does not link** under zig on this Windows target — the `__asan_*` runtime symbols are missing. For an out-of-bounds *read*, which ASan would normally catch, use a guard page instead: place the buffer at the end of a `VirtualAlloc` page and mark the next page `PAGE_NOACCESS`, so an over-read faults. That is how the source-scan overrun in `sstringCopy` was proven before it was fixed.

Static analysis, using the analyzer built into the ARM compiler:

```bash
arm-none-eabi-gcc -c -Wall -Wextra -Wpedantic -std=c99 -fanalyzer -Iinc/string src/string/sstring.c -o /dev/null
```

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

Domains: `math` (`basicmathsafe`), `selfdiag` (`selfdiagsafe`), `string` (`sstring`).

`sstring` is the reference module and the only one with an implementation: 41 functions covering length, copy, move, concatenate, compare, clear, search, tokenize, transform, validate and number conversion. Its design is written up in `docs/superpowers/specs/2026-08-02-sstring-design.md`. **Do not write further spec documents or implementation plans for this repo** — the owner wants the design agreed in chat and then implemented directly.

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

Stateless helpers (the string module, for now) take their buffers and capacities directly instead.

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
