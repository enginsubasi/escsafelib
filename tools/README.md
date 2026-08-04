# tools

Generators for the two modules that repeat one set of operations across
several numeric types.

These are **not** part of any build. The generated C in `inc/` and `src/` is
the source of truth, is what ships, and is what a consumer copies into a
project. Nothing here has to exist for the library to be used, and no target
toolchain ever runs Python.

They exist because `sarray` and `basicmathsafe` are each one design repeated
across four element types. Typing that out four times invites the failure
where a fix lands in `sarrayFindu32` and never reaches `sarrayFindu16`.
Editing the template makes the change in all four families at once.

| Script | Produces |
|---|---|
| `gen_sarray.py` | `inc/array/sarray.h`, `src/array/sarray.c` |
| `gen_sarray_test.py` | `test/SArray_Test/SArray_Test.c` |
| `gen_basicmathsafe.py` | `inc/math/basicmathsafe.h`, `src/math/basicmathsafe.c` |
| `gen_basicmathsafe_test.py` | `test/BasicMathSafe_Test/BasicMathSafe_Test.c` |

Run them from the repository root:

```bash
python tools/gen_sarray.py
python tools/gen_sarray_test.py
python tools/gen_basicmathsafe.py
python tools/gen_basicmathsafe_test.py
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
python tools/gen_basicmathsafe.py && python tools/gen_basicmathsafe_test.py
git diff --stat
```

An empty diff means the two have not drifted apart. A non empty one means
somebody edited the output by hand, and whatever they changed is about to be
lost.

`sstring`, `smemory` and `selfdiagsafe` are single implementations with no
repetition, are written by hand, and have no generator.
