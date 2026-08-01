# sstring — Safe String Module Design

Date: 2026-08-02
Status: approved

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

`sstring` replaces them with a set that cannot scan without a bound, cannot write past a declared capacity, and cannot partially write.

## Design decisions

Settled before design:

1. **Flat functions, no driver struct.** These are stateless transformations on caller memory. The `esclib` driver pattern buys nothing here.
2. **Status enum return, not `TRUE`/`FALSE`.** A caller must be able to tell "you passed NULL" from "it did not fit" from "your source is not terminated". The three demand different fixes. This is a deliberate, documented deviation from `codingReference.md`, which specifies `TRUE`/`FALSE` for status results. The enum is still returned as `uint8_t`, matching the `esclib` `circBufGetStatusu32` / `enum BUFFERSTATUS` pattern.
3. **On failure the destination is left untouched.** No partial write, no truncation, no clearing.
4. **Scan bounds come from the destination where one exists, and are explicit otherwise.** `sstringCopy` needs no `srcMaxLen` because scanning the source beyond `destSize` is pointless — anything found there cannot fit anyway. Functions with no destination (`sstringLength`, `sstringCompare`, the search family) take an explicit `maxLen`.
5. **Overlap is detected, not documented away.** `SS_OVERLAP` is a real status, checked by comparing addresses through `uintptr_t`.

## Module contract

Five invariants. Every function obeys all five, with no exceptions.

**1. Every loop bound comes from a parameter.** The module contains no data-driven loop — no `while ( *p )`, no `for ( ; *p != '\0'; ++p )`. Every loop is `for ( i = 0; i < bound; ++i )` where `bound` is a `uint32_t` the caller supplied or that was derived from one. An unterminated or corrupt input therefore causes a bounded read and a status code, never a runaway scan. This invariant is what makes the module safe; it is checked in review.

**2. Validate, then commit.** A writing function completes every check before it writes the first byte to the destination. When it returns anything other than `SS_OK` the destination is bit-for-bit unchanged. There is no partial write and no truncation.

**3. Output parameters are written only on `SS_OK`.** Same reasoning as invariant 2, applied to result pointers.

**4. Reads are bounded too.** Where a destination exists, the source is scanned across at most `destSize` bytes. Where none exists, the caller passes `maxLen`. Nothing outside that window is ever read.

**5. Freestanding.** Includes are `<stdint.h>` and `<stddef.h>` only. No `<string.h>`, no `<ctype.h>`, no `<stdlib.h>`. No allocation, no `assert`, no logging, no I/O. Failure travels through the return value and nowhere else.

## Status codes

```c
enum SSTRINGSTATUS
{
    SS_OK            = 0,   /* Operation completed. Outputs are valid.   */
    SS_NULLPTR       = 1,   /* A required pointer argument was NULL.     */
    SS_INVALIDSIZE   = 2,   /* A size or bound argument was zero.        */
    SS_OVERFLOW      = 3,   /* Result does not fit into the destination. */
    SS_UNTERMINATED  = 4,   /* No terminator within the scanned window.  */
    SS_NOTFOUND      = 5,   /* Search completed, no match.               */
    SS_OUTOFRANGE    = 6,   /* Index or start position past the string.  */
    SS_OVERLAP       = 7,   /* Source and destination buffers overlap.   */
    SS_INVALIDFORMAT = 8,   /* Text is not valid for the conversion.     */
};
```

Two notes:

`SS_NOTFOUND` is a normal outcome of a search, not an error. It is still not `SS_OK`, so a caller cannot use the index without having looked at the status.

`SS_INVALIDFORMAT` is only produced by the phase 4 conversion functions. It is defined from the start so that the numbering never shifts.

## Overlap detection

A `static` helper compares the two byte ranges:

```c
static uint8_t isOverlapping ( const void* a, uint32_t aSize, const void* b, uint32_t bSize );
```

It converts both pointers to `uintptr_t` and returns `TRUE` when the ranges intersect. Comparing pointers into unrelated objects is not defined by the standard; on a flat address space — every target this library is meant for — it is exact. The limitation is documented in the header of the `.c` file rather than hidden.

The check runs after the source length is known and before the first byte is written, so an overlapping call reads but never corrupts.

