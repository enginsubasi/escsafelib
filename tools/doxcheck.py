#!/usr/bin/env python3
"""Check the Doxygen comments across escsafelib against this repository's own
convention.

It exists because doxygen is not installed on the development machine, and
because the two things most worth checking are ones doxygen would not check
even if it were:

  - that a @param list matches the signature, which doxygen only reports
    with WARN_NO_PARAMDOC and only in one direction;
  - that a @return names every status the body can actually produce, and no
    status it cannot. Nothing but a reader ever checks that, and a reader
    checks it once.

The second one is what found the real defect it was written for. The four
smath families share one template, so the unsigned ones had inherited the
signed text and promised an SH_UNDERFLOW that an unsigned addition cannot
produce and an SH_OVERFLOW that an unsigned subtraction cannot.

Run from anywhere:

    python tools/doxcheck.py            # this repository
    python tools/doxcheck.py <dir>      # somewhere else, for testing

Exits non zero when anything is found, so it can gate a commit.

What it enforces, from CLAUDE.md and codingReference.md:

  banner      every .c opens with @file @author @version @date @brief,
              @par Device and @par History; @file names the real file; and
              the banner date is not older than the newest history line
  block       every function, static ones included, has a /** block
              immediately before it -- a /* block is invisible to doxygen
  brief       every block has exactly one @brief
  param       every parameter is documented once, in order, with a
              direction and a description, and no @param names a parameter
              that is not there
  direction   a by-value or const-pointer parameter is [in]. A non-const
              pointer is not checked, because whether it is written through
              is not visible in the signature
  return      @return is present exactly when the return type is not void
  status      every status the body can set is named somewhere in the block,
              and every status @return names can really be set
  tag         no tag outside the agreed set, which catches misspellings
  header      headers carry no doxygen at all

Two things it deliberately accepts:

  - "otherwise the status sarrayCopyNu8 reports" documents the delegate's
    statuses. That is better than a copied list, which would drift.
  - a status that the body can form but can never in fact produce may be
    explained in a @note rather than listed in @return. sfixedFloor calls a
    range check that can say SX_OVERFLOW, and rounding down cannot reach it.

A checker that passes on its first run has not been shown to check
anything. This one was developed against a control file carrying one
deliberate defect per rule, and every rule fired.
"""

import io
import os
import re
import sys

REPO = sys.argv[1] if len(sys.argv) > 1 else os.path.dirname(
    os.path.dirname(os.path.abspath(__file__)))

BANNER_TAGS = ["@file", "@author", "@version", "@date", "@brief"]
KNOWN_TAGS = {
    "@file", "@author", "@version", "@date", "@brief", "@par", "@note",
    "@param", "@return", "@n", "@warning", "@details", "@see",
}

findings = []
CHECKED = {}
# path -> {function name: the status codes its body assigns directly}
ASSIGNED_BY_NAME = {}


def report(path, line, kind, message):
    findings.append((os.path.relpath(path, REPO).replace("\\", "/"),
                     line, kind, message))


def function_bodies(text):
    """Yield (name, body) for every definition in a file.

    Every definition in this codebase starts at column zero with its opening
    brace alone on the next line, and ends at a closing brace in column zero.
    """
    for m in re.finditer(
            r"^(?:static\s+)?[A-Za-z_]\w*\s*\**\s*(\w+)\s*\([^;{]*\)\s*\n\{",
            text, re.M):
        body_start = text.index("{", m.start())
        end = re.search(r"^\}", text[body_start:], re.M)
        yield m.group(1), (text[body_start: body_start + end.end()] if end else "")


def statuses_in(body):
    """The status codes a body sets on its own return value."""
    codes = set(re.findall(r"retVal\s*=\s*([A-Z]{2}_[A-Z0-9]+)\s*;", body))
    codes |= set(re.findall(r"return\s*\(\s*([A-Z]{2}_[A-Z0-9]+)\s*\)", body))
    return codes


def collect_assignments(path):
    """First pass, so a wrapper can be checked against its helper."""
    text = io.open(path, encoding="utf-8").read()
    ASSIGNED_BY_NAME[path] = {n: statuses_in(b) for n, b in function_bodies(text)}


def split_params(text):
    """Split a parameter list on the commas that are not inside brackets."""
    out = []
    depth = 0
    current = ""

    for ch in text:
        if ch in "([":
            depth += 1
        elif ch in ")]":
            depth -= 1
        if (ch == ",") and (depth == 0):
            out.append(current)
            current = ""
        else:
            current += ch

    if current.strip():
        out.append(current)
    return [p.strip() for p in out if p.strip()]


