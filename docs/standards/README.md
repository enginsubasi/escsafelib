# Standards application notes

Reviewed on 2026-11-05 against repository base `0348352`. These notes select
the parts of the owner's standards collection that are most useful for a
freestanding, reusable C library. They are engineering summaries and review
aids, not a declaration of SIL, ASIL, certification or MISRA compliance.

## Reading guide

| Document | Use it for |
|---|---|
| [Sources](SOURCES.md) | Exact editions, local filenames, hashes and reviewed sections. |
| [IEC 61508](IEC_61508.md) | Software lifecycle evidence, reuse, diagnostics and integration constraints. |
| [ISO 26262](ISO_26262.md) | Software verification, component qualification, tool confidence and dependent failures. |
| [MISRA C:2012](MISRA_C_2012.md) | Coding-rule evidence, deviation records and limits of the current checker. |
| [Repository evidence and integration questions](EVIDENCE_MAP.md) | How these subjects relate to actual modules, tools and evidence still needed. |

IEC 61508 is the most general functional-safety reference in this selection.
ISO 26262 is useful when an automotive integrator uses the library; its
application is conditional on the item and allocated requirements. MISRA
is directly relevant to the C implementation. These are different scopes,
and one does not establish conformance to another.

## How to interpret the notes

- **Source reading** paraphrases the identified edition and gives clause
  and PDF page references. It is selective, not a complete requirements list.
- **Application to escsafelib** is our interpretation, grounded in the
  linked repository files. It is not wording imposed by the standard.
- **Evidence limit** describes what was not established by this review.
  Reading a test or a historical result is not a new test execution.

Page numbers are **one-based PDF page positions, including covers**, not
the printed page number. Source IDs resolve through `SOURCES.md`. Refer to
the original for exact wording, exceptions, normative references and
SIL/ASIL-dependent recommendations. In particular, these notes do not copy
the standards' technique-selection tables or replace their selection rules.

The reviewed PDFs remain outside the repository. Only original summaries,
source metadata and repository mappings belong here; do not add PDF copies,
page images, license watermarks or extracted full text. The existing
[coding reference](../../codingReference.md) and [project guidance](../../CLAUDE.md)
remain the authorities for implementation. These notes do not silently
change the API, approved conventions or existing deviation policy.

## Sources deferred from this pass

The collection also contains the following families. This is a selection
decision based on the file inventory; their requirements were not audited.

| Collection | Why it was deferred |
|---|---|
| EN 81 lift standards | Revisit for a specified lift safety function and product architecture. A generic C library does not establish lift-system compliance. |
| ISO 21434 and UN R155 | Revisit with a cybersecurity scope, assets and threat analysis. Secure memory helper names alone do not establish cybersecurity assurance. |
| ISO 21448 collection | Revisit for an intended-functionality or perception use case. The listed file is ISO/PAS 21448:2019 although its folder says 2022. |
| ISO 24089 and UN R156 | Revisit if the project supplies an update mechanism or release/update integration contract. |
| Other ISO 26262 parts, IEC 61508 parts 4-6 and related reports | Relevant to full lifecycle, hardware or system assessment; this pass focuses on the reusable software component. |
| Existing `ISO_26262-*_Expert_Agent.md` files | Secondary notes, not normative sources. They were not used to establish the clause interpretations here. |

## Maintaining this reference

When changing a relevant module, check the linked application notes and
update the evidence mapping if its assumptions change. Record the source
edition explicitly; do not silently substitute another edition. New claims
need a source locator and an identifiable repository revision or report.
Update the shared [handoff](../ai/HANDOFF.md) when the reference changes.

This task added documentation only. It did not run the library test suites,
recalculate coverage, qualify tools or validate a target implementation.
