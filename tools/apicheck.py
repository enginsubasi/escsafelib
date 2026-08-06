#!/usr/bin/env python3
"""Check the public API of every module against the rules this library
states about itself.

`tools/doxcheck.py` checks what the comments say. This checks what the
declarations do. Between them the two cover the conventions in CLAUDE.md
that nothing else enforces, and the first of these is the one the whole
library is arranged around:

  capacity    Every pointer to a buffer is immediately followed by the
              capacity of that buffer, and a bound is never inferred from a
              different buffer. This is not a style rule. Deriving a source
              scan bound from the destination size once let a short
              unterminated source be read past its end, a guard page test
              reproduced it as a fault, and giving every pointer its own
              capacity is the fix that is still holding.
  adjacency   A parameter named <x>Size, <x>Len or <x>Capacity sits
              immediately after the parameter named <x>. A size separated
              from its pointer is how the two come to disagree.
  status      Every public function returns uint8_t, the status. A function
              that returned its answer would have nowhere to put a failure.
  prefix      Every public function is named after its module.
  enum        Every enum member uses its module's registered status prefix,
              so that two modules in one translation unit cannot collide.
  types       stdint.h types throughout. A bare int is a different width on
              a different part, which is the whole reason the rule exists.

Run from anywhere:

    python tools/apicheck.py            # this repository
    python tools/apicheck.py <dir>      # somewhere else, for testing

Exits non zero when anything is found, so it can gate a commit.

A pointer to a module's own driver struct is a handle rather than a buffer
and carries no capacity: there is nothing to overrun. Those types are found
by reading each header's own typedefs rather than by keeping a list here,
so a new module needs no change to this file.

A checker that passes on its first run has not been shown to check
anything. This one was developed against a copy of a header carrying one
deliberate breach per rule, and every rule fired.
"""

import io
import os
import re
import sys

REPO = sys.argv[1] if len(sys.argv) > 1 else os.path.dirname(
    os.path.dirname(os.path.abspath(__file__)))

SIZE_SUFFIXES = ("Size", "Len", "Length", "Capacity", "Bytes", "Words")

# Types the library is allowed to name. Anything else in a prototype is
# either a module's own typedef, which is checked separately, or a breach.
STDINT = {
    "uint8_t", "uint16_t", "uint32_t", "uint64_t",
    "int8_t", "int16_t", "int32_t", "int64_t",
    "void", "char",
}

BANNED = ("int", "short", "long", "float", "double", "unsigned", "signed",
          "size_t")

findings = []
checked = 0


def report(path, kind, message):
    findings.append((os.path.relpath(path, REPO).replace("\\", "/"), kind, message))


def splitParams(text):
    """Split a parameter list on the commas outside any brackets."""
    out = []
    depth = 0
    current = ""

    for ch in text:
        if ch in "([":
            depth += 1
        elif ch in ")]":
            depth -= 1
        if (ch == ",") and (depth == 0):
            out.append(current.strip())
            current = ""
        else:
            current += ch

    if current.strip():
        out.append(current.strip())
    return out


def nameOf(decl):
    """The parameter's name, which is its last identifier."""
    m = re.match(r"^[\w\s]*\(\s*\*\s*(\w+)\s*\)", decl)
    if m:
        return m.group(1)

    ids = re.findall(r"[A-Za-z_]\w*", decl)
    return ids[-1] if ids else ""


def baseType(decl):
    """The declared type with const, the star and the name removed."""
    d = decl.replace("const", " ").replace("*", " ")
    ids = re.findall(r"[A-Za-z_]\w*", d)
    return ids[0] if len(ids) > 1 else ""


