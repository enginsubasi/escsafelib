# sstring — Safe String Module Design

Date: 2026-08-02
Status: implemented

This document describes the module as built. It was written before implementation and revised afterwards, because implementation found a defect in the original design — see *What changed during implementation*.

## Problem

`<string.h>` is unusable in a safety context. Its functions trust their inputs in ways that cannot be checked from the outside:

| Function | Failure mode |
|---|---|
| `strlen` | Scans until it finds a zero byte. A source without a terminator runs past the end of the buffer until it faults or wraps. There is no upper bound on the scan. |
| `strcpy`, `strcat` | Write until the source terminates. The destination capacity is never passed in, so overflow cannot be detected, only avoided by the caller getting the arithmetic right. |
| `strncpy` | Does not terminate the destination when the source is exactly `n` bytes or longer. Silently truncates otherwise. Two separate traps in one function. |
| `strtok` | Holds parser state in a static, so it is not reentrant, and it mutates the source string by writing terminators into it. |
| `atoi`, `strtol` | Undefined behaviour on overflow, or diagnosis only through `errno`. |
| all of them | Overlapping source and destination is undefined behaviour and corrupts data silently. |

## Design decisions

1. **Flat functions, no driver struct.** These are stateless transformations on caller memory. The `esclib` driver pattern buys nothing here.
2. **Status enum return, not `TRUE`/`FALSE`.** A caller must be able to tell "you passed NULL" from "it did not fit" from "your source is not terminated". The three demand different fixes. This is a documented deviation from `codingReference.md`; the enum is still returned as `uint8_t`, matching the `esclib` `enum BUFFERSTATUS` pattern.
3. **On failure the destination is left untouched.** No partial write, no truncation, no clearing.
4. **Every pointer parameter is immediately followed by the capacity of the buffer it points at.** A scan bound is never inferred from a different buffer. This is the decision that replaced the original one, which derived the source bound from `destSize`.
5. **Overlap is detected, not documented away**, by comparing addresses through `uintptr_t`.

## Module contract

Six invariants. Every function obeys all six.

**1. Every loop bound comes from a parameter.** The module contains no data-driven loop. Every loop is `for ( i = 0; i < bound; ++i )` where `bound` is a `uint32_t` the caller supplied or that was derived from one. An unterminated or corrupt input causes a bounded read and a status code, never a runaway scan.

**2. Every pointer carries its own capacity,** and nothing outside that capacity is read or written.

**3. Validate, then commit.** A writing function completes every check before it writes the first byte. On any status other than `SS_OK` the destination is bit-for-bit unchanged. Where the final size is not known up front the result is built in a local scratch array first — `sstringFromI32` does this so that the overflow path cannot leave half a number behind.

**4. Output parameters are written only on `SS_OK`.**

**5. No module state.** Every function is reentrant.

**6. Freestanding.** `<stdint.h>` and `<stddef.h>` only. No allocation, no `assert`, no logging.

The library cannot discover how large a buffer really is; C does not carry that information. The guarantee is that nothing outside the capacity the caller *declares* is touched. A caller that declares a capacity larger than the allocation defeats it.

## Status codes

```c
enum SSTRINGSTATUS
{
    SS_OK            = 0,   /* Operation completed. Outputs are valid.   */
    SS_NULLPTR       = 1,   /* A required pointer argument was NULL.     */
    SS_INVALIDSIZE   = 2,   /* A size or bound argument was zero.        */
    SS_OVERFLOW      = 3,   /* Result does not fit into the destination. */
    SS_UNTERMINATED  = 4,   /* No terminator within the buffer.          */
    SS_NOTFOUND      = 5,   /* Search completed, no match.               */
    SS_OUTOFRANGE    = 6,   /* Index or start position past the string.  */
    SS_OVERLAP       = 7,   /* Source and destination overlap unsafely.  */
    SS_INVALIDFORMAT = 8,   /* Text is not valid for the conversion.     */
};
```

