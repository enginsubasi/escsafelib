#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Check cppcheck MISRA Rule 10.4 findings against reviewed source lines.

A finding is identified by its module, the function it sits in and the
SHA-256 of the exact text of its source line, never by its line number.
Adding a history line to a banner, or a generator moving a whole family
down the file, must not invalidate every record below it. The count
records how many findings share one key, so a second copy of a recorded
line in the same function is still a new finding.

Every finding must also sit on a line that holds what the recorded
rationale is about: TRUE, FALSE or a member of a module enum (two letter
prefix, S then one letter, and an underscore). A finding on any other line
is outside the rationale whether or not it is in the manifest. The test is
coarse on purpose: it reads the line, not the expression, so a line holding
both kinds passes it, and the manifest review is still what settles it.

Nothing here writes the manifest. A new or removed finding is reported
with the exact record that would have to be added or deleted, and a person
who has read the source line makes that edit.
"""

import argparse
import hashlib
import re
import sys
from collections import Counter
from pathlib import Path

FILE_SCOPE = "<file>"

RATIONALE = re.compile(r"\b(?:TRUE|FALSE|S[A-Z]_[A-Z0-9_]+)\b")

# A definition starts in column zero with a return type and a name followed
# by " (". The house style puts every definition this way and nothing else
# in a source file starts with an identifier in column zero.
DEFINITION = re.compile(r"^(?:static\s+)?[A-Za-z_][A-Za-z0-9_ \*]*?\b([A-Za-z_][A-Za-z0-9_]*)\s*\(")


def source_lines(path: Path) -> list:
    return path.read_bytes().splitlines()


def enclosing_function(lines: list, line: int) -> str:
    """Name of the function whose definition is the last one at or above line."""
    name = FILE_SCOPE
    for raw in lines[:line]:
        text = raw.decode("utf-8", errors="replace")
        match = DEFINITION.match(text)
        if match and not text.rstrip().endswith(";"):
            name = match.group(1)
    return name


def within_rationale(text: str) -> bool:
    return RATIONALE.search(text) is not None


def fingerprint(lines: list, line: int) -> str:
    return hashlib.sha256(lines[line - 1]).hexdigest()


def load_manifest(path: Path) -> Counter:
    records = Counter()
    for number, raw in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        if not raw or raw.startswith("#"):
            continue
        fields = raw.split("\t")
        if len(fields) != 4:
            raise ValueError(f"{path}:{number}: expected module, function, sha256, count")
        module, function, digest, count = fields
        if not re.fullmatch(r"[a-z][a-z0-9_]*", module):
            raise ValueError(f"{path}:{number}: invalid module name")
        if function != FILE_SCOPE and not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", function):
            raise ValueError(f"{path}:{number}: invalid function name")
        if len(digest) != 64 or any(c not in "0123456789abcdef" for c in digest):
            raise ValueError(f"{path}:{number}: invalid SHA-256 fingerprint")
        key = (module, function, digest)
        amount = int(count)
        if amount < 1 or key in records:
            raise ValueError(f"{path}:{number}: duplicate key or invalid count")
        records[key] = amount
    return records


def actual_records(path: Path, repo: Path) -> tuple:
    """Findings keyed as the manifest keys them, and one line number per key for the report."""
    records = Counter()
    where = {}
    outside = []
    cache = {}
    for number, raw in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        if not raw:
            continue
        fields = raw.split("\t")
        if len(fields) != 2:
            raise ValueError(f"{path}:{number}: expected module and line")
        module, line_text = fields
        line = int(line_text)
        if module not in cache:
            source = next((repo / "src").glob(f"*/{module}.c"), None)
            if source is None:
                raise ValueError(f"unknown MISRA module: {module}")
            cache[module] = source_lines(source)
        lines = cache[module]
        if line < 1 or line > len(lines):
            raise ValueError(f"{module}:{line}: source line does not exist")
        key = (module, enclosing_function(lines, line), fingerprint(lines, line))
        records[key] += 1
        where.setdefault(key, line)
        text = lines[line - 1].decode("utf-8", errors="replace").strip()
        if not within_rationale(text):
            outside.append(f"outside-rationale Rule 10.4 occurrence in {key[1]} ({module}.c:{line}): {text}")
    return records, where, outside


def compare(expected: Counter, actual: Counter, where: dict) -> list:
    errors = []
    for key in sorted(expected.keys() | actual.keys()):
        want = expected[key]
        got = actual[key]
        if want != got:
            module, function, digest = key
            state = "new/excess" if got > want else "stale/missing"
            at = f"{module}.c:{where[key]}" if key in where else f"{module}.c"
            errors.append(
                f"{state} Rule 10.4 occurrence in {function} ({at}): expected {want}, found {got}\n"
                f"    record: {module}\t{function}\t{digest}\t{got}"
            )
    return errors


def self_test() -> None:
    # The controls are assert statements, which python -O strips. Refuse
    # rather than report a pass that checked nothing.
    if not __debug__:
        raise SystemExit("MISRA occurrence controls cannot run under python -O")
    lines = [
        b"/** banner */",
        b"static uint8_t helper ( uint8_t a )",
        b"{",
        b"    if ( a == 0 )",
        b"}",
        b"uint8_t sxPublic ( const char* s,",
        b"                   uint32_t n )",
        b"{",
        b"    if ( a == 0 )",
        b"}",
    ]
    assert enclosing_function(lines, 1) == FILE_SCOPE
    assert enclosing_function(lines, 4) == "helper"
    assert enclosing_function(lines, 9) == "sxPublic"
    assert fingerprint(lines, 4) == fingerprint(lines, 9)
    assert within_rationale("if ( retVal == SA_OK )")
    assert within_rationale("else if ( isReady ( driver ) == FALSE )")
    assert within_rationale("if ( driver->state == SU_STATE_HEALING )")
    assert not within_rationale("else if ( arrSize == 0 )")
    assert not within_rationale("else if ( width > SBITS_WORDBITS )")
    assert not within_rationale("keep = i + 1;")

    key = ("sstate", "sstateGo", "a" * 64)
    expected = Counter({key: 2})
    assert compare(expected, Counter({key: 2}), {}) == []
    assert compare(expected, Counter({key: 3}), {})
    assert compare(expected, Counter({key: 1}), {})
    assert compare(expected, Counter({("sstate", "sstateGo", "b" * 64): 2}), {})
    assert compare(expected, Counter({("sstate", "sstateInit", "a" * 64): 2}), {})
    assert compare(expected, Counter({key: 2, ("sstate", "sstateGo", "c" * 64): 1}), {})


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--self-test", action="store_true")
    parser.add_argument("--repo", type=Path, default=Path(__file__).resolve().parent.parent)
    parser.add_argument("--manifest", type=Path)
    parser.add_argument("--actual", type=Path)
    parser.add_argument("--modules", nargs="*")
    args = parser.parse_args()
    if args.self_test:
        self_test()
        print("MISRA occurrence controls: pass")
        return 0
    if not args.actual or not args.manifest:
        parser.error("--manifest and --actual are required")
    actual, where, outside = actual_records(args.actual, args.repo)
    expected = load_manifest(args.manifest)
    if args.modules:
        selected = set(args.modules)
        expected = Counter({key: count for key, count in expected.items() if key[0] in selected})
        actual = Counter({key: count for key, count in actual.items() if key[0] in selected})
        outside = [item for item in outside if item.split("(")[1].split(".c:")[0] in selected]
    errors = outside + compare(expected, actual, where)
    for error in errors:
        print(error, file=sys.stderr)
    return 1 if errors else 0


if __name__ == "__main__":
    raise SystemExit(main())
