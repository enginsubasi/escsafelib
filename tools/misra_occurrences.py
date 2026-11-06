#!/usr/bin/env python3
"""Check cppcheck MISRA Rule 10.4 findings against reviewed source lines."""

import argparse
import hashlib
import re
import sys
from collections import Counter
from pathlib import Path


def source_fingerprint(path: Path, line: int) -> str:
    lines = path.read_bytes().splitlines()
    if line < 1 or line > len(lines):
        raise ValueError(f"{path}:{line}: source line does not exist")
    return hashlib.sha256(lines[line - 1]).hexdigest()


def load_manifest(path: Path) -> Counter:
    records = Counter()
    for number, raw in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        if not raw or raw.startswith("#"):
            continue
        fields = raw.split("\t")
        if len(fields) != 4:
            raise ValueError(f"{path}:{number}: expected module, line, sha256, count")
        module, line, digest, count = fields
        if not re.fullmatch(r"[a-z][a-z0-9_]*", module):
            raise ValueError(f"{path}:{number}: invalid module name")
        if len(digest) != 64 or any(c not in "0123456789abcdef" for c in digest):
            raise ValueError(f"{path}:{number}: invalid SHA-256 fingerprint")
        key = (module, int(line), digest)
        amount = int(count)
        if int(line) < 1 or amount < 1 or key in records:
            raise ValueError(f"{path}:{number}: duplicate key or invalid count")
        records[key] = amount
    return records


def actual_records(path: Path, repo: Path) -> Counter:
    records = Counter()
    for number, raw in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        if not raw:
            continue
        fields = raw.split("\t")
        if len(fields) != 2:
            raise ValueError(f"{path}:{number}: expected module and line")
        module, line_text = fields
        line = int(line_text)
        source = next((repo / "src").glob(f"*/{module}.c"), None)
        if source is None:
            raise ValueError(f"unknown MISRA module: {module}")
        records[(module, line, source_fingerprint(source, line))] += 1
    return records


def compare(expected: Counter, actual: Counter) -> list[str]:
    errors = []
    for key in sorted(expected.keys() | actual.keys()):
        want = expected[key]
        got = actual[key]
        if want != got:
            module, line, _ = key
            state = "new/excess" if got > want else "stale/missing"
            errors.append(f"{state} Rule 10.4 occurrence: {module}:{line} expected {want}, found {got}")
    return errors


def self_test() -> None:
    key = ("sstate", 249, "a" * 64)
    expected = Counter({key: 2})
    assert compare(expected, Counter({key: 2})) == []
    assert compare(expected, Counter({key: 3}))
    assert compare(expected, Counter({key: 1}))
    assert compare(expected, Counter({("sstate", 249, "b" * 64): 2}))
    assert compare(expected, Counter({key: 1, ("sstate", 250, "c" * 64): 1}))


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
    if not args.actual:
        parser.error("--actual is required")
    actual = actual_records(args.actual, args.repo)
    expected = None
    if args.manifest:
        expected = load_manifest(args.manifest)
        if args.modules:
            selected = set(args.modules)
            expected = Counter({key: count for key, count in expected.items() if key[0] in selected})
            actual = Counter({key: count for key, count in actual.items() if key[0] in selected})
    if expected is None:
        parser.error("--manifest and --actual are required")
    errors = compare(expected, actual)
    for error in errors:
        print(error, file=sys.stderr)
    return 1 if errors else 0


if __name__ == "__main__":
    raise SystemExit(main())