## Scope

Four phases. Phase 1 is a usable library on its own; later phases build on it. **Only phase 1 is implemented in this round** — the contract is proven on eight functions before it is copied into thirty-three.

### Phase 1 — Core

```c
uint8_t sstringLength   ( const char* str, uint32_t maxLen, uint32_t* length );
uint8_t sstringCopy     ( char* dest, uint32_t destSize, const char* src );
uint8_t sstringCopyN    ( char* dest, uint32_t destSize, const char* src, uint32_t count );
uint8_t sstringConcat   ( char* dest, uint32_t destSize, const char* src );
uint8_t sstringConcatN  ( char* dest, uint32_t destSize, const char* src, uint32_t count );
uint8_t sstringCompare  ( const char* a, const char* b, uint32_t maxLen, int32_t* result );
uint8_t sstringCompareN ( const char* a, const char* b, uint32_t count,  int32_t* result );
uint8_t sstringClear    ( char* dest, uint32_t destSize );
```

### Phase 2 — Search and tokenize

```c
uint8_t sstringFindChar     ( const char* str, uint32_t maxLen, char ch, uint32_t* index );
uint8_t sstringFindLastChar ( const char* str, uint32_t maxLen, char ch, uint32_t* index );
uint8_t sstringFindString   ( const char* str, uint32_t maxLen, const char* needle, uint32_t needleMaxLen, uint32_t* index );
uint8_t sstringFindAny      ( const char* str, uint32_t maxLen, const char* set,    uint32_t setMaxLen,    uint32_t* index );
uint8_t sstringSpan         ( const char* str, uint32_t maxLen, const char* set,    uint32_t setMaxLen,    uint32_t* length );
uint8_t sstringSpanNot      ( const char* str, uint32_t maxLen, const char* set,    uint32_t setMaxLen,    uint32_t* length );
uint8_t sstringCountChar    ( const char* str, uint32_t maxLen, char ch, uint32_t* count );
uint8_t sstringToken        ( const char* str, uint32_t maxLen, const char* delims, uint32_t delimsMaxLen,
                              uint32_t* cursor, uint32_t* start, uint32_t* length );
```

`sstringToken` replaces `strtok` and closes all three of its defects: the parser position lives in a caller owned `cursor` instead of a static, the source is `const` and is never written to, and the token is reported as a start offset plus a length rather than by cutting the input apart. It is therefore reentrant and can run over two strings at once.

### Phase 3 — Transform and validate

```c
uint8_t sstringSubstring   ( char* dest, uint32_t destSize, const char* src, uint32_t srcMaxLen, uint32_t start, uint32_t count );
uint8_t sstringTrim        ( char* dest, uint32_t destSize, const char* src, uint32_t srcMaxLen );
uint8_t sstringTrimLeft    ( char* dest, uint32_t destSize, const char* src, uint32_t srcMaxLen );
uint8_t sstringTrimRight   ( char* dest, uint32_t destSize, const char* src, uint32_t srcMaxLen );
uint8_t sstringToUpper     ( char* dest, uint32_t destSize, const char* src );
uint8_t sstringToLower     ( char* dest, uint32_t destSize, const char* src );
uint8_t sstringReplaceChar ( char* dest, uint32_t destSize, const char* src, char from, char to );
uint8_t sstringReverse     ( char* dest, uint32_t destSize, const char* src );
uint8_t sstringCompareCI   ( const char* a, const char* b, uint32_t maxLen, int32_t* result );
uint8_t sstringStartsWith  ( const char* str, uint32_t maxLen, const char* prefix, uint32_t prefixMaxLen, uint8_t* result );
uint8_t sstringEndsWith    ( const char* str, uint32_t maxLen, const char* suffix, uint32_t suffixMaxLen, uint8_t* result );

uint8_t sstringIsPrintableAscii ( const char* str, uint32_t maxLen, uint8_t* result );
uint8_t sstringIsNumeric        ( const char* str, uint32_t maxLen, uint8_t* result );
uint8_t sstringIsAlpha          ( const char* str, uint32_t maxLen, uint8_t* result );
uint8_t sstringIsAlphaNumeric   ( const char* str, uint32_t maxLen, uint8_t* result );
uint8_t sstringIsHex            ( const char* str, uint32_t maxLen, uint8_t* result );
```

