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

## tracecheck.py

Traces every public function to the cases in its own suite that check it.
Coverage says a line ran and mutation testing says a sampled defect was
seen; neither says which documented answer of which function a suite has
ever asked for.

```bash
python tools/tracecheck.py              # check, exit non zero on a gap
python tools/tracecheck.py --matrix     # also print function, calls, statuses
python tools/tracecheck.py --self-test  # the controls
```

For each prototype it requires that the module's own suite calls it, and
that for every status its return block names (following "otherwise the
status X reports" to the delegate) some statement both calls it and names
that status, or parks its result in a local that one of the next twelve
statements compares with that status. The integration suite does not count:
a copied-out module carries only its own suite.

A status no case can expect is recorded in `trace_exceptions.tsv` with its
reason. **Every exception must still be a gap**: one that a case now covers
is reported as stale, the same rule `mutate.py` applies to an equivalent
mutant that has been killed.

The first run reported 106 gaps across 358 functions. Fourteen were sweeps
that park the status in a local, which the checker now follows. The rest:

- **`sstring`, the reference module, had never handed an unterminated
  source to twenty one functions that promise to refuse one**, nor a zero
  capacity to seventeen. A version of any of them that read past the end of
  an unterminated source would have passed the whole suite.
- **`sfixedFloor` and `sfixedRound` promised an `SX_UNDERFLOW` no input can
  produce.** All 2^32 inputs were run: -32768, the smallest value of the
  type, is a whole number, so nothing rounds below it. The promise moved to
  a `@note`, the same correction `smath` needed when `doxcheck.py` found it.
- `sfilter` discarded the success status of seven calls with `( void )`, so
  a version that produced the right value and reported a failure passed.
- `sscaleApply` and `sscaleApplyClamped` refuse a table the caller changed
  after Init, and no case had ever asked them to.
- `sarraySumu8` and `sarraySumu16` overflow: 16843009 bytes of 255 and
  65537 halfwords of 65535 land exactly on the top of the `uint32_t`
  accumulator, and one more element carries it past. The arrays hold one
  more after that, so the sum also has to stop early. Both were recorded as
  unreachable before; on a host they are only memory.
- `smemory`, `sdiag`, `svote`, `sarray`: NULL, zero capacity and overlap
  refusals that only their whole-buffer wrappers had reached.

The ten exceptions that remain are delegated statuses a wrapper cannot
reach, the two March test failures only the aliasing harness can reach,
and `sscaleFindSegment`'s defensive refusal.

The test is textual and can only err one way: a statement that names the
call and the status is not proof the two are compared. The suites all use
`expectStatus ( name, call, status )`, which is.

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
gcc's `--coverage` and gcov, and MC/DC when the compiler has
`-fcondition-coverage` (gcc 14 and later; a `gcc-NN` compiler is paired
with `gcov-NN`). The MC/DC gaps go to `<module>.conditions.txt`.

```bash
PATH="/c/Program Files/CodeBlocks/MinGW/bin:$PATH" bash tools/coverage.sh gcc
```

Under GitHub Actions it also prints the table as one `::notice`, naming
the compiler and gcov it used. The job log needs admin rights to read and
annotations do not, so that is how CI's numbers, and whether CI measured
MC/DC at all, are read from outside.

It leaves the annotated listings behind on purpose. **Read them; the
percentage is not the result.** Every uncovered line is either a case nobody
wrote, which wants a test, or a defensive branch the API cannot reach, which
wants a comment saying why. Telling the two apart is the whole exercise, and
it is what turned the first run into four real holes rather than a number.

## size.sh

What each module costs on a target, for two cores.

```bash
bash tools/size.sh
```

The first question an integrator asks about an embedded library, and the
repository could not answer it before this. The whole of it is about 25 KB
of flash on a Cortex-M0+ and 22 KB on a Cortex-M4.

**The numbers are upper bounds.** Each module is compiled whole and
measured as one object, so the figure covers every function in it whether a
caller uses them or not. With `-ffunction-sections` and `--gc-sections` a
project that calls four of `sarray`'s ninety two functions pays for four.
Read the table as the cost of taking a whole module, which is the worst
case and the easy one to compare.

It also checks something rather than only reporting it. **Every module must
show `.data` and `.bss` of zero**, because there is no module state
anywhere in this library, and a number other than zero in either column
means somebody added some. That invariant was stated in `CLAUDE.md` from
the beginning and measured for the first time here.

## misra.sh

The cppcheck MISRA C:2012 addon over every module, with the recorded
deviations applied.

```bash
bash tools/misra.sh                 # every module, exits non zero on a finding
bash tools/misra.sh sfilter svote   # some of them
bash tools/misra.sh --annotate      # findings as GitHub annotations, for CI
```

It exists because `cppcheck --addon=misra` printed nothing and returned
zero on the development machine and on the CI runner alike, for a file
built to break five rules. The script runs `cppcheck --dump` and then the
addon by hand, which works on both, and it runs that control file first
every time. If the control does not fire it exits 2 rather than report a
clean tree.

Rules 11.4, 11.5 and 11.6 remain scoped by module in the script and source
banners. Rule 10.4 uses `misra_10_4.tsv`: each record stores the module, the
function the finding sits in, the SHA-256 of the exact source line and how
many findings share that key. The line number is deliberately not part of
the key, so a new history line in a banner or a generator moving a family
down the file does not invalidate the records below it. A second copy of a
recorded line in the same function raises the count and is reported.

A full-tree run fails on new, changed or stale occurrences; a partial run
checks only the selected modules' records. Each difference is printed with
the exact record that would have to be added or deleted. The manifest pins
observed findings; it does not approve a formal deviation.

What it does check is each finding's rationale, by expression. The addon is
run with `--cli`, whose JSON carries the column the text form leaves out,
and the column names the operator the finding is about. In the cppcheck
dump that operator must be `==` or `!=`, and one of its two operands must
be `TRUE` or `FALSE` as the preprocessor expanded them, or a member of the
module's own status enum. A finding that is anything else fails the run,
manifest or not. A line test alone would pass a line holding a status
comparison and an unsuffixed unsigned one side by side; a control built
exactly that way is refused at its column.

`misra_occurrences.py --self-test` checks function attribution, matching,
added, removed, moved-to-another-function, duplicate and
changed-fingerprint cases, the expression test against a hand written dump
(including a header token at the same place as the operator), and the
JSON parsing. Nothing writes the manifest: review the source line before
adding or deleting a record by hand.

`CPPCHECK`, `MISRA_PY` and `PYTHON` override where the tools are found.

## doxcheck.py

Not a generator. It checks the Doxygen comments in every `.c` against the
convention. It was written when doxygen was not available on the
development machine, and it stays because the checks worth most are ones
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
