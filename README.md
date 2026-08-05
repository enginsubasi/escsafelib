# escsafelib

[![CI](https://github.com/enginsubasi/escsafelib/actions/workflows/ci.yml/badge.svg)](https://github.com/enginsubasi/escsafelib/actions/workflows/ci.yml)

A generic C library to design safety related applications.

github.com/enginsubasi/escsafelib/

Freestanding C99. No heap, no OS, no `<string.h>`. The caller owns all
storage. Every pointer parameter is followed by the capacity of the buffer
it points at, every loop bound comes from a parameter, and a function that
fails leaves its destination untouched.

## Modules

| Module | State |
|---|---|
| `inc/string/sstring.h` | 41 functions. Bounded replacements for `<string.h>`, plus tokenizing, transforms, validation and number conversion. |
| `inc/array/sarray.h` | 92 functions. Twenty three bounded array operations in four element families, `uint8_t`, `uint16_t`, `uint32_t` and `int32_t`. |
| `inc/memory/smemory.h` | 17 functions. Bounded replacements for the `mem` family of `<string.h>`, plus a constant time comparison and an erase the compiler may not remove. |
| `inc/math/smath.h` | 68 functions. Checked and saturating arithmetic, safe division, scaling, clamping and range tests in the same four numeric families. |
| `inc/ring/sring.h` | 12 functions. Single producer single consumer byte ring buffer. Lock free for an interrupt filling it while the main loop drains it. |
| `inc/filter/sfilter.h` | 19 functions. Moving average, exponential average, debounce, slew limit, hysteresis and median. |
| `inc/fixed/sfixed.h` | 19 functions. Q16.16 fixed point: conversion, checked arithmetic, rounding, interpolation and square root. No floating point. |
| `inc/scale/sscale.h` | 12 functions. Piecewise linear scaling: a validated breakpoint table in either direction, its inverse, and the two point map that needs no table. |
| `inc/vote/svote.h` | 12 functions. Redundant channel voting: agreement within a tolerance, majority, median, mean, spread, outlier reporting and fail safe selection. |
| `inc/diag/sdiag.h` | 16 functions. CRC and checksum, March memory tests, stack usage measurement, control flow monitoring and redundant storage. No hardware dependency. |

`sarray` and `smemory` split the same territory along one line. Operations
that do not need to know what the bytes mean live in `smemory` and take a
`void*`. Operations that interpret a value, such as a sum, a minimum or an
ordering by magnitude, need the element type and live in `sarray`.

Every size in `sstring` and `smemory` is a byte count. Every size in
`sarray` is an element count.

## What it looks like

Every function returns a status, and on any status other than success the
output is left exactly as the caller had it. That is the whole idea: a
caller that ignores the return value reads its own variable rather than a
wrong answer that looks like a right one.

```c
#include <stddef.h>     /* for NULL; the library headers pull in stdint.h only */
#include <stdint.h>

#include "sring.h"
#include "sstring.h"
#include "smath.h"

static uint8_t      rxStorage[ 128 ];
static sringu8_t    rxRing;
static uint32_t     overruns = 0;

void uartSetup ( void )
{
    /* A NULL barrier is correct on a Cortex-M0 to M4 and wrong on an M7.
       See the note in sring.c before choosing. */
    ( void ) sringInitu8 ( &rxRing, rxStorage, ( uint32_t ) sizeof ( rxStorage ), NULL );
}

/* In the UART interrupt. A full ring is an event worth counting, not a
   detail to swallow: the link is arriving faster than the loop drains it. */
void uartOnByte ( uint8_t byte )
{
    if ( sringPutu8 ( &rxRing, byte ) == SR_FULL )
    {
        ++overruns;
    }
    else
    {
        // Intentionally blank.
    }
}

/* In the main loop. Every step is checked, and *rpm is written only if all
   of them succeed. */
uint8_t readTargetRpm ( uint32_t* rpm )
{
    uint8_t retVal = FALSE;
    uint8_t text[ 8 ];
    uint32_t waiting = 0;
    uint32_t raw = 0;
    uint32_t scaled = 0;
    uint8_t inRange = FALSE;

    if ( sringCountu8 ( &rxRing, &waiting ) != SR_OK )
    {
        retVal = FALSE;
    }
    else if ( waiting < 4u )
    {
        retVal = FALSE;
    }
    /* All four bytes or none, so a half arrived message is never parsed. */
    else if ( sringGetBlocku8 ( &rxRing, text, ( uint32_t ) sizeof ( text ), 4u ) != SR_OK )
    {
        retVal = FALSE;
    }
    else
    {
        text[ 4 ] = '\0';

        /* Bounded: an unterminated or malformed field is a status, not a
           runaway scan. Overflow is caught before the multiply causes it. */
        if ( sstringToU32 ( ( const char* ) text, 5u, &raw ) != SS_OK )
        {
            retVal = FALSE;
        }
        /* raw * 3000 overflows a uint32_t long before the divide brings it
           back. Written out by hand this line is silently wrong. */
        else if ( smathScaleu32 ( raw, 3000u, 1000u, &scaled ) != SH_OK )
        {
            retVal = FALSE;
        }
        else if ( smathInRangeu32 ( scaled, 500u, 6000u, &inRange ) != SH_OK )
        {
            retVal = FALSE;
        }
        else if ( inRange == FALSE )
        {
            retVal = FALSE;
        }
        else
        {
            *rpm = scaled;
            retVal = TRUE;
        }
    }

    return ( retVal );
}
```

## Building

There is no build system. Copy the header and source pair of the module
you need into your project.

## Tests

`test/<Name>_Test/` holds one self checking `main()` per module. It returns
a non zero exit code when a case fails.

```bash
gcc -Wall -Wextra -Wpedantic -std=c99 -Iinc/string \
  test/SString_Test/SString_Test.c src/string/sstring.c -o sstring_test && ./sstring_test

gcc -Wall -Wextra -Wpedantic -std=c99 -Iinc/array \
  test/SArray_Test/SArray_Test.c src/array/sarray.c -o sarray_test && ./sarray_test

gcc -Wall -Wextra -Wpedantic -std=c99 -Iinc/memory \
  test/SMemory_Test/SMemory_Test.c src/memory/smemory.c -o smemory_test && ./smemory_test

gcc -Wall -Wextra -Wpedantic -std=c99 -Iinc/math \
  test/SMath_Test/SMath_Test.c src/math/smath.c \
  -o smath_test && ./smath_test

gcc -Wall -Wextra -Wpedantic -std=c99 -Iinc/diag \
  test/SDiag_Test/SDiag_Test.c src/diag/sdiag.c \
  -o sdiag_test && ./sdiag_test

gcc -Wall -Wextra -Wpedantic -std=c99 -Iinc/ring \
  test/SRing_Test/SRing_Test.c src/ring/sring.c -o sring_test && ./sring_test

gcc -Wall -Wextra -Wpedantic -std=c99 -Iinc/filter \
  test/SFilter_Test/SFilter_Test.c src/filter/sfilter.c -o sfilter_test && ./sfilter_test

gcc -Wall -Wextra -Wpedantic -std=c99 -Iinc/fixed \
  test/SFixed_Test/SFixed_Test.c src/fixed/sfixed.c -o sfixed_test && ./sfixed_test

gcc -Wall -Wextra -Wpedantic -std=c99 -Iinc/scale \
  test/SScale_Test/SScale_Test.c src/scale/sscale.c -o sscale_test && ./sscale_test

gcc -Wall -Wextra -Wpedantic -std=c99 -Iinc/vote \
  test/SVote_Test/SVote_Test.c src/vote/svote.c -o svote_test && ./svote_test
```

## Generated modules

`sarray` and `smath` are each one design repeated across four
numeric types, so their C is emitted from a template in `tools/` rather
than typed out four times. The generated C is what ships and what you copy
into a project; nothing in `tools/` has to exist for the library to be
used. See `tools/README.md` before changing either module.

## What this library does not do

`sdiag` covers the self tests that can be written in portable C. A
CPU register test, a program counter test and an instruction set test
cannot be, because C gives no way to name a register or to guarantee an
instruction is issued. A complete IEC 61508 or ISO 26262 self test needs
assembly for those.

## Where the verification stops

Every module cross compiles for `arm-none-eabi` with `-Wall -Wextra
-Wpedantic` and passes the gcc analyzer, and every test suite passes on a
host under both gcc and clang, at a warning level well beyond the project's
own, and under UBSan.

**None of it has ever run on an ARM target.** Building clean for a target is
not the same as behaving on one, and the difference is not only theoretical
here. The suites cannot see an unaligned access fault, anything that depends
on the target's word size or padding, the memory ordering assumption behind
the `sring` barrier, or the real timing of the constant time comparison in
`smemory` on a core with a cache.

Treat host verification as what it is. An integrator putting this on a
target owns the on target validation.

# Coding Reference

The library coding references are defined in codingReference.md file.
