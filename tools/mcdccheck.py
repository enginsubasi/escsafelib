#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Check every MC/DC gap gcov reports against the reviewed list of them.

tools/coverage.sh measures MC/DC with gcc's -fcondition-coverage. A gap is
either a case nobody wrote, which wants a test, or a condition the API
cannot reach, which wants a reason. The reasons used to be prose in
CLAUDE.md, which nothing compared with the next measurement. They are now
records in tools/mcdc_exceptions.tsv:

    module  function  sha256(source line)  condition  count  reason

keyed like the Rule 10.4 manifest, by the function a gap sits in and the
hash of its line rather than a line number, so a banner line added above
it changes nothing. The count is how many gaps share one key: the same
line written more than once in one function, as the March test's passes
are. The condition is gcov's own, such as c1=true: the second condition
never shown to decide the outcome as true.

This fails, exit 1, on a gap no record explains, which is a missing case
or an unreviewed one, and on a record no gap needs any more, which is a
claim that has gone stale. Nothing here writes the manifest: a new record
is printed for a person to check and add with its reason.

    python tools/mcdccheck.py --dir <coverage output>   # after coverage.sh
    python tools/mcdccheck.py --self-test
"""

import argparse
import hashlib
import re
import sys
import tempfile
from collections import Counter
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
FILE_SCOPE = "<file>"
DEFINITION = re.compile(r"^(?:static\s+)?[A-Za-z_][A-Za-z0-9_ \*]*?\b([A-Za-z_][A-Za-z0-9_]*)\s*\(")
SOURCE = re.compile(r"^\s*[^:]+:\s*(\d+):(.*)$")
GAP = re.compile(r"^condition\s+(\d+) not covered \((true|false)\)")


def enclosing_function(lines, line):
    """Name of the last definition at or above a line, as misra_occurrences finds it."""
    name = FILE_SCOPE
    for raw in lines[:line]:
        text = raw.decode("utf-8", errors="replace")
        match = DEFINITION.match(text)
        if match and not text.rstrip().endswith(";"):
            name = match.group(1)
    return name


def gaps_in(gcov_text, module, source_lines):
    """Every MC/DC gap in one gcov listing, counted by manifest key, and a line for each key."""
    found = Counter()
    where = {}
    line = 0
    for raw in gcov_text.splitlines():
        source = SOURCE.match(raw)
        if source and int(source.group(1)) > 0:
            line = int(source.group(1))
            continue
        gap = GAP.match(raw)
        if gap and line > 0:
            key = (module,
                   enclosing_function(source_lines, line),
                   hashlib.sha256(source_lines[line - 1]).hexdigest(),
                   "c%s=%s" % (gap.group(1), gap.group(2)))
            found[key] += 1
            where.setdefault(key, line)
    return found, where


def load_manifest(path):
    records = {}
    for number, raw in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        if not raw or raw.startswith("#"):
            continue
        fields = raw.split("\t")
        if len(fields) != 6 or not fields[5].strip() or not fields[4].isdigit() or int(fields[4]) < 1:
            raise ValueError("%s:%d: expected module, function, sha256, condition, count and a reason"
                             % (path, number))
        key = tuple(fields[:4])
        if key in records:
            raise ValueError("%s:%d: duplicate record" % (path, number))
        records[key] = int(fields[4])
    return records


def measured(cov_dir, repo):
    """Every gap in every module the coverage run left a listing for."""
    found = Counter()
    where = {}
    listed = 0
    for source in sorted((repo / "src").glob("*/*.c")):
        module = source.stem
        listing = cov_dir / module / (module + ".c.gcov")
        if not listing.exists():
            continue
        listed += 1
        gaps, lines = gaps_in(listing.read_text(encoding="utf-8", errors="replace"),
                              module, source.read_bytes().splitlines())
        found.update(gaps)
        where.update(lines)
    return found, where, listed


def compare(found, where, records):
    errors = []
    for key in sorted(set(found) | set(records)):
        module, function, digest, condition = key
        got = found.get(key, 0)
        want = records.get(key, 0)
        if got > want:
            errors.append("unexplained MC/DC gap in %s (%s.c:%d) %s, %d recorded, %d found\n"
                          "    record: %s\t%s\t%s\t%s\t%d\t<reason>"
                          % (function, module, where[key], condition, want, got,
                             module, function, digest, condition, got))
        elif got < want:
            errors.append("stale MC/DC exception %s %s %s: %d recorded, %d found"
                          % (module, function, condition, want, got))
        else:
            pass
    return errors


def self_test():
    if not __debug__:
        raise SystemExit("mcdccheck controls cannot run under python -O")
    source = [b"/** banner */", b"static uint8_t helper ( uint8_t a, uint8_t b )", b"{",
              b"    if ( ( a == 0u ) || ( b == 0u ) )", b"    if ( ( a == 0u ) || ( b == 0u ) )", b"}"]
    listing = ("        -:    1:/** banner */\n"
               "        1:    2:static uint8_t helper ( uint8_t a, uint8_t b )\n"
               "        1:    3:{\n"
               "        1:    4:    if ( ( a == 0u ) || ( b == 0u ) )\n"
               "condition outcomes covered 3/4\n"
               "condition  1 not covered (true)\n"
               "        1:    5:    if ( ( a == 0u ) || ( b == 0u ) )\n"
               "condition outcomes covered 3/4\n"
               "condition  1 not covered (true)\n"
               "        1:    6:}\n")
    found, where = gaps_in(listing, "sdemo", source)
    digest = hashlib.sha256(source[3]).hexdigest()
    key = ("sdemo", "helper", digest, "c1=true")
    assert dict(found) == {key: 2}, found
    assert where[key] == 4
    assert compare(found, where, {key: 2}) == []
    assert "unexplained" in compare(found, where, {})[0]
    assert "unexplained" in compare(found, where, {key: 1})[0]
    assert "stale" in compare(found, where, {key: 3})[0]
    assert "stale" in compare(Counter(), {}, {key: 2})[0]
    other = ("sdemo", "helper", digest, "c0=true")
    assert len(compare(found, where, {other: 2})) == 2
    with tempfile.TemporaryDirectory() as scratch:
        path = Path(scratch) / "m.tsv"
        path.write_text("sdemo\thelper\t%s\tc1=true\t1\t\n" % digest, encoding="utf-8")
        try:
            load_manifest(path)
            raise AssertionError("a record without a reason was accepted")
        except ValueError:
            pass
        path.write_text("sdemo\thelper\t%s\tc1=true\t0\tr\n" % digest, encoding="utf-8")
        try:
            load_manifest(path)
            raise AssertionError("a record with a count of zero was accepted")
        except ValueError:
            pass
    print("mcdccheck controls: pass")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--dir", type=Path, required=False)
    parser.add_argument("--manifest", type=Path, default=REPO / "tools" / "mcdc_exceptions.tsv")
    parser.add_argument("--annotate", action="store_true")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        self_test()
        return 0
    if args.dir is None:
        parser.error("--dir is required: the directory tools/coverage.sh wrote")
    found, where, listed = measured(args.dir, REPO)
    if listed == 0:
        print("mcdccheck: no gcov listings under %s; run tools/coverage.sh first" % args.dir)
        return 2
    records = load_manifest(args.manifest)
    errors = compare(found, where, records)
    for error in errors:
        print(error)
        if args.annotate:
            print("::error::" + error.splitlines()[0])
    print("%d modules, %d MC/DC gaps, %d explained, %d problems"
          % (listed, sum(found.values()), sum(records.values()), len(errors)))
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