def checkHeader(path, module, prefix, handles):
    global checked

    text = io.open(path, encoding="utf-8").read()

    for m in re.finditer(r"^(\w[\w\s]*?)\s+(\w+)\s*\(\s*(.*?)\s*\)\s*;\s*$", text, re.M):
        rettype, name, params = m.group(1).strip(), m.group(2), m.group(3)

        # A function pointer typedef looks like a declaration and is not
        # one. sring's injected memory barrier is the only case, and
        # reporting it would be reporting the shape of the driver struct
        # pattern rather than a breach of anything.
        if rettype.startswith("typedef"):
            continue

        checked += 1

        # ---- status ------------------------------------------------
        if rettype != "uint8_t":
            report(path, "status",
                   "%s returns %s; every public function returns the status"
                   % (name, rettype))

        # ---- prefix ------------------------------------------------
        if not name.startswith(module):
            report(path, "prefix",
                   "%s is not named after its module %s" % (name, module))

        decls = [d for d in splitParams(params) if d != "void"]
        names = [nameOf(d) for d in decls]

        for i, d in enumerate(decls):
            base = baseType(d)
            who = nameOf(d)

            # ---- types ---------------------------------------------
            for word in re.findall(r"[A-Za-z_]\w*", d.replace("const", " ")):
                if word == who:
                    continue
                if (word in BANNED) and (word not in STDINT):
                    report(path, "types",
                           "%s takes a %s; the library uses stdint.h types"
                           % (name, word))

            # ---- capacity ------------------------------------------
            # A const pointer is an input the callee reads through, so it
            # needs its own bound. A pointer to a driver struct is a handle
            # and has nothing to overrun.
            if ("*" in d) and d.startswith("const") and (base not in handles):
                following = decls[i + 1] if (i + 1) < len(decls) else None

                if following is None:
                    report(path, "capacity",
                           "%s reads through %s and it is the last parameter, "
                           "so nothing bounds it" % (name, who))
                elif baseType(following) != "uint32_t":
                    report(path, "capacity",
                           "%s reads through %s and the next parameter is %s, "
                           "not a capacity" % (name, who, following))
                elif "*" in following:
                    report(path, "capacity",
                           "%s reads through %s and the next parameter is a "
                           "pointer, not a capacity" % (name, who))
                else:
                    # Intentionally blank.
                    pass

            # ---- adjacency -----------------------------------------
            for suffix in SIZE_SUFFIXES:
                if who.endswith(suffix) and (len(who) > len(suffix)):
                    owner = who[: -len(suffix)]

                    if owner in names:
                        if names.index(owner) != (i - 1):
                            report(path, "adjacency",
                                   "%s puts %s away from %s" % (name, who, owner))
                        else:
                            # Intentionally blank.
                            pass
                    else:
                        # Intentionally blank.
                        pass

    # ---- enum ------------------------------------------------------
    for member in re.findall(r"^\s{4}([A-Z][A-Z0-9_]*)\s*=", text, re.M):
        if not member.startswith(prefix):
            report(path, "enum",
                   "%s does not use this module's %s prefix" % (member, prefix))


def modulePrefix(text):
    """The status prefix a header's own enum uses."""
    members = re.findall(r"^\s{4}([A-Z]{2})_[A-Z0-9_]*\s*=", text, re.M)
    return (members[0] + "_") if members else ""


def moduleHandles(text):
    """The struct types this header declares, which are handles."""
    return set(re.findall(r"^\}\s*(\w+_t)\s*;", text, re.M))


def main():
    global checked

    headers = []
    for root, dirs, files in os.walk(os.path.join(REPO, "inc")):
        for f in sorted(files):
            if f.endswith(".h"):
                headers.append(os.path.join(root, f))

    for h in sorted(headers):
        text = io.open(h, encoding="utf-8").read()
        module = os.path.basename(h)[:-2]
        checkHeader(h, module, modulePrefix(text), moduleHandles(text))

    byFile = {}
    for f, kind, msg in findings:
        byFile.setdefault(f, []).append((kind, msg))

    for f in sorted(byFile):
        print("\n%s" % f)
        for kind, msg in sorted(byFile[f]):
            print("  %-10s %s" % (kind, msg))

    print("\n%d declarations in %d headers, %d findings"
          % (checked, len(headers), len(findings)))

    return 1 if findings else 0


if __name__ == "__main__":
    sys.exit(main())
