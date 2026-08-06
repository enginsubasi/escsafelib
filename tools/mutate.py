#!/usr/bin/env python3
"""Run the mutation tests.

A suite that passes has been shown to run the code. It has not been shown
to check anything: every assertion could be missing and the run would look
the same. Mutation testing is how that gets established. Break the module
on purpose, one defect at a time, and require the suite to go red.

This file is the runner. The defects live in tools/mutants/<module>.py, one
file per module, so that each set can be read and reviewed on its own.

    python tools/mutate.py                  # every module
    python tools/mutate.py svote sbits      # named modules
    python tools/mutate.py --list           # what is defined

Exits non zero when anything is wrong, so it can gate a commit.

Three outcomes, and the third is the reason this is a tool rather than a
number in a document:

  killed      the suite went red. What was wanted.
  SURVIVED    the suite passed against a broken module. A hole in the
              suite, unless the mutant is marked equivalent.
  RESURRECTED a mutant marked equivalent was killed. The equivalence
              argument written next to it is wrong, or the suite grew a
              case that distinguishes what was claimed indistinguishable.
              Either way the note has to change.

An equivalent mutant is one that cannot change any observable behaviour,
so no test could ever kill it. They are not failures and they are not
excuses: each carries the argument for why it cannot be killed, and the
runner checks that argument still holds by requiring it to survive.

Some defects are undefined behaviour that happens to compute the right
answer on this host. svote's channel spread formed in 32 bits is one: every
assertion passes and it is still signed overflow. Those mutants are marked
`ubsan` and are re-run under -fsanitize=undefined -fsanitize-trap=undefined,
which is how the real suite runs in tools/run_all.sh and in CI. A trap is a
kill.
"""

import io
import os
import shutil
import subprocess
import sys
import tempfile

# A mutant is meant to break the module, so a good many of them crash. On
# Windows a crash normally raises a dialog and waits for somebody to click
# it, which stops the run dead: the first version of this script sat for
# four minutes on a mutant that removed the guard against INT32_MIN divided
# by minus one, because the resulting SIGFPE was waiting for a mouse. Ask
# the operating system not to do that; child processes inherit the setting.
if os.name == "nt":
    try:
        import ctypes

        SEM_FAILCRITICALERRORS = 0x0001
        SEM_NOGPFAULTERRORBOX = 0x0002
        ctypes.windll.kernel32.SetErrorMode(
            SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX)
    except Exception:
        pass

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
MUTANTDIR = os.path.join(REPO, "tools", "mutants")

CC = os.environ.get("CC", "gcc")
UBCC = os.environ.get("UBSANCC", CC)


def load(name):
    """Read one module's mutant set without any package machinery."""
    path = os.path.join(MUTANTDIR, name + ".py")
    if not os.path.exists(path):
        return None

    namespace = {}
    exec(compile(io.open(path, encoding="utf-8").read(), path, "exec"), namespace)
    return namespace


def modules():
    names = []
    for f in sorted(os.listdir(MUTANTDIR)):
        if f.endswith(".py") and not f.startswith("_"):
            names.append(f[:-3])
    return names


def build_and_run(compiler, source, domain, test, work, extra):
    """Build the test suite against a source and run it. True when it fails."""
    exe = os.path.join(work, "run.exe")
    command = compiler.split() + [
        "-std=c99", "-g", "-I" + os.path.join(REPO, "inc", domain),
    ] + extra + [
        os.path.join(REPO, "test", test + "_Test", test + "_Test.c"),
        source, "-o", exe,
    ]

    build = subprocess.run(command, capture_output=True, text=True)
    if build.returncode != 0:
        return ( None, build.stderr.strip()[:300] )

    try:
        run = subprocess.run([exe], capture_output=True, text=True, timeout=300)
    except subprocess.TimeoutExpired:
        # A suite that never finishes has not passed. A mutant that sends it
        # into a loop that does not end is caught as surely as one that
        # trips an assertion, and the run has to carry on rather than stop
        # on the first one.
        return ( True, "the suite did not finish" )

    tail = run.stdout.strip().split("\n")[-1] if run.stdout.strip() else "(no output)"
    return ( run.returncode != 0, tail )


def check(name):
    space = load(name)
    if space is None:
        print("no mutant set for %s" % name)
        return 1

    domain, module, test = space["MODULE"]
    mutants = space["MUTANTS"]
    original = io.open(os.path.join(REPO, "src", domain, module + ".c"),
                       encoding="utf-8").read()

    work = tempfile.mkdtemp(prefix="escsafelib-mutate-")
    killed = 0
    equivalent = 0
    problems = []

    print("\n== %s: %d mutants ==" % (module, len(mutants)))

    for m in mutants:
        tag = "%-4s %s" % (m["id"], m["what"])
        found = original.count(m["old"])

        if found != 1:
            print("  %-62s ANCHOR MATCHES %d TIMES" % (tag, found))
            problems.append("%s %s: anchor matches %d times" % (module, m["id"], found))
            continue

        source = os.path.join(work, module + ".c")
        io.open(source, "w", encoding="utf-8", newline="\n").write(
            original.replace(m["old"], m["new"], 1))

        died, note = build_and_run(CC, source, domain, test, work, [])

        if died is None:
            print("  %-62s DID NOT BUILD" % tag)
            problems.append("%s %s: did not build: %s" % (module, m["id"], note))
            continue

        how = "suite"

        # A defect that is undefined behaviour may compute the right answer
        # here and still be a defect. The sanitizer is part of how the real
        # suite runs, so a trap counts as a kill.
        if (not died) and m.get("ubsan", False):
            died, note = build_and_run(
                UBCC, source, domain, test, work,
                ["-fsanitize=undefined", "-fsanitize-trap=undefined"])
            how = "ubsan"
            if died is None:
                died = False

        expected = m.get("equivalent")

        if died and (expected is None):
            killed += 1
            print("  %-62s killed (%s)" % (tag, how))
        elif died and (expected is not None):
            print("  %-62s RESURRECTED" % tag)
            problems.append("%s %s: marked equivalent but the suite killed it. "
                            "The argument is: %s" % (module, m["id"], expected))
        elif (not died) and (expected is not None):
            equivalent += 1
            print("  %-62s survives, equivalent" % tag)
        else:
            print("  %-62s SURVIVED  %s" % (tag, note))
            problems.append("%s %s: survived. %s" % (module, m["id"], m["what"]))

    shutil.rmtree(work, ignore_errors=True)

    print("  %d killed, %d equivalent, %d of %d accounted for"
          % (killed, equivalent, killed + equivalent, len(mutants)))

    for p in problems:
        print("  !! %s" % p)

    return len(problems)


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]

    if "--list" in sys.argv:
        for n in modules():
            space = load(n)
            print("  %-10s %d mutants" % (n, len(space["MUTANTS"])))
        return 0

    names = args if args else modules()
    problems = 0

    for n in names:
        problems += check(n)

    print()
    if problems == 0:
        print("every mutant is accounted for")
    else:
        print("%d mutants are not accounted for, see above" % problems)

    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