`SS_NOTFOUND` is a normal outcome of a search, not an error. It is still not `SS_OK`, so a caller cannot use the index without having looked at the status.

## Overlap

Two `static` helpers.

`isOverlapping` converts both pointers to `uintptr_t` and reports whether the ranges intersect. A range whose end address would wrap is reported as intersecting, because the arithmetic cannot be trusted and refusing is the safe direction to fail in. Comparing pointers into unrelated objects is not defined by the standard; on a flat address space — every target this library is meant for — it is exact. This is a **MISRA C:2012 Rule 11.4 deviation**, confined to that one function and recorded in the file banner.

`isUnsafeOverlap` adds direction. For a transform whose output byte *i* comes from input byte *i* of the read range, overlap alone is not a defect — what matters is whether the write position runs ahead of the read position. While `dest <= readStart`, every byte the write lands on has already been consumed, so converting a string in place or shifting one towards the front of its own buffer is correct and is allowed. Only a destination that runs ahead is refused.

`sstringCopy` and `sstringConcat` refuse all overlap regardless; `sstringMove` is the documented way to copy between overlapping buffers, and it chooses its direction from the addresses.

## API

All sizes are byte counts of the whole buffer, terminator included. A `destSize` of 8 holds a string of at most 7 characters.

### Core

```c
uint8_t sstringLength       ( const char* str, uint32_t strSize, uint32_t* length );
uint8_t sstringRequiredSize ( const char* src, uint32_t srcSize, uint32_t* required );
uint8_t sstringCopy         ( char* dest, uint32_t destSize, const char* src, uint32_t srcSize );
uint8_t sstringCopyN        ( char* dest, uint32_t destSize, const char* src, uint32_t srcSize, uint32_t count );
uint8_t sstringMove         ( char* dest, uint32_t destSize, const char* src, uint32_t srcSize );
uint8_t sstringConcat       ( char* dest, uint32_t destSize, const char* src, uint32_t srcSize );
uint8_t sstringConcatN      ( char* dest, uint32_t destSize, const char* src, uint32_t srcSize, uint32_t count );
uint8_t sstringCompare      ( const char* a, uint32_t aSize, const char* b, uint32_t bSize, int32_t* result );
uint8_t sstringCompareN     ( const char* a, uint32_t aSize, const char* b, uint32_t bSize, uint32_t count, int32_t* result );
uint8_t sstringClear        ( char* dest, uint32_t destSize );
uint8_t sstringClearSecure  ( char* dest, uint32_t destSize );
```

`sstringCopyN` avoids both `strncpy` traps: the destination is always terminated, and a result that does not fit is refused rather than silently truncated. `count` is capped by `srcSize`, so an oversized `count` cannot become a read past the end of the source.

`sstringRequiredSize` is what a caller uses after `SS_OVERFLOW`, so that growing the destination is a calculation rather than a guess.

`sstringClear` is documented as **not** a secure erase — when the buffer is never read again the compiler may remove the loop as a dead store, and at higher optimisation levels it does. `sstringClearSecure` writes through a `volatile` pointer so each store is an observable side effect. It cannot erase copies left in registers or spill slots; no portable C construct can.

### Search and tokenize

```c
uint8_t sstringFindChar     ( const char* str, uint32_t strSize, char ch, uint32_t* index );
uint8_t sstringFindLastChar ( const char* str, uint32_t strSize, char ch, uint32_t* index );
uint8_t sstringFindString   ( const char* str, uint32_t strSize, const char* needle, uint32_t needleSize, uint32_t* index );
uint8_t sstringFindAny      ( const char* str, uint32_t strSize, const char* set,    uint32_t setSize,    uint32_t* index );
uint8_t sstringSpan         ( const char* str, uint32_t strSize, const char* set,    uint32_t setSize,    uint32_t* length );
uint8_t sstringSpanNot      ( const char* str, uint32_t strSize, const char* set,    uint32_t setSize,    uint32_t* length );
uint8_t sstringCountChar    ( const char* str, uint32_t strSize, char ch, uint32_t* count );
uint8_t sstringToken        ( const char* str, uint32_t strSize, const char* delims, uint32_t delimsSize,
                              uint32_t* cursor, uint32_t* start, uint32_t* length );
```