def param_name(decl):
    """Last identifier of a parameter declaration, or None for void."""
    d = decl.replace("[", " [").strip()
    if d == "void":
        return None
    # A function pointer parameter names itself inside the first bracket.
    m = re.match(r"^[\w\s]*\(\s*\*\s*(\w+)\s*\)", d)
    if m:
        return m.group(1)
    ids = re.findall(r"[A-Za-z_]\w*", d)
    return ids[-1] if ids else None


def is_read_only(decl):
    """True when the callee cannot write through the parameter."""
    if "*" not in decl:
        return True
    return bool(re.match(r"^\s*const\b", decl))


def check_banner(path, text, head):
    name = os.path.basename(path)

    if not text.startswith("/**"):
        report(path, 1, "banner", "file does not open with a /** block")

    for tag in BANNER_TAGS + ["@par Device", "@par History"]:
        if tag not in head:
            report(path, 1, "banner", "banner has no %s" % tag)

    m = re.search(r"@file\s+(\S+)", head)
    if m and (m.group(1) != name):
        report(path, 1, "banner", "@file says %s, file is %s" % (m.group(1), name))

    # The banner date should not be older than the newest history line, or
    # the two drift apart and neither can be trusted.
    dm = re.search(r"@date\s+(\d{2}/\d{2}/\d{4})", head)
    hist = re.findall(r"^\s*\*\s*(\d{2}/\d{2}/\d{4})\s", head, re.M)

    if dm and hist:
        def key(d):
            return (d[6:10], d[3:5], d[0:2])

        newest = max(hist, key=key)
        if key(newest) > key(dm.group(1)):
            report(path, 1, "banner",
                   "@date is %s but the history runs to %s" % (dm.group(1), newest))


def check_statuses(path, lineno, fname, block, body, ret_text):
    assigned = statuses_in(body)

    # A status can also arrive from another function of the same file.
    # Only a result flowing straight into retVal is followed: unioning the
    # codes of every callee whose result was assigned anywhere reported the
    # saturating forms as able to return the very statuses they absorb,
    # because they ask the status helper and then answer SH_OK.
    forwards = False
    for callee in re.findall(r"retVal\s*=\s*([a-z]\w*)\s*\(", body):
        if callee in ASSIGNED_BY_NAME.get(path, {}):
            assigned |= ASSIGNED_BY_NAME[path][callee]
        else:
            forwards = True

    # A status parked in a local and returned later is not modelled.
    if re.search(r"retVal\s*=\s*[a-z]\w*\s*;", body):
        forwards = True

    documented = set(re.findall(r"\b([A-Z]{2}_[A-Z0-9]+)\b", ret_text))

    # The leading comment stars have to come off before the words are
    # joined, or a delegation phrase broken across two lines reads as
    # "status * name reports" and the delegate is missed.
    flat = " ".join(re.sub(r"^\s*\*", "", ln).strip() for ln in ret_text.split("\n"))
    for delegate in re.findall(r"status\s+(\w+)\s+reports", flat):
        documented |= ASSIGNED_BY_NAME.get(path, {}).get(delegate, set())

    mentioned = documented | set(re.findall(r"\b([A-Z]{2}_[A-Z0-9]+)\b", block))

    if assigned or documented:
        for code in sorted(assigned - mentioned):
            report(path, lineno, "status",
                   "%s: can return %s, and the block never mentions it"
                   % (fname, code))
        if not forwards:
            for code in sorted(documented - assigned):
                report(path, lineno, "status",
                       "%s: @return names %s, which the body never sets"
                       % (fname, code))


