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
| `inc/math/basicmathsafe.h` | 68 functions. Checked and saturating arithmetic, safe division, scaling, clamping and range tests in the same four numeric families. |
| `inc/selfdiag/selfdiagsafe.h` | 16 functions. CRC and checksum, March memory tests, stack usage measurement, control flow monitoring and redundant storage. No hardware dependency. |

`sarray` and `smemory` split the same territory along one line. Operations
that do not need to know what the bytes mean live in `smemory` and take a
`void*`. Operations that interpret a value, such as a sum, a minimum or an
ordering by magnitude, need the element type and live in `sarray`.

Every size in `sstring` and `smemory` is a byte count. Every size in
`sarray` is an element count.

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
  test/BasicMathSafe_Test/BasicMathSafe_Test.c src/math/basicmathsafe.c \
  -o basicmathsafe_test && ./basicmathsafe_test

gcc -Wall -Wextra -Wpedantic -std=c99 -Iinc/selfdiag \
  test/SelfDiagSafe_Test/SelfDiagSafe_Test.c src/selfdiag/selfdiagsafe.c \
  -o selfdiagsafe_test && ./selfdiagsafe_test
```

## Generated modules

`sarray` and `basicmathsafe` are each one design repeated across four
numeric types, so their C is emitted from a template in `tools/` rather
than typed out four times. The generated C is what ships and what you copy
into a project; nothing in `tools/` has to exist for the library to be
used. See `tools/README.md` before changing either module.

## What this library does not do

`selfdiagsafe` covers the self tests that can be written in portable C. A
CPU register test, a program counter test and an instruction set test
cannot be, because C gives no way to name a register or to guarantee an
instruction is issued. A complete IEC 61508 or ISO 26262 self test needs
assembly for those.

No module has ever executed on an ARM target. Every one of them
cross compiles cleanly and passes the ARM analyzer, which is not the same
thing.

# Coding Reference

The library coding references are defined in codingReference.md file.
