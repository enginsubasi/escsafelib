# Platform harnesses

Not part of any test suite. Windows only, they call `VirtualAlloc` and
`CreateFileMapping` directly, and nothing in the library needs them.

They exist because a whole class of defect is invisible to a portable test.
Reading past the end of a buffer on a host reads bytes that are there, so
the answer comes back correct and every assertion passes. The suites cannot
tell the difference; a page that faults on the read can.

```bash
PATH="/c/Program Files/CodeBlocks/MinGW/bin:$PATH" bash test/harness/run.sh
```

## What each one is for

| harness | catches |
|---|---|
| `overread.c` | a bounded read that is not bounded by the buffer it reads |

## Which mutants only die here

`tools/mutate.py` records these as surviving the portable suites, with the
reason next to each. They are killed by `run.sh`, and that is the whole
argument for keeping this directory:

| mutant | what it breaks |
|---|---|
| `sstring` M2 | the source scan bounded by the destination alone |
| `sstring` M5 | the copy scan not bounded by the destination |
| `sstring` M6 | the concatenation scan not bounded by what is left |
| `smemory` M5 | the comparison scanning to the longer of two buffers |
| `sarray` M4 | the same, in the three Compare families |

`sstring` M2 is not hypothetical. It was a real defect in this library,
found by a harness like this one, and fixing it is why every pointer
parameter now carries its own capacity.

## The rule these follow

**Nothing is reported until the mechanism has been shown to work.** Case 0
of the guard page harness deliberately reads past the end and must die. If
it lives, the page was never armed and every other result is meaningless,
so `run.sh` stops rather than printing a clean sheet. That is the same rule
the AddressSanitizer check in `tools/run_all.sh` follows, and for the same
reason: this repository has already produced one false clean result from a
sanitizer that was silently doing nothing.

**A harness can also be wrong in the direction that says everything is
fine, which is the direction that matters.** The first version of the
comparison cases filled the long buffer with zeroes. The comparison
differed at the first byte, the loop stopped there, and the harness
reported a clean run against a module it was built to catch. The two
buffers now agree over the whole of the shorter one, so nothing stops the
scan before it reaches the guard. Check a new case against a deliberately
broken module before believing it.

## Why they are not in CI

They are Windows API code and the runners are Ubuntu. The same defects are
reachable there with AddressSanitizer, which CI already runs on every
module, so nothing is lost — but ASan does not work on either compiler on
the development machine, and these do. See the note on that in `CLAUDE.md`.