Case handling is ASCII only. There is no locale and no `<ctype.h>` dependency; bytes outside `A`–`Z` and `a`–`z` are left alone.

### Phase 4 — Number conversion

```c
uint8_t sstringToU32     ( const char* str, uint32_t maxLen, uint32_t* value );
uint8_t sstringToI32     ( const char* str, uint32_t maxLen, int32_t*  value );
uint8_t sstringToU32Hex  ( const char* str, uint32_t maxLen, uint32_t* value );
uint8_t sstringFromU32   ( char* dest, uint32_t destSize, uint32_t value );
uint8_t sstringFromI32   ( char* dest, uint32_t destSize, int32_t  value );
uint8_t sstringFromU32Hex( char* dest, uint32_t destSize, uint32_t value, uint8_t digits );
```

Overflow is detected before it happens, by testing `value > ( ( UINT32_MAX - digit ) / 10 )` before the multiply, so the overflowing operation is never executed. The result is `SS_OVERFLOW`. A non-digit byte gives `SS_INVALIDFORMAT`.

## Phase 1 semantics

`maxLen` and `destSize` are always byte counts of the whole window, terminator included. A `destSize` of 8 holds a string of at most 7 characters.

### sstringLength

Scans `str` for a terminator across at most `maxLen` bytes.

| Condition | Result |
|---|---|
| `str` or `length` is NULL | `SS_NULLPTR` |
| `maxLen == 0` | `SS_INVALIDSIZE` |
| terminator found at index `i` | `*length = i`, `SS_OK` |
| no terminator in `maxLen` bytes | `SS_UNTERMINATED`, `*length` untouched |

### sstringCopy

Copies `src` into `dest`, terminator included.

Order of operations: NULL check, size check, scan `src` for a terminator across at most `destSize` bytes, overlap check, write.

| Condition | Result |
|---|---|
| `dest` or `src` is NULL | `SS_NULLPTR` |
| `destSize == 0` | `SS_INVALIDSIZE` |
| no terminator in the first `destSize` bytes of `src` | `SS_OVERFLOW` |
| ranges overlap | `SS_OVERLAP` |
| otherwise | `dest` holds the copy, `SS_OK` |

A source that is too long and a source that has no terminator at all both give `SS_OVERFLOW`. They are indistinguishable here because the scan deliberately stops at `destSize`, and that is the accepted cost of not requiring a `srcMaxLen` argument. Neither case reads past the window.

### sstringCopyN

Copies at most `count` characters from `src`, then terminates. Unlike `strncpy` it always terminates, and it never truncates silently.

Let `scanBound` be the smaller of `count` and `destSize`. The source is scanned across `scanBound` bytes.

| Condition | Result |
|---|---|
| `dest` or `src` is NULL | `SS_NULLPTR` |
| `destSize == 0` | `SS_INVALIDSIZE` |
| `count == 0` | `dest[0] = '\0'`, `SS_OK` |
| terminator found at index `i < count` | `copyLen = i` |
| no terminator within `scanBound` | `copyLen = count` |
| `copyLen + 1 > destSize` | `SS_OVERFLOW` |
| ranges overlap | `SS_OVERLAP` |
| otherwise | `copyLen` bytes plus a terminator written, `SS_OK` |

### sstringConcat

Appends `src` to the string already in `dest`.

`dest` must already hold a terminated string inside `destSize`; that is verified first, because appending to an unterminated buffer is exactly the bug this module exists to prevent.

| Condition | Result |
|---|---|
| `dest` or `src` is NULL | `SS_NULLPTR` |
| `destSize == 0` | `SS_INVALIDSIZE` |
| `dest` has no terminator within `destSize` | `SS_UNTERMINATED` |
| no terminator in the first `destSize - destLen` bytes of `src` | `SS_OVERFLOW` |
| ranges overlap | `SS_OVERLAP` |
| otherwise | `src` appended, `SS_OK` |

### sstringConcatN

As `sstringConcat`, appending at most `count` characters. The source scan window is the smaller of `count` and `destSize - destLen`.

