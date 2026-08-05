# tools

Developer conveniences. **Nothing here is part of a build.** The C in `inc/`
and `src/` is the source of truth, is what ships, and is what a consumer
copies into a project. No target toolchain runs any of this.

## run_all.sh

Builds and runs all six test suites, then the checks that are worth running
but belong to no single build.

```bash
bash tools/run_all.sh                        # gcc
bash tools/run_all.sh clang
bash tools/run_all.sh "python -m ziglang cc"
```

It exits non zero if anything fails. Five sections:

1. Every suite at `-Wall -Wextra -Wpedantic`, with any warning treated as a
   failure.
2. Every module under a much stricter set — `-Wconversion -Wsign-conversion
   -Wcast-qual -Wcast-align -Wshadow -Wstrict-prototypes -Wmissing-prototypes
   -Wredundant-decls -Wundef -Wwrite-strings`. All six are currently clean.
3. Whether AddressSanitizer is actually armed.
4. Every suite under UBSan in trap mode, which needs no runtime.
5. Whether the generated modules still match their generators.

**Section 3 is the one to understand.** It builds a control program that
reads past an allocation and must fault. Only if the control faults are any
ASan results believed; otherwise the whole section is skipped. That is not
caution for its own sake: under zig on Windows, `-fsanitize=address` alone
fails to link honestly, but `-fsanitize=address,undefined` links cleanly and
then detects nothing at all, so a suite built that way reports a confident
pass having been checked by nothing. Neither host compiler on the
development machine has a working ASan. The Ubuntu CI runner is the only
place it works.

## Generators

For the two modules that repeat one set of operations across several numeric
types.

These are **not** part of any build. The generated C in `inc/` and `src/` is
the source of truth, is what ships, and is what a consumer copies into a
project. Nothing here has to exist for the library to be used, and no target
toolchain ever runs Python.

They exist because `sarray` and `smath` are each one design repeated
across four element types. Typing that out four times invites the failure
where a fix lands in `sarrayFindu32` and never reaches `sarrayFindu16`.
Editing the template makes the change in all four families at once.

| Script | Produces |
|---|---|
| `gen_sarray.py` | `inc/array/sarray.h`, `src/array/sarray.c` |
| `gen_sarray_test.py` | `test/SArray_Test/SArray_Test.c` |
| `gen_smath.py` | `inc/math/smath.h`, `src/math/smath.c` |
| `gen_smath_test.py` | `test/SMath_Test/SMath_Test.c` |

Run them from the repository root:

```bash
python tools/gen_sarray.py
python tools/gen_sarray_test.py
python tools/gen_smath.py
python tools/gen_smath_test.py
```

Each writes its files with LF line endings and no other side effect.

## Changing one of these modules

Edit the generator, run it, run the test suite, and commit the generated C
along with the generator. Do not edit the generated `.c` or `.h` directly:
the next run of the generator silently reverts it.

To check that the committed C really is what the generator produces, run all
four and confirm the tree is unchanged:

```bash
python tools/gen_sarray.py && python tools/gen_sarray_test.py
python tools/gen_smath.py && python tools/gen_smath_test.py
git diff --stat
```

An empty diff means the two have not drifted apart. A non empty one means
somebody edited the output by hand, and whatever they changed is about to be
lost.

`sstring`, `smemory` and `sdiag` are single implementations with no
repetition, are written by hand, and have no generator.