def check_source(path):
    text = io.open(path, encoding="utf-8").read()
    check_banner(path, text, "\n".join(text.split("\n")[:60]))

    pattern = re.compile(
        r"^((?:static\s+)?[A-Za-z_]\w*(?:\s+\w+)*\s*\**)\s+(\w+)\s*\(", re.M)

    for m in pattern.finditer(text):
        start = m.start()
        lineno = text.count("\n", 0, start) + 1
        rettype = m.group(1).strip()
        fname = m.group(2)

        if fname in ("if", "for", "while", "switch", "return", "sizeof"):
            continue

        # Walk to the paren that closes the parameter list.
        i = m.end() - 1
        depth = 0
        while i < len(text):
            if text[i] == "(":
                depth += 1
            elif text[i] == ")":
                depth -= 1
                if depth == 0:
                    break
            i += 1

        params_text = text[m.end(): i]

        # A definition, not a prototype and not a call.
        if not re.match(r"\s*\n\s*\{", text[i + 1: i + 40]):
            continue

        CHECKED[path] = CHECKED.get(path, 0) + 1

        decls = [d for d in split_params(params_text) if param_name(d) is not None]
        params = [param_name(d) for d in decls]

        # ---- the doc block immediately before ------------------------
        before = text[:start].rstrip()
        if not before.endswith("*/"):
            report(path, lineno, "block", "%s has no comment block before it" % fname)
            continue

        open_dd = before.rfind("/**")
        open_d = before.rfind("/*")

        if open_d > open_dd:
            report(path, lineno, "block",
                   "%s is documented with /* and is invisible to doxygen" % fname)
            continue
        if open_dd < 0:
            report(path, lineno, "block", "%s has no /** block" % fname)
            continue

        block = before[open_dd:]

        # ---- tags ----------------------------------------------------
        for tag in sorted(set(re.findall(r"@\w+", block))):
            if tag not in KNOWN_TAGS:
                report(path, lineno, "tag", "%s: unknown tag %s" % (fname, tag))

        briefs = len(re.findall(r"@brief\b", block))
        if briefs != 1:
            report(path, lineno, "brief", "%s has %d @brief" % (fname, briefs))

        # ---- params --------------------------------------------------
        documented = re.findall(r"@param\s*(\[[^\]]*\])?\s*(\w+)", block)
        doc_names = [d[1] for d in documented]

        for direction, pname in documented:
            if not direction:
                report(path, lineno, "param",
                       "%s: @param %s has no direction" % (fname, pname))

        for pname in params:
            if pname not in doc_names:
                report(path, lineno, "param",
                       "%s: parameter %s is not documented" % (fname, pname))

        for pname in doc_names:
            if pname not in params:
                report(path, lineno, "param",
                       "%s: @param %s is not a parameter" % (fname, pname))

        for pname in sorted(set(doc_names)):
            if doc_names.count(pname) > 1:
                report(path, lineno, "param",
                       "%s: @param %s appears %d times"
                       % (fname, pname, doc_names.count(pname)))

        if [n for n in doc_names if n in params] != [p for p in params if p in doc_names]:
            report(path, lineno, "param",
                   "%s: @param order does not match the signature" % fname)

        for pm in re.finditer(r"@param\s*(?:\[[^\]]*\])?\s*(\w+)([^\n]*)", block):
            if pm.group(2).strip() == "":
                report(path, lineno, "param",
                       "%s: @param %s has no description" % (fname, pm.group(1)))

        for decl in decls:
            pname = param_name(decl)
            for direction, dname in documented:
                if dname != pname:
                    continue
                d = (direction or "").strip("[]").replace(" ", "")
                if is_read_only(decl) and (d != "in"):
                    report(path, lineno, "direction",
                           "%s: %s is read only but marked [%s]" % (fname, pname, d))

        # ---- return --------------------------------------------------
        has_return = bool(re.search(r"@return\b", block))
        is_void = bool(re.match(r"^(static\s+)?void$", rettype))

        if is_void and has_return:
            report(path, lineno, "return", "%s returns void but has @return" % fname)
        if (not is_void) and (not has_return):
            report(path, lineno, "return", "%s has no @return" % fname)

        # ---- statuses ------------------------------------------------
        body_start = text.index("{", i)
        end = re.search(r"^\}", text[body_start:], re.M)
        body = text[body_start: body_start + end.end()] if end else ""

        rm = re.search(r"@return(.*?)(?=\n\s*\*\s*@|\n\s*\*/)", block, re.S)
        check_statuses(path, lineno, fname, block, body, rm.group(1) if rm else "")


def check_header(path):
    text = io.open(path, encoding="utf-8").read()

    if "/**" in text:
        report(path, text[: text.find("/**")].count("\n") + 1,
               "header", "header carries a doxygen block")

    for tag in ("@brief", "@param", "@return", "@file"):
        if tag in text:
            report(path, 1, "header", "header carries %s" % tag)


def main():
    targets = []

    for root, dirs, files in os.walk(REPO):
        dirs[:] = [d for d in dirs if d not in (".git", ".github")]
        for f in files:
            if f.endswith(".c"):
                targets.append(os.path.join(root, f))
            elif f.endswith(".h"):
                check_header(os.path.join(root, f))

    for t in sorted(targets):
        collect_assignments(t)

    for t in sorted(targets):
        check_source(t)

    by_file = {}
    for f, line, kind, msg in findings:
        by_file.setdefault(f, []).append((line, kind, msg))

    for f in sorted(by_file):
        print("\n%s" % f)
        for line, kind, msg in sorted(by_file[f]):
            print("  %5d  %-10s %s" % (line, kind, msg))

    print("\n%d functions in %d .c files, %d findings"
          % (sum(CHECKED.values()), len(targets), len(findings)))

    return 1 if findings else 0


if __name__ == "__main__":
    sys.exit(main())
