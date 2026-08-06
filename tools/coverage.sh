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

read -r -a CC <<< "${1:-gcc}"

REPO="$( cd "$( dirname "$0" )/.." && pwd )"
OUT="${TMPDIR:-/tmp}/escsafelib-coverage"

cd "$REPO" || exit 2

rm -rf "$OUT"
mkdir -p "$OUT"

MODULES=(
  "array    sarray        SArray"
  "diag     sdiag         SDiag"
  "fault    sfault        SFault"
  "filter   sfilter       SFilter"
  "fixed    sfixed        SFixed"
  "math     smath         SMath"
  "memory   smemory       SMemory"
  "ring     sring         SRing"
  "scale    sscale        SScale"
  "state    sstate        SState"
  "string   sstring       SString"
  "vote     svote         SVote"
  "watch    swatch        SWatch"
)

fail=0

printf "%-10s %10s %10s %10s %10s\n" module lines "lines %" branches "taken %"
printf -- "---------------------------------------------------------\n"

for m in "${MODULES[@]}"; do
  set -- $m
  d="$1"; n="$2"; t="$3"

  work="$OUT/$n"
  mkdir -p "$work"

  if ! "${CC[@]}" --coverage -O0 -std=c99 -I"$REPO/inc/$d" \
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

  ( cd "$work" && gcov -b "$n.gcda" >"$work/gcov.log" 2>&1 )

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

  printf "%-10s %10s %9s%% %10s %9s%%\n" \
    "$n" "${lines:-?}" "${linepc:-?}" "${branches:-?}" "${takenpc:-?}"

  # The lines gcov marks ##### were never executed. Pull them out with their
  # numbers so each one can be looked at rather than counted.
  if [ -f "$work/$n.c.gcov" ]; then
    grep -n '#####' "$work/$n.c.gcov" > "$OUT/${n}.uncovered.txt" 2>/dev/null
    grep -n 'never executed' "$work/$n.c.gcov" > "$OUT/${n}.untakenbranch.txt" 2>/dev/null
  fi
done

echo
echo "annotated listings:   $OUT/<module>/<module>.c.gcov"
echo "uncovered lines:      $OUT/<module>.uncovered.txt"
echo "branches never taken: $OUT/<module>.untakenbranch.txt"
echo
echo "A percentage is not the result. Classify every entry: a case nobody"
echo "wrote is a hole, a branch the API cannot reach is a comment."

exit "$fail"
