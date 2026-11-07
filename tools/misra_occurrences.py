#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Check cppcheck MISRA Rule 10.4 findings against reviewed source lines.

A finding is identified by its module, the function it sits in and the
SHA-256 of the exact text of its source line, never by its line number.
Adding a history line to a banner, or a generator moving a whole family
down the file, must not invalidate every record below it. The count
records how many findings share one key, so a second copy of a recorded
line in the same function is still a new finding.

Every finding must also be what the recorded rationale is about, and that
is checked twice. The line must hold TRUE, FALSE or a member of a module
enum (two letter prefix, S then one letter, and an underscore). Then, given
the cppcheck dumps, the expression itself: the token the addon reports
must be == or !=, and one of its two operands must be TRUE or FALSE as the
preprocessor expanded them, or a member of the module's own status enum.
The line test alone would pass a line holding both kinds of comparison;
the expression test reads the operator the finding is about.

Nothing here writes the manifest. A new or removed finding is reported
with the exact record that would have to be added or deleted, and a person
who has read the source line makes that edit.

--parse-addon turns the addon's --cli output, one JSON object per finding,
into "line rule column" lines for one module. The text output carries no
column, and the expression test needs it.
"""

import argparse
import hashlib
import json
import re
import sys
import tempfile
import xml.etree.ElementTree as ET
from collections import Counter
from pathlib import Path

FILE_SCOPE = "<file>"

RATIONALE = re.compile(r"\b(?:TRUE|FALSE|S[A-Z]_[A-Z0-9_]+)\b")
MEMBER = re.compile(r"\b(S[A-Z]_[A-Z0-9_]+)\b")
EQUALITY = ("==", "!=")
BOOLEANS = ("TRUE", "FALSE")

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


def enum_members(header: Path) -> set:
    """Every module status name a header declares. Nothing else in a header
    has the two letter prefix followed by an underscore."""
    return set(MEMBER.findall(header.read_text(encoding="utf-8")))


def load_dump(path: Path, source_name: str) -> tuple:
    """Tokens of the first configuration: by id, and the module's own by place."""
    root = ET.parse(str(path)).getroot()
    names = {f.get("index"): f.get("name") for f in root.iter("file")}
    dump = root.find("dump")
    if dump is None or dump.find("tokenlist") is None:
        raise ValueError(f"{path}: no token list")
    by_id = {}
    by_place = {}
    for token in dump.find("tokenlist"):
        by_id[token.get("id")] = token
        name = token.get("file") or names.get(token.get("fileIndex"), "")
        if Path(name.replace("\\", "/")).name != source_name:
            continue
        place = (int(token.get("linenr")), int(token.get("column")))
        by_place.setdefault(place, []).append(token)
    return by_id, by_place


def expression_within_rationale(by_id: dict, by_place: dict, line: int, column: int, members: set) -> bool:
    for op in by_place.get((line, column), []):
        if op.get("str") not in EQUALITY:
            continue
        for side in ("astOperand1", "astOperand2"):
            operand = by_id.get(op.get(side))
            if operand is None:
                continue
            if operand.get("macroName") in BOOLEANS or operand.get("str") in members:
                return True
    return False


def parse_addon(stream, module: str) -> list:
    """(line, rule, column) for each finding the addon reports in module.c."""
    found = []
    for raw in stream:
        try:
            item = json.loads(raw)
        except ValueError:
            continue
        error = str(item.get("errorId", ""))
        if "linenr" not in item or "c2012-" not in error:
            continue
        if Path(str(item.get("file", "")).replace("\\", "/")).name != f"{module}.c":
            continue
        rule = error.split("c2012-", 1)[1]
        found.append((int(item["linenr"]), rule, int(item.get("column", 0))))
    return found


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