`sstringToken` closes all three `strtok` defects: the parser position lives in a caller-owned `cursor` rather than a static, the source is `const` and is never cut apart, and the token is reported as a start offset plus a length. It is reentrant, and two strings can be tokenized at once — the test suite does exactly that.

### Transform and validate

```c
uint8_t sstringSubstring   ( char* dest, uint32_t destSize, const char* src, uint32_t srcSize, uint32_t start, uint32_t count );
uint8_t sstringTrim        ( char* dest, uint32_t destSize, const char* src, uint32_t srcSize );
uint8_t sstringTrimLeft    ( char* dest, uint32_t destSize, const char* src, uint32_t srcSize );
uint8_t sstringTrimRight   ( char* dest, uint32_t destSize, const char* src, uint32_t srcSize );
uint8_t sstringToUpper     ( char* dest, uint32_t destSize, const char* src, uint32_t srcSize );
uint8_t sstringToLower     ( char* dest, uint32_t destSize, const char* src, uint32_t srcSize );
uint8_t sstringReplaceChar ( char* dest, uint32_t destSize, const char* src, uint32_t srcSize, char from, char to );
uint8_t sstringReverse     ( char* dest, uint32_t destSize, const char* src, uint32_t srcSize );
uint8_t sstringCompareCI   ( const char* a, uint32_t aSize, const char* b, uint32_t bSize, int32_t* result );
uint8_t sstringStartsWith  ( const char* str, uint32_t strSize, const char* prefix, uint32_t prefixSize, uint8_t* result );
uint8_t sstringEndsWith    ( const char* str, uint32_t strSize, const char* suffix, uint32_t suffixSize, uint8_t* result );

uint8_t sstringIsPrintableAscii ( const char* str, uint32_t strSize, uint8_t* result );
uint8_t sstringIsNumeric        ( const char* str, uint32_t strSize, uint8_t* result );
uint8_t sstringIsAlpha          ( const char* str, uint32_t strSize, uint8_t* result );
uint8_t sstringIsAlphaNumeric   ( const char* str, uint32_t strSize, uint8_t* result );
uint8_t sstringIsHex            ( const char* str, uint32_t strSize, uint8_t* result );
```

Case handling is ASCII only. There is no locale and no `<ctype.h>` dependency; bytes above 0x7F are left alone, which keeps UTF-8 sequences intact.

`sstringReplaceChar` refuses `to == '\0'` with `SS_INVALIDFORMAT`. Replacing with the terminator would cut the string short and leave the bytes after the cut unreachable, which is a silent truncation of exactly the kind this module exists to prevent.

`sstringReverse` is the one transform that refuses an aliasing destination, because output byte *i* depends on input byte `srcLen - 1 - i`.

Every predicate reports `FALSE` for an empty string, so that a caller validating input does not accept nothing as valid.

### Number conversion

```c
uint8_t sstringToU32     ( const char* str, uint32_t strSize, uint32_t* value );
uint8_t sstringToI32     ( const char* str, uint32_t strSize, int32_t*  value );
uint8_t sstringToU32Hex  ( const char* str, uint32_t strSize, uint32_t* value );
uint8_t sstringFromU32   ( char* dest, uint32_t destSize, uint32_t value );
uint8_t sstringFromI32   ( char* dest, uint32_t destSize, int32_t  value );
uint8_t sstringFromU32Hex( char* dest, uint32_t destSize, uint32_t value, uint8_t digits );
```

Overflow is detected *before* it happens, by testing `accumulator > ( ( UINT32_MAX - digit ) / 10 )` before the multiply, so the overflowing operation is never executed. The result is `SS_OVERFLOW`; there is no undefined behaviour and no `errno`.