`count == 0` still runs every check, including the terminator check on `dest`, and only then returns `SS_OK` with `dest` unchanged. It is not a shortcut that skips validation — a `dest` that is not terminated still gives `SS_UNTERMINATED`.

### sstringCompare

Compares two strings across at most `maxLen` bytes. Bytes are compared as `unsigned char`, matching `strcmp`.

| Condition | Result |
|---|---|
| `a`, `b` or `result` is NULL | `SS_NULLPTR` |
| `maxLen == 0` | `SS_INVALIDSIZE` |
| bytes differ at index `i` | `*result = (int32_t) (unsigned char) a[i] - (int32_t) (unsigned char) b[i]`, `SS_OK` |
| both terminate at the same index | `*result = 0`, `SS_OK` |
| `maxLen` exhausted, neither terminated | `SS_UNTERMINATED`, `*result` untouched |

A string that ends while the other continues is covered by the "bytes differ" row, not by the "both terminate" row: at that index one byte is `'\0'` and the other is not, so the difference is reported normally. `SS_UNTERMINATED` is reached only when neither string terminates inside the window.

### sstringCompareN

Compares at most `count` characters, following `strncmp` semantics.

There is no `SS_UNTERMINATED` case: `count` is the caller's explicit statement of how far to compare, so reaching it without a difference means the strings are equal over that range. `count == 0` gives `*result = 0` and `SS_OK`.

### sstringClear

Zero-fills `destSize` bytes of `dest`. `SS_NULLPTR` when `dest` is NULL, `SS_INVALIDSIZE` when `destSize` is zero, otherwise `SS_OK`. This is the intended way to bring a buffer into a known state before use, since no other function in the module clears anything.

## Files

```
inc/string/sstring.h            declarations only, template section layout preserved
src/string/sstring.c            implementation, all documentation
test/SString_Test/SString_Test.c  self-checking test main
```

`sstring.h` declares `enum SSTRINGSTATUS` in the `/* ENUMS */` section and the eight prototypes in `/* FUNCTION PROTOTYPES */`. It gains no typedefs and no structures.

Two existing documents change with this work: `codingReference.md` records that a module may return a prefixed status enum instead of `TRUE`/`FALSE` when the failure reason is actionable, and `CLAUDE.md` gains the `test/` directory and the way tests are run, since it currently states that no test harness exists.

## Verification

**Compilation.** `arm-none-eabi-gcc -c -Wall -Wextra -Iinc/string src/string/sstring.c -o /dev/null` produces no output. The header is also checked for coexistence with the other module headers in one translation unit.

**Tests.** `esclib` verifies tests by reading printed output against a checked-in `output.txt`. That is too weak for this module: the interesting cases are the failure paths, and a human comparing columns will not notice that `dest` was modified on a path where it should not have been.

`SString_Test` is self-checking instead. Each case asserts an expected status, an expected buffer content, and — for every failing path — that the destination is byte-identical to a copy taken before the call. It prints one line per case, a pass and fail total, and returns non-zero when anything failed. This is a deliberate deviation from the `esclib` test style, and it is the reason for it.

Coverage per function: NULL for each pointer parameter, zero size, exact fit, one byte too long, unterminated source, overlapping buffers, and the empty string.

**The bounded-loop invariant.** Every loop in `sstring.c` is read in review against invariant 1. A loop whose bound is not traceable to a parameter is a defect regardless of whether a test catches it.

## Out of scope

- **The `mem*` family** — `memcpy`, `memmove`, `memset`, `memcmp`, `memchr` operate on raw blocks with no terminator concept, so their contract differs from every function here. They belong in a separate `inc/memory/smemory` module, not in this one.
- **`strcoll`, `strxfrm`, `strerror`** — these need a locale or `errno`, which do not exist on a freestanding target.
- **UTF-8 and wide characters** — every function is byte oriented. Copy, concatenate, compare and search are safe on UTF-8 data because they never split a decision across bytes. `sstringLength` counts bytes rather than characters, and the case functions leave multi-byte sequences untouched. This is stated in the module documentation so no caller assumes otherwise.
- **Phases 2, 3 and 4** — designed here, implemented later, each in its own round.