def actual_records(path: Path, repo: Path, dumps: Path = None) -> tuple:
    """Findings keyed as the manifest keys them, and one line number per key for the report.

    Each record is module, line and column. With dumps, the column is used
    to find the operator the finding is about and check its operands.
    """
    records = Counter()
    where = {}
    outside = []
    cache = {}
    tokens = {}
    for number, raw in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        if not raw:
            continue
        fields = raw.split("\t")
        if len(fields) != 3:
            raise ValueError(f"{path}:{number}: expected module, line and column")
        module, line_text, column_text = fields
        line = int(line_text)
        column = int(column_text)
        if module not in cache:
            source = next((repo / "src").glob(f"*/{module}.c"), None)
            if source is None:
                raise ValueError(f"unknown MISRA module: {module}")
            cache[module] = source_lines(source)
            if dumps is not None:
                header = repo / "inc" / source.parent.name / f"{module}.h"
                tokens[module] = load_dump(dumps / f"{module}.c.dump", f"{module}.c") + (enum_members(header),)
        lines = cache[module]
        if line < 1 or line > len(lines):
            raise ValueError(f"{module}:{line}: source line does not exist")
        key = (module, enclosing_function(lines, line), fingerprint(lines, line))
        records[key] += 1
        where.setdefault(key, line)
        text = lines[line - 1].decode("utf-8", errors="replace").strip()
        if not within_rationale(text):
            outside.append(f"outside-rationale Rule 10.4 occurrence in {key[1]} ({module}.c:{line}): {text}")
        elif dumps is not None and not expression_within_rationale(
                tokens[module][0], tokens[module][1], line, column, tokens[module][2]):
            outside.append(f"outside-rationale Rule 10.4 occurrence in {key[1]} ({module}.c:{line}:{column}), "
                           f"by expression: {text}")
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

    # The expression test, against a dump written the way cppcheck writes
    # one: a header token at the same place must not be mistaken for the
    # operator, and an operand of the wrong kind must be refused.
    dump = (
        '<dumps><file index="0" name="dir/sxmod.c"/><file index="1" name="inc/sxmod.h"/>'
        '<dump cfg=""><tokenlist>'
        '<token id="h1" file="inc/sxmod.h" linenr="4" column="18" str="=="/>'
        '<token id="a1" file="dir/sxmod.c" linenr="4" column="10" str="state"/>'
        '<token id="o1" file="dir/sxmod.c" linenr="4" column="18" str="==" astOperand1="a1" astOperand2="t1"/>'
        '<token id="t1" file="dir/sxmod.c" linenr="4" column="21" str="1" macroName="TRUE"/>'
        '<token id="o2" file="dir/sxmod.c" linenr="5" column="18" str="!=" astOperand1="a1" astOperand2="e1"/>'
        '<token id="e1" file="dir/sxmod.c" linenr="5" column="21" str="SX_OK"/>'
        '<token id="o3" file="dir/sxmod.c" linenr="6" column="18" str="==" astOperand1="a1" astOperand2="n1"/>'
        '<token id="n1" file="dir/sxmod.c" linenr="6" column="21" str="0"/>'
        '<token id="o4" file="dir/sxmod.c" linenr="7" column="18" str="&lt;" astOperand1="a1" astOperand2="t2"/>'
        '<token id="t2" file="dir/sxmod.c" linenr="7" column="20" str="1" macroName="TRUE"/>'
        '<token id="o5" file="dir/sxmod.c" linenr="8" column="18" str="==" astOperand1="a1" astOperand2="e2"/>'
        '<token id="e2" file="dir/sxmod.c" linenr="8" column="21" str="SY_OK"/>'
        '<token id="o6" file="inc/sxmod.h" linenr="9" column="18" str="==" astOperand1="a1" astOperand2="t1"/>'
        '</tokenlist></dump></dumps>'
    )
    with tempfile.TemporaryDirectory() as scratch:
        path = Path(scratch) / "sxmod.c.dump"
        path.write_text(dump, encoding="utf-8")
        by_id, by_place = load_dump(path, "sxmod.c")
    members = {"SX_OK", "SX_NULLPTR"}
    assert expression_within_rationale(by_id, by_place, 4, 18, members)
    assert expression_within_rationale(by_id, by_place, 5, 18, members)
    assert not expression_within_rationale(by_id, by_place, 6, 18, members)
    assert not expression_within_rationale(by_id, by_place, 7, 18, members)
    assert not expression_within_rationale(by_id, by_place, 8, 18, members)
    assert not expression_within_rationale(by_id, by_place, 9, 18, members)
    assert not expression_within_rationale(by_id, by_place, 4, 19, members)

    cli = [
        '{"addon": "misra", "severity": "none", "message": "Misra C: 10.4", "errorId": "logChecker"}',
        '{"file": "C:/w/sxmod.c", "linenr": 4, "column": 18, "addon": "misra", "errorId": "c2012-10.4"}',
        '{"file": "C:/w/other.c", "linenr": 4, "column": 18, "addon": "misra", "errorId": "c2012-10.4"}',
        '{"file": "C:\\\\w\\\\sxmod.c", "linenr": 9, "column": 2, "addon": "misra", "errorId": "misra-c2012-11.5"}',
        'Checking sxmod.c ...',
    ]
    assert parse_addon(cli, "sxmod") == [(4, "10.4", 18), (9, "11.5", 2)]

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
    parser.add_argument("--dumps", type=Path, help="directory holding <module>.c.dump")
    parser.add_argument("--parse-addon", metavar="MODULE")
    args = parser.parse_args()
    if args.self_test:
        self_test()
        print("MISRA occurrence controls: pass")
        return 0
    if args.parse_addon:
        for line, rule, column in parse_addon(sys.stdin, args.parse_addon):
            print(line, rule, column)
        return 0
    if not args.actual or not args.manifest:
        parser.error("--manifest and --actual are required")
    actual, where, outside = actual_records(args.actual, args.repo, args.dumps)
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
