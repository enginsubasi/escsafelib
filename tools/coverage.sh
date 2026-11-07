#!/usr/bin/env bash
#
# Measure statement and branch coverage of every module against its own test
# suite, using gcc's --coverage and gcov.
#
#   bash tools/coverage.sh                 # gcc
#   bash tools/coverage.sh <compiler>
#
# Why this exists alongside the mutation testing: mutation testing samples,
# coverage counts. A module cleared against six mutants has been shown to
# catch those six, and nothing about the paths nobody thought to mutate.
# The first run of this script found that sscale, the most heavily tested
# module in the library, never took ten percent of its own branches.
#
# Each module is built in its own directory, because gcov keys its data
# files off the object name and two modules whose test programs share a
# basename would otherwise overwrite each other.
#
# Every uncovered line is one of two things and the difference matters:
#
#   - a case nobody wrote, which is a hole and wants a test;
#   - a defensive branch that cannot be reached through the API, which is
#     not a hole and wants a comment saying why.
#
# The annotated listings are left behind so that each one can be classified
# rather than guessed at. Read them, do not just read the percentages.
#
# When the compiler accepts -fcondition-coverage (gcc 14 and later), the
# table gains a column of MC/DC: the share of condition outcomes shown to
# change their decision on their own. Branch coverage of
# "if ( ( a == NULL ) || ( b == NULL ) )" is satisfied by one NULL and one
# good call; MC/DC also wants b alone to be shown to matter. ISO 26262-6
# Table 9 (2018 edition) recommends it at ASIL D. This measures it; it does not claim the
# library meets that table.

read -r -a CC <<< "${1:-gcc}"

REPO="$( cd "$( dirname "$0" )/.." && pwd )"
OUT="${TMPDIR:-/tmp}/escsafelib-coverage"

cd "$REPO" || exit 2

rm -rf "$OUT"
mkdir -p "$OUT"

# gcov has to be the same version as the compiler that wrote the data, so
# gcc-14 is read by gcov-14. GCOV in the environment overrides the choice.
GCOV=${GCOV:-gcov}
case "${CC[0]}" in
  *gcc-[0-9]*)
    candidate="gcov-${CC[0]##*gcc-}"
    if command -v "$candidate" > /dev/null 2>&1; then GCOV=$candidate; fi
    ;;
esac

MCDC=()
GCOVMCDC=()
summary=""
printf 'int f ( int a, int b ) { return ( a && b ); }\n' > "$OUT/probe.c"
if "${CC[@]}" --coverage -fcondition-coverage -c "$OUT/probe.c" -o "$OUT/probe.o" >/dev/null 2>&1; then
  MCDC=( -fcondition-coverage )
  GCOVMCDC=( -g )
fi

