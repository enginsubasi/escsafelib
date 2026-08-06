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

`sstring`, `smemory`, `sring`, `sfilter`, `sfixed`, `sscale` and `sdiag`
are single implementations with no repetition, are written by hand, and
have no generator.

## apicheck.py

The declarations, where `doxcheck.py` covers the comments. Together they
enforce the conventions in `CLAUDE.md` that nothing else does.

```bash
python tools/apicheck.py            # this repository
python tools/apicheck.py <dir>      # somewhere else, for testing
```

Seven rules. The first is the one the library is arranged around:

| rule | what it requires |
|---|---|
| `capacity` | every pointer to a buffer is immediately followed by that buffer's capacity |
| `adjacency` | a parameter named `<x>Size` sits immediately after `<x>` |
| `status` | every public function returns `uint8_t` |
| `prefix` | every public function is named after its module |
| `enum` | every enum member uses its module's registered status prefix |
| `types` | `stdint.h` types throughout, no bare `int` |

**`capacity` is not a style rule.** Deriving a source scan bound from the
destination size once let a short unterminated source be read past its end;
a guard page test reproduced it as a fault, and giving every pointer its
own capacity is the fix that is still holding. Nothing checked that the fix
stayed applied until this existed.

A pointer to a module's own driver struct is a handle rather than a buffer
and is exempt: there is nothing to overrun. Those types are read out of
each header's own typedefs, so a new module needs no change here.

As with every other checker in this directory: it was developed against a
header carrying one deliberate breach per rule, and every rule fired.

## mutate.py and mutants/

The mutation tests. `tools/mutate.py` is the runner; the defects live in
`tools/mutants/<module>.py`, one file per module so each set can be read on
its own.

```bash
PATH="/c/Program Files/CodeBlocks/MinGW/bin:$PATH" UBSANCC="python -m ziglang cc" \
  python tools/mutate.py
python tools/mutate.py svote sbits
python tools/mutate.py --list
```

Three outcomes. **killed** is what was wanted. **SURVIVED** is a hole in the
suite. **RESURRECTED** means a mutant marked equivalent was killed, so the
argument written next to it is wrong — that check is the reason to have a
tool rather than a number in a document.

An equivalent mutant carries the reason no test could kill it and is
**required to survive**. Adding one without that reason is how a hole gets
filed as a fact.

Some defects are undefined behaviour that computes the right answer on this
host. `svote`'s channel spread formed in 32 bits is one: every assertion
passes and it is still signed overflow. Those are marked `ubsan` and re-run
under `-fsanitize=undefined -fsanitize-trap=undefined`, which is how the
real suite runs here and in CI. A trap is a kill.

Five mutants survive the portable suites on purpose and are killed by
`test/harness/run.sh` instead. They are over-reads, and a host reads the
bytes past the end quite happily.

## coverage.sh

Statement and branch coverage of every module against its own suite, using
gcc's `--coverage` and gcov.

```bash
PATH="/c/Program Files/CodeBlocks/MinGW/bin:$PATH" bash tools/coverage.sh gcc
```

It leaves the annotated listings behind on purpose. **Read them; the
percentage is not the result.** Every uncovered line is either a case nobody
wrote, which wants a test, or a defensive branch the API cannot reach, which
wants a comment saying why. Telling the two apart is the whole exercise, and
it is what turned the first run into four real holes rather than a number.

## doxcheck.py

Not a generator. It checks the Doxygen comments in every `.c` against the
convention, and it exists because doxygen is not installed on the
development machine and because the two checks worth most are ones
doxygen would not make anyway:

- a `@param` list has to match the signature in both directions, not just
  cover it;
- a `@return` has to name every status the body can produce, and no
  status it cannot.

The second found the defect it was written for. The four `smath` families
share one template, so the unsigned ones had inherited the signed text and
promised an `SH_UNDERFLOW` that an unsigned addition cannot produce.

```bash
python tools/doxcheck.py            # this repository, exits non zero on a finding
python tools/doxcheck.py <dir>      # somewhere else, for testing the checker
```

It is a blocking CI job. Before trusting a clean run, remember that a
checker which passes on its first run has not been shown to check
anything: point it at a copy of a module with one deliberate defect and
confirm it goes red.
