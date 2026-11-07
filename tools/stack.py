#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Stack use of every module on the target, measured by the cross compiler.

There is no ARM part to run on, and no emulator, but stack use does not
need one: arm-none-eabi-gcc reports the frame of every function it emits
(-fstack-usage) and the calls between them (-fcallgraph-info). From those
this tool works out, for each module and each core:

  - the deepest call chain inside the module, frames added up;
  - the largest single frame;
  - every call it cannot follow: a compiler runtime helper such as
    __aeabi_lmul, whose stack is the toolchain's, and an indirect call
    through a function pointer, whose stack is the injected function's.

It fails, exit 1, on a frame that is not static (a variable length array
or alloca, whose size the compiler cannot know) and on any cycle in the
call graph (recursion, whose depth no static figure bounds). Both are
outside what this library promises.

The figures are for -Os, the flags size.sh uses, and they include
inlining: a helper the compiler folded into its caller has no frame of its
own. A project building with other flags gets other numbers, and the
worst case it must allow for is the chain here plus the deepest of the
runtime helpers and injected functions listed beside it.

  python tools/stack.py                 # both cores, every module
  python tools/stack.py --cc <compiler>
  python tools/stack.py --annotate      # also one GitHub notice per core

Before anything else it compiles a control file holding a recursive
function and a variable length array, and exits 2 unless both are reported:
a stack check that passes on its first run has not been shown to check
anything.
"""

import argparse
import os
import re
import subprocess
import sys
import tempfile

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CORES = ("cortex-m0plus", "cortex-m4")
NODE = re.compile(r'^node: \{ title: "([^"]*)" label: "([^"]*)"')
EDGE = re.compile(r'^edge: \{ sourcename: "([^"]*)" targetname: "([^"]*)"')
FRAME = re.compile(r"(\d+) bytes \((\w+)\)")

CONTROL = """
int depth ( int n )
{
    int r = n;
    if ( n > 1 )
    {
        r = depth ( n - 1 ) * depth ( n - 2 ) + n;
    }
    return r;
}

int sized ( int n )
{
    volatile char buffer[ n ];
    buffer[ 0 ] = 1;
    return buffer[ 0 ];
}
"""


def compile_graph(cc, core, source, include, work):
    """Frames and edges of one translation unit, by node title."""
    base = os.path.join(work, os.path.splitext(os.path.basename(source))[0] + "_" + core)
    cmd = [cc, "-c", "-std=c99", "-Os", "-mthumb", "-mcpu=" + core,
           "-fstack-usage", "-fcallgraph-info=su", "-o", base + ".o", source]
    if include:
        cmd[1:1] = ["-I" + include]
    subprocess.run(cmd, check=True, capture_output=True)
    nodes = {}
    edges = {}
    with open(base + ".ci", encoding="utf-8", errors="replace") as graph:
        for line in graph:
            node = NODE.match(line)
            edge = EDGE.match(line)
            if node:
                title, label = node.group(1), node.group(2)
                frame = FRAME.search(label)
                if frame:
                    nodes[title] = (int(frame.group(1)), frame.group(2))
                else:
                    nodes[title] = (None, "external")
            elif edge:
                edges.setdefault(edge.group(1), set()).add(edge.group(2))
    return nodes, edges


def short(title):
    """The function name of a node title, which carries a path for a static."""
    return title.rsplit(":", 1)[-1]


def analyse(nodes, edges):
    """Worst chain, largest frame, externals and every problem found."""
    problems = []
    depth = {}
    worst_chain = {}
    externals = set()
    indirect = 0

    for title, (size, kind) in nodes.items():
        if kind not in ("static", "external"):
            problems.append("%s has a %s frame" % (short(title), kind))

    def visit(title, path):
        if title in path:
            cycle = path[path.index(title):] + [title]
            problems.append("recursion: " + " -> ".join(short(t) for t in cycle))
            return 0, []
        if title in depth:
            return depth[title], worst_chain[title]
        size, kind = nodes.get(title, (None, "external"))
        if kind == "external":
            return 0, []
        best = 0
        best_chain = []
        for callee in sorted(edges.get(title, ())):
            got, chain = visit(callee, path + [title])
            if got > best:
                best, best_chain = got, chain
        depth[title] = size + best
        worst_chain[title] = [short(title)] + best_chain
        return depth[title], worst_chain[title]

    for title, targets in edges.items():
        for target in targets:
            if target == "__indirect_call":
                indirect += 1
            elif nodes.get(target, (None, "external"))[1] == "external":
                externals.add(target)

    deepest = 0
    deepest_chain = []
    for title in sorted(nodes):
        if nodes[title][1] == "external":
            continue
        got, chain = visit(title, [])
        if got > deepest:
            deepest, deepest_chain = got, chain

    largest = max([size for size, kind in nodes.values() if size is not None] or [0])
    return deepest, deepest_chain, largest, sorted(externals), indirect, sorted(set(problems))


def modules():
    found = []
    for domain in sorted(os.listdir(os.path.join(REPO, "src"))):
        folder = os.path.join(REPO, "src", domain)
        for name in sorted(os.listdir(folder)):
            if name.endswith(".c"):
                found.append((domain, name[:-2]))
    return found


def control(cc, work):
    path = os.path.join(work, "control.c")
    with open(path, "w", encoding="utf-8") as handle:
        handle.write(CONTROL)
    problems = analyse(*compile_graph(cc, CORES[0], path, None, work))[5]
    text = " | ".join(problems)
    return ("recursion" in text) and ("dynamic" in text or "bounded" in text), text


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--cc", default="arm-none-eabi-gcc")
    parser.add_argument("--annotate", action="store_true")
    args = parser.parse_args()

    status = 0
    with tempfile.TemporaryDirectory(prefix="escsafelib-stack-") as work:
        live, text = control(args.cc, work)
        if not live:
            print("CONTROL FAILED: a recursive function and a variable length array were not both reported")
            print("  " + text)
            return 2
        print("control: recursion and a dynamic frame are both reported")

        for core in CORES:
            print()
            print("== %s, -Os ==" % core)
            print("%-10s %8s %8s  %-40s %s" % ("module", "deepest", "frame", "deepest chain", "not followed"))
            summary = []
            for domain, module in modules():
                source = os.path.join(REPO, "src", domain, module + ".c")
                include = os.path.join(REPO, "inc", domain)
                nodes, edges = compile_graph(args.cc, core, source, include, work)
                deepest, chain, largest, externals, indirect, problems = analyse(nodes, edges)
                beyond = list(externals)
                if indirect:
                    beyond.append("%d indirect" % indirect)
                print("%-10s %8d %8d  %-40s %s" % (module, deepest, largest, " > ".join(chain)[:40],
                                                    ", ".join(beyond) or "-"))
                summary.append("%s %d" % (module, deepest))
                for problem in problems:
                    status = 1
                    print("  FINDING %s: %s" % (module, problem))
                    if args.annotate:
                        print("::error::stack %s %s: %s" % (core, module, problem))
            if args.annotate:
                print("::notice title=stack on %s, deepest chain in bytes::%s" % (core, ", ".join(summary)))

    print()
    print("Bytes of stack below the caller, frames added up along the deepest")
    print("chain. A runtime helper or an injected function adds its own.")
    return status


if __name__ == "__main__":
    sys.exit(main())
