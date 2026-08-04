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
| `inc/math/basicmathsafe.h` | Skeleton. Not implemented yet. |
| `inc/selfdiag/selfdiagsafe.h` | Skeleton. Not implemented yet. |

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
```

# Coding Reference

The library coding references are defined in codingReference.md file.
