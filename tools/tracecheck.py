#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Trace every public function to the cases in its own suite that check it.

Coverage says a line ran. Mutation testing says a sampled defect was seen.
Neither says which documented answer of which function a suite has ever
asked for, and that is the question a reviewer asks first: for this
function, where is the case that expects each status its @return promises?

For every prototype in inc/<domain>/<module>.h this checks, against
test/<Module>_Test/<Module>_Test.c and nothing else:

  called      the suite calls the function at all.
  status      for every status the function's @return block names, there
              is a statement in the suite that calls the function and names
              that status. That is the shape every suite here uses:

                  expectStatus ( "...", sxFoo ( ... ), SX_OVERFLOW );

A status named in the @return block of a delegate ("otherwise the status
sarrayCopyNu8 reports") counts as named by the wrapper too. A status the
body can form but can never produce belongs in a @note, which this does
not read, so it is not required here.

The test is textual on purpose, and it can only err one way. A statement
that calls the function and names the status is not proof the two are
compared with each other; a status parked in a local and checked on a
later line is reported as missing although it is checked. The first would
need a reviewer to notice; the second is fixed by writing the check in the
same statement, which is what every suite here already does.

A status no case can expect is recorded in tools/trace_exceptions.tsv with
the reason. Every line there must still be a gap: an exception that a case
now covers is reported as stale, so a recorded reason cannot outlive the
fact it was written about.

Only a module's own suite counts. The integration suite exercises chains,
and a status first seen there has not been asked for by the suite that a
copied-out module carries with it.

    python tools/tracecheck.py              # check, exit non zero on a gap
    python tools/tracecheck.py --matrix     # also print the full matrix
    python tools/tracecheck.py --self-test  # the controls

A checker that passes on its first run has not been shown to check
anything; --self-test builds one module with one gap of each kind and
requires both to be reported.
"""

import contextlib
import io
import os
import re
import sys
import tempfile

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

STATUS = re.compile(r"\bS[A-Z]_[A-Z0-9_]+\b")


def strip_comments(text):
    text = re.sub(r"/\*.*?\*/", lambda m: "\n" * m.group(0).count("\n"), text, flags=re.S)
    return re.sub(r"//[^\n]*", "", text)


def prototypes(header):
    text = strip_comments(io.open(header, encoding="utf-8").read())
    return re.findall(r"^\s*uint8_t\s+(\w+)\s*\(", text, re.M)


def return_blocks(source):
    """Function name -> the text of its @return paragraph."""
    text = io.open(source, encoding="utf-8").read()
    blocks = {}
    for m in re.finditer(r"/\*\*(.*?)\*/\s*\n(?:static\s+)?[A-Za-z_]\w*\s*\**\s*(\w+)\s*\(", text, re.S):
        comment, name = m.group(1), m.group(2)
        lines = [re.sub(r"^\s*\*\s?", "", ln) for ln in comment.split("\n")]
        taking = False
        kept = []
        for ln in lines:
            if ln.startswith("@"):
                taking = ln.startswith("@return")
            if taking:
                kept.append(ln)
        blocks[name] = " ".join(kept)
    return blocks


def promised(blocks, name, seen=None):
    """The statuses a function's @return names, following delegates."""
    seen = seen or set()
    if name in seen or name not in blocks:
        return set()
    seen.add(name)
    text = blocks[name]
    codes = set(STATUS.findall(text))
    for delegate in re.findall(r"status\s+(\w+)\s+reports", text):
        codes |= promised(blocks, delegate, seen)
    return codes


def statements(test_source):
    text = strip_comments(io.open(test_source, encoding="utf-8").read())
    return [s for s in text.split(";") if s.strip()]


PARK_WINDOW = 12


def parked(stmts, call, wanted):
    """Statuses checked through a local the call's result was parked in.

    The sweeps write status = sxFoo ( ... ) and then compare status with
    the answer an oracle expects. A status counts when one of the next few
    statements compares that same local with it, or hands the local and the
    status to an expect helper together, before the local is assigned again.
    """
    found = set()
    for i, s in enumerate(stmts):
        m = re.search(r"\b(\w+)\s*=\s*" + call.pattern, s)
        if not m:
            continue
        local = re.escape(m.group(1))
        for later in stmts[i + 1: i + 1 + PARK_WINDOW]:
            if re.search(r"\b" + local + r"\s*=[^=]", later):
                break
            for code in wanted:
                if re.search(r"\b" + local + r"\s*[!=]=\s*" + code + r"\b", later) \
                        or re.search(r"\b" + code + r"\s*[!=]=\s*" + local + r"\b", later) \
                        or re.search(r"\b" + local + r"\s*,\s*" + code + r"\b", later):
                    found.add(code)
    return found


def check_module(header, source, suite):
    """Yield (function, kind, detail) for every gap, and the matrix rows."""
    gaps = []
    rows = []
    blocks = return_blocks(source)
    stmts = statements(suite)
    for name in prototypes(header):
        call = re.compile(r"\b" + re.escape(name) + r"\s*\(")
        calling = [s for s in stmts if call.search(s)]
        wanted = sorted(promised(blocks, name))
        named = set()
        for code in wanted:
            if any(re.search(r"\b" + code + r"\b", s) for s in calling):
                named.add(code)
        named |= parked(stmts, call, wanted)
        asked = sorted(named)
        missing = [code for code in wanted if code not in asked]
        if not calling:
            gaps.append((name, "called", "never called by its own suite"))
        for code in missing:
            gaps.append((name, "status", "no case expects " + code))
        rows.append((name, len(calling), asked, missing))
    return gaps, rows


def exists_exactly(path):
    """os.path.exists, but with every component's case matched.

    Windows and macOS find test/SE2E_Test when asked for test/SE2e_Test;
    the Linux runner does not. A name that only resolves case-insensitively
    passes here and fails in CI, so it is refused here as well.
    """
    if not os.path.exists(path):
        return False
    head, tail = os.path.split(os.path.abspath(path))
    while tail:
        if tail not in os.listdir(head):
            return False
        head, tail = os.path.split(head)
    return True


def modules(repo):
    found = []
    for domain in sorted(os.listdir(os.path.join(repo, "inc"))):
        for f in sorted(os.listdir(os.path.join(repo, "inc", domain))):
            if not f.endswith(".h"):
                continue
            module = f[:-2]
            suite_name = "S" + module[1].upper() + module[2:]
            found.append((module,
                          os.path.join(repo, "inc", domain, f),
                          os.path.join(repo, "src", domain, module + ".c"),
                          os.path.join(repo, "test", suite_name + "_Test", suite_name + "_Test.c")))
    return found


def load_exceptions(path):
    found = {}
    if not os.path.exists(path):
        return found
    for number, raw in enumerate(io.open(path, encoding="utf-8").read().splitlines(), 1):
        if not raw.strip() or raw.startswith("#"):
            continue
        fields = raw.split("\t")
        if len(fields) != 4 or not fields[3].strip():
            raise ValueError("%s:%d: expected module, function, status and a reason" % (path, number))
        key = tuple(fields[:3])
        if key in found:
            raise ValueError("%s:%d: duplicate exception" % (path, number))
        found[key] = fields[3]
    return found


def run(repo, matrix):
    total_gaps = 0
    functions = 0
    pairs = 0
    exceptions = load_exceptions(os.path.join(repo, "tools", "trace_exceptions.tsv"))
    used = set()
    for module, header, source, suite in modules(repo):
        if not exists_exactly(suite):
            print("%s: no suite at %s" % (module, os.path.relpath(suite, repo)))
            total_gaps += 1
            continue
        gaps, rows = check_module(header, source, suite)
        functions += len(rows)
        pairs += sum(len(a) + len(m) for _, _, a, m in rows)
        for name, kind, detail in gaps:
            key = (module, name, detail.split()[-1]) if kind == "status" else None
            if key in exceptions:
                used.add(key)
                continue
            print("%s  %-8s %s: %s" % (module, kind, name, detail))
            total_gaps += 1
        if matrix:
            for name, calls, asked, missing in rows:
                print("  %-28s %4d calls  %s%s" % (
                    name, calls, " ".join(asked),
                    ("  MISSING " + " ".join(missing)) if missing else ""))
    for key in sorted(set(exceptions) - used):
        print("%s  stale    %s: the exception for %s is recorded but a case now expects it" % (key[0], key[1], key[2]))
        total_gaps += 1
    print("%d functions, %d function/status pairs, %d recorded exceptions, %d gaps"
          % (functions, pairs, len(used), total_gaps))
    return 1 if total_gaps else 0


def self_test():
    # The controls are assert statements, which python -O strips. Refuse
    # rather than report a pass that checked nothing.
    if not __debug__:
        raise SystemExit("tracecheck controls cannot run under python -O")
    root = tempfile.mkdtemp(prefix="tracecheck-")
    os.makedirs(os.path.join(root, "inc", "demo"))
    os.makedirs(os.path.join(root, "src", "demo"))
    os.makedirs(os.path.join(root, "test", "SDemo_Test"))
    io.open(os.path.join(root, "inc", "demo", "sdemo.h"), "w").write(
        "uint8_t sdemoA ( uint8_t* x );\nuint8_t sdemoB ( uint8_t* x );\n"
        "uint8_t sdemoC ( uint8_t* x );\nuint8_t sdemoD ( uint8_t* x );\n")
    io.open(os.path.join(root, "src", "demo", "sdemo.c"), "w").write(
        "/**\n * @brief A.\n * @return SZ_OK, SZ_NULLPTR when x is NULL.\n * @note SZ_ODD cannot arise.\n */\n"
        "uint8_t sdemoA ( uint8_t* x )\n{\n}\n"
        "/**\n * @brief B.\n * @return SZ_OK, SZ_NULLPTR when x is NULL.\n */\n"
        "uint8_t sdemoB ( uint8_t* x )\n{\n}\n"
        "/**\n * @brief C.\n * @return SZ_OK.\n */\n"
        "uint8_t sdemoC ( uint8_t* x )\n{\n}\n"
        "/**\n * @brief D.\n * @return SZ_EMPTY, otherwise the status sdemoA reports.\n */\n"
        "uint8_t sdemoD ( uint8_t* x )\n{\n}\n")
    io.open(os.path.join(root, "test", "SDemo_Test", "SDemo_Test.c"), "w").write(
        "expectStatus ( \"a\", sdemoA ( &v ), SZ_OK );\n"
        "expectStatus ( \"a\", sdemoA ( NULL ), SZ_NULLPTR );\n"
        "expectStatus ( \"b\", sdemoB ( &v ), SZ_OK );\n"
        "/* expectStatus ( \"b\", sdemoB ( NULL ), SZ_NULLPTR ); */\n"
        "expectStatus ( \"d\", sdemoD ( &v ), SZ_OK );\n"
        "expectStatus ( \"d\", sdemoD ( &v ), SZ_EMPTY );\n")
    module = modules(root)[0]
    gaps, _ = check_module(*module[1:])
    got = sorted((n, k, d) for n, k, d in gaps)
    want = sorted([
        ("sdemoB", "status", "no case expects SZ_NULLPTR"),
        ("sdemoC", "called", "never called by its own suite"),
        ("sdemoC", "status", "no case expects SZ_OK"),
        ("sdemoD", "status", "no case expects SZ_NULLPTR"),
    ])
    assert got == want, got

    # One exception that covers a real gap, and one stale: sdemoA's SZ_OK is
    # expected by a case, so its exception must be reported.
    os.makedirs(os.path.join(root, "tools"))
    io.open(os.path.join(root, "tools", "trace_exceptions.tsv"), "w").write(
        "sdemo\tsdemoB\tSZ_NULLPTR\tcontrol\nsdemo\tsdemoA\tSZ_OK\tcontrol\n")
    out = io.StringIO()
    with contextlib.redirect_stdout(out):
        status = run(root, False)
    text = out.getvalue()
    assert status == 1
    assert "sdemoB: no case expects SZ_NULLPTR" not in text
    assert "stale    sdemoA" in text
    assert "1 recorded exceptions, 4 gaps" in text, text

    # A suite directory whose name matches only when case is ignored must
    # not count as found, on any file system.
    suite = os.path.join(root, "test", "SDemo_Test", "SDemo_Test.c")
    assert exists_exactly(suite)
    assert not exists_exactly(os.path.join(root, "test", "SDEMO_Test", "SDemo_Test.c"))
    assert not exists_exactly(os.path.join(root, "test", "SDemo_Test", "SDEMO_Test.c"))
    print("tracecheck controls: pass")
    return 0


def main():
    if "--self-test" in sys.argv:
        return self_test()
    return run(REPO, "--matrix" in sys.argv)


if __name__ == "__main__":
    sys.exit(main())
