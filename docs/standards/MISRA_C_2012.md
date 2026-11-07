# MISRA C:2012 application notes

The local `misrac2012_v3.pdf` identifies itself as the **March 2013**
publication of MISRA C:2012. Its PDF ISBN ends in `11-8`. The filename does
not establish that it contains the 2019 first revision or later amendments.
All rule categories and locators in this note refer to this inspected copy.
([MISRA2012](SOURCES.md#misra2012), PDF pp. 2-3.)

## What a useful compliance argument contains

**Source reading.** Sections 5.3-5.5 distinguish a complete guideline checking
matrix, tool configuration, investigation of diagnostics, documented
deviations and a project-specific compliance claim. Some checks need manual
review. Sections 6.1-6.5 distinguish directives from rules, their categories,
and limitations of automated analysis.
([MISRA2012](SOURCES.md#misra2012), PDF pp. 17-22.)

**Application to escsafelib.** A successful `tools/misra.sh` run means the
configured addon produced no findings outside the recorded module exceptions
and the occurrence-level Rule 10.4 checker baseline. It does not establish
coverage of every guideline, every target configuration, the caller's code
or every permissible use. The script is useful evidence; it is not a complete
MISRA compliance matrix.

The inspected edition distinguishes these categories:

| Category | Consequence in this edition |
|---|---|
| Mandatory | A compliance claim cannot use a deviation to excuse a violation. |
| Required | Noncompliance needs a formal justified deviation. |
| Advisory | Follow where reasonably practical; if not using the formal deviation process, retain an alternative record of noncompliance. |

Local policy may be stricter. Do not silently reclassify a rule or delete an
existing record because it was advisory in this edition. Exact requirements
and any later changes must be checked against the chosen project baseline.

## Review subjects that matter here

| Source locator | Reading and application to this library |
|---|---|
| Directive 4.1, PDF p. 32 | Consider runtime failures explicitly. Bounds, overflow and division guards are relevant, but they depend on valid pointers and truthful capacities supplied by the caller. |
| Directive 4.7, PDF p. 38 | Error information calls for meaningful handling, with the documented exception where failure is shown impossible. A caller cannot treat every non-success result as a valid computed value. |
| Rule 17.7, PDF p. 149 | Casting a return value to `void` can satisfy this rule's treatment of an intentionally discarded value. It does not, by itself, satisfy Directive 4.7's error-handling purpose. |
| Rule 10.4, PDF p. 95 | Essential-type categories matter in expressions. The library's status/enum convention is a recorded deviation, not permission for unrelated arithmetic conversions. |
| Rules 11.4-11.6, PDF pp. 105-107 | Pointer/integer and `void*` conversions need their own reasoning. The fact that C permits a conversion does not remove a MISRA restriction. |
| Section 5.2.2, PDF p. 17 | MISRA leaves purely stylistic choices to local guidance. Keep Allman braces and spacing governed by `codingReference.md`; do not call those choices MISRA requirements. |

These are selected review subjects, not a replacement list of the guidelines.
The [existing coding reference](../../codingReference.md) remains in force.

## Existing deviation records

The module-scoped exceptions below are read from [misra.sh](../../tools/misra.sh)
and the corresponding module banners. Rule 10.4's known diagnostics are pinned
by source identity in [the occurrence manifest](../../tools/misra_10_4.tsv).
The rationale column describes the repository's engineering argument; neither
the comments nor the machine filter constitute formal approval.

| Rule | Category in inspected PDF | Recorded scope | Repository rationale and review boundary |
|---|---|---|---|
| 10.4 | Required | 366 diagnostic occurrences in 346 module/function/line-hash keys (636 at baseline) | The recorded rationale is `uint8_t` status, state and flag comparisons with enum members and the `TRUE`/`FALSE` macros. Unsigned comparisons and arithmetic with unsuffixed constants were removed from the code rather than covered. The manifest detects new, changed and stale occurrences. The checker also classifies each expression: the operator at the reported column must be `==` or `!=` with a `TRUE`/`FALSE` expansion or one of the module's own enum members as a direct operand, according to cppcheck's dump. |
| 11.4 | Advisory | `sarray`, `sstring` | Pointer addresses support overlap and copy-direction checks. Confirm the target's address representation and the relevant operation. |
| 11.6 | Required | `sarray`, `smemory`, `sstring` | Address conversions through `void*` support range checks. `uintptr_t` availability alone does not prove that integer order matches the required memory-range model on every target. |
| 11.5 | Advisory | `sdiag`, `smemory` | Byte inspection uses `unsigned char*`. Keep the rationale bounded to that use; it is not a blanket permission to access arbitrary types through a converted pointer. |

**Evidence limit.** At base `0348352`, the Rule 10.4 exception was a
rule/module wildcard that could hide unrelated findings. It has since been
replaced with a function-scoped line-fingerprint and duplicate-count manifest. This narrows
the automated filter to observed source occurrences. On 2026-11-07 every
occurrence outside the status/enum/Boolean argument was removed from the
code (636 to 359), and the checker gained a line-level rationale test. Later
the same day it gained an expression-level one, which reads the operator the
addon reports from cppcheck's own token tree; all occurrences pass it (366 after the `sbits` byte order block and `ssignal`),
and a control line holding a status comparison beside an unsuffixed unsigned
one is refused at the second comparison's column. That classification is
only as good as the analyser's parse and column report: it narrows what a
reviewer has to read, and it does not replace the controlled record
described below.

For a formal claim, the existing comments would need to be part of a
controlled record identifying scope, risks, alternatives, supporting checks,
applicable configuration and review/authorization. Sections 5.3-5.4 describe
the need for investigation and authorized records. This note does not infer
that a named authority has approved them merely because the comments exist.

## Configuration and evidence boundaries

Sections 5.3.1-5.3.2 require attention to the C variant, compiler and analyzer
configuration, including implementation properties such as type sizes.
([MISRA2012](SOURCES.md#misra2012), PDF p. 18.)
The present script uses C99 and cppcheck's `unix32` platform model. Treat
that as the configured analysis model, not proof that it describes every
ARM ABI or the 64-bit Windows test environment. Review the model against
the intended compiler, integer widths, pointer representation and extensions.

The script runs the addon through a dump file and first checks that selected
deliberate violations are reported. Retain that check: the project records
a prior failure of the direct addon route. Also retain the analyzer version,
addon identity, flags, raw findings and their disposition for the actual
revision being assessed. A control covers its selected defects; it does not
establish complete guideline implementation by the addon.

No complete guideline-to-checker/manual-review matrix or formal compliance
assessment has been established. The documentation pass recorded a source
snapshot; the later filter change does not alter the selected MISRA baseline
or convert these notes into a compliance claim.