# Every module is found from src/, never listed: src/<domain>/<module>.c,
# and its suite is test/S<Module>_Test/, the module name with its second
# letter capitalised, the rule tools/tracecheck.py applies. A new module is
# built, run and measured here without this file changing.
MODULES=()
for f in src/*/*.c; do
  d=$( basename "$( dirname "$f" )" )
  n=$( basename "$f" .c )
  rest=${n:1}
  MODULES+=( "$d $n S${rest^}" )
done

fail=0

printf "%-10s %10s %10s %10s %10s %10s\n" module lines "lines %" branches "taken %" "MC/DC %"
printf -- "--------------------------------------------------------------------\n"

for m in "${MODULES[@]}"; do
  set -- $m
  d="$1"; n="$2"; t="$3"

  work="$OUT/$n"
  mkdir -p "$work"

  if ! "${CC[@]}" --coverage "${MCDC[@]}" -O0 -std=c99 -I"$REPO/inc/$d" \
       -c "$REPO/src/$d/$n.c" -o "$work/$n.o" 2>"$work/build.log"; then
    printf "%-10s BUILD FAILED\n" "$n"; fail=1; continue
  fi

  if ! "${CC[@]}" --coverage -O0 -std=c99 -I"$REPO/inc/$d" \
       -c "$REPO/test/${t}_Test/${t}_Test.c" -o "$work/test.o" 2>>"$work/build.log"; then
    printf "%-10s TEST BUILD FAILED\n" "$n"; fail=1; continue
  fi

  if ! "${CC[@]}" --coverage "$work/$n.o" "$work/test.o" -o "$work/run.exe" 2>>"$work/build.log"; then
    printf "%-10s LINK FAILED\n" "$n"; fail=1; continue
  fi

  ( cd "$work" && ./run.exe >"$work/run.log" 2>&1 )
  rc=$?

  if [ "$rc" -ne 0 ]; then
    printf "%-10s SUITE FAILED exit=%s\n" "$n" "$rc"; fail=1; continue
  fi

  # The integration suite exercises several of the modules as a chain, so its
  # coverage belongs in these numbers. Linking it against the same object
  # accumulates into the same data file, which is what gcov counts.
  if grep -q "\"$n.h\"" "$REPO/test/Integration_Test/Integration_Test.c"; then
    if "${CC[@]}" --coverage -O0 -std=c99 \
         $( for d in "$REPO"/inc/*/; do printf -- " -I%s" "${d%/}"; done ) \
         "$REPO/test/Integration_Test/Integration_Test.c" \
         $( for g in "$REPO"/src/*/*.c; do
              gn=$( basename "$g" .c )
              if [ "$gn" = "$n" ]; then echo "$work/$n.o"; else echo "$g"; fi
            done ) \
         -o "$work/integration.exe" 2>>"$work/build.log"; then
      ( cd "$work" && ./integration.exe >/dev/null 2>&1 )
    else
      echo "  (the integration suite did not build for $n)"
    fi
  fi

  ( cd "$work" && "$GCOV" -b "${GCOVMCDC[@]}" "$n.gcda" >"$work/gcov.log" 2>&1 )

  # gcov reports each file it was asked about in turn. Take the block that
  # names the module's own source, not the test program's.
  block=$(awk -v want="/src/$d/$n.c" '
    /^File / { keep = index( $0, want ) > 0 }
    keep { print }
  ' "$work/gcov.log")

  # head -1 on each: gcov repeats a summary after the per file block, so the
  # pattern matches twice and an unguarded capture yields two lines.
  lines=$(echo "$block"    | sed -n 's/^Lines executed:\([0-9.]*\)% of \([0-9]*\)$/\2/p' | head -1)
  linepc=$(echo "$block"   | sed -n 's/^Lines executed:\([0-9.]*\)% of \([0-9]*\)$/\1/p' | head -1)
  branches=$(echo "$block" | sed -n 's/^Branches executed:\([0-9.]*\)% of \([0-9]*\)$/\2/p' | head -1)
  takenpc=$(echo "$block"  | sed -n 's/^Taken at least once:\([0-9.]*\)% of \([0-9]*\)$/\1/p' | head -1)
  mcdcpc=$(echo "$block"   | sed -n 's/^Condition outcomes covered:\([0-9.]*\)% of \([0-9]*\)$/\1/p' | head -1)

  printf "%-10s %10s %9s%% %10s %9s%% %9s%%\n" \
    "$n" "${lines:-?}" "${linepc:-?}" "${branches:-?}" "${takenpc:-?}" "${mcdcpc:-n/a}"
  # A workflow command reads %XX as an escape, so a literal % is %25.
  summary="${summary}$n lines ${linepc:-?}%25, branches ${takenpc:-?}%25, MC/DC ${mcdcpc:-n/a}%25%0A"

  # The lines gcov marks ##### were never executed. Pull them out with their
  # numbers so each one can be looked at rather than counted.
  if [ -f "$work/$n.c.gcov" ]; then
    grep -n '#####' "$work/$n.c.gcov" > "$OUT/${n}.uncovered.txt" 2>/dev/null
    grep -n 'never executed' "$work/$n.c.gcov" > "$OUT/${n}.untakenbranch.txt" 2>/dev/null
    grep -n -B1 'not covered' "$work/$n.c.gcov" > "$OUT/${n}.conditions.txt" 2>/dev/null
  fi
done

echo
echo "annotated listings:   $OUT/<module>/<module>.c.gcov"
echo "uncovered lines:      $OUT/<module>.uncovered.txt"
echo "branches never taken: $OUT/<module>.untakenbranch.txt"
if [ "${#MCDC[@]}" -gt 0 ]; then
  echo "MC/DC gaps:           $OUT/<module>.conditions.txt"
else
  echo "MC/DC:                not measured, the compiler has no -fcondition-coverage"
fi
echo
echo "A percentage is not the result. Classify every entry: a case nobody"
echo "wrote is a hole, a branch the API cannot reach is a comment."

# On GitHub the table also goes out as one notice. A job's log needs admin
# rights to read and its annotations do not, so this is how the numbers CI
# measured, and whether it measured MC/DC at all, can be read from here.
# One notice rather than one per module, because a step shows only ten.
if [ "${GITHUB_ACTIONS:-}" = "true" ]; then
  echo "::notice title=coverage by ${CC[*]} and ${GCOV}::${summary}"
fi

# Every MC/DC gap must be one tools/mcdc_exceptions.tsv explains, and every
# record there must still be needed. A gap nobody explained is a missing
# case or an unreviewed one, so this, not a percentage, is what fails the
# run. Only when the compiler measured MC/DC at all.
if [ "${#MCDC[@]}" -gt 0 ]; then
  PYTHON=${PYTHON:-}
  if [ -z "$PYTHON" ]; then
    for candidate in python3 python; do
      if "$candidate" -c "import sys" > /dev/null 2>&1; then PYTHON=$candidate; break; fi
    done
  fi
  native() { if command -v cygpath > /dev/null 2>&1; then cygpath -w "$1"; else echo "$1"; fi; }
  ANNOTATE=()
  if [ "${GITHUB_ACTIONS:-}" = "true" ]; then ANNOTATE=( --annotate ); fi
  echo
  echo "== every MC/DC gap is a reviewed one =="
  if ! "$PYTHON" "$( native "$REPO/tools/mcdccheck.py" )" --self-test; then
    echo "CONTROL FAILED: the MC/DC gap checker failed its own controls"
    exit 2
  fi
  if ! "$PYTHON" "$( native "$REPO/tools/mcdccheck.py" )" --dir "$( native "$OUT" )" "${ANNOTATE[@]}"; then
    fail=1
  fi
fi

exit "$fail"