The parsers are strict: no leading whitespace, no `0x` prefix, no trailing text. `sstringToI32` accepts a leading `-` or `+` and nothing else. Its magnitude is accumulated in a `uint32_t` and range-checked before it is given a sign, which is what lets `-2147483648` be accepted without ever computing its positive counterpart.

## What changed during implementation

The original design derived the source scan bound from `destSize`, on the reasoning that scanning a source beyond the destination capacity is pointless because anything found there cannot fit anyway. The stated cost was diagnostic: `SS_OVERFLOW` and `SS_UNTERMINATED` became indistinguishable.

The real cost was worse. `destSize` says nothing about how large the *source* is. A four-byte unterminated source copied into a 200-byte destination caused a 200-byte scan of a four-byte buffer. A guard-page harness — source placed at the end of a `VirtualAlloc` page with the next page marked `PAGE_NOACCESS` — reproduced this as a segmentation fault. That is the defect decision 4 now prevents.

Giving the source its own capacity also removed the diagnostic cost that motivated the original choice: a scan that runs out at `srcSize` means the source is malformed (`SS_UNTERMINATED`), while one that runs out at the destination space means the destination is too small (`SS_OVERFLOW`). The `static` helper `sourceLength` makes that distinction in one place.

A second correction came from the tests. The first overlap rule allowed aliasing only when `dest == src` exactly, which rejected `sstringSubstring ( buf, n, buf, n, 2, 4 )` — shifting a string towards the front of its own buffer, which is safe. The rule is direction, not identity: safe while the write position stays at or behind the read position.

## Verification

**Tests.** `test/SString_Test/SString_Test.c` — 421 self-checking cases, exit code non-zero on any failure. Comparison helpers are written by hand rather than taken from `<string.h>` or from `sstring` itself, because a module cannot be its own oracle. Every failure case asserts that the destination is byte-identical to a snapshot taken before the call, which is the validate-then-commit invariant and is not something a printed transcript would show.

Coverage per function: NULL for each pointer, zero capacity, exact fit, one byte too long, unterminated source, oversized `count`, overlapping buffers, in-place aliasing, and the empty string. Conversions additionally cover the exact boundary values, one past them, and a round trip.

**Toolchain.** There is no `gcc`, `clang` or MSVC on the development machine. The host compiler is zig (`python -m pip install ziglang`, then `python -m ziglang cc`).

| Check | Result |
|---|---|
| `arm-none-eabi-gcc -Wall -Wextra -Wpedantic -std=c99` | clean |
| `arm-none-eabi-gcc -fanalyzer` | clean |
| Test suite, 421 cases | 0 failed |
| UBSan (`-fsanitize=undefined -fsanitize-trap=undefined`) | 0 failed, no trap |
| All module headers in one translation unit | clean |
| Guard-page over-read harness | no fault after the fix |

**AddressSanitizer does not link** under zig on this Windows target; the `__asan_*` runtime symbols are missing. The guard-page harness covers the over-read class that ASan would otherwise catch.

**The bounded-loop invariant** is checked by reading, not by a tool. A loop whose bound is not traceable to a parameter is a defect regardless of whether a test catches it.

## Out of scope

- **The `mem*` family** — `memcpy`, `memmove`, `memset`, `memcmp`, `memchr` operate on raw blocks with no terminator concept, so their contract differs from every function here. They belong in a separate `inc/memory/smemory` module.
- **`strcoll`, `strxfrm`, `strerror`** — these need a locale or `errno`, which do not exist on a freestanding target.
- **UTF-8 and wide characters** — every function is byte oriented. Copy, concatenate, compare and search are safe on UTF-8 because no decision is split across bytes. `sstringLength` counts bytes rather than characters, the case functions leave multi-byte sequences untouched, and `sstringReverse` produces malformed output on multi-byte text. All three are documented at the call site.
