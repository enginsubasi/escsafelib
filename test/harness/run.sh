#!/usr/bin/env bash
#
# Run the platform harnesses.
#
#   bash test/harness/run.sh
#
# These are not part of any test suite. They are Windows only, they use
# VirtualAlloc and CreateFileMapping directly, and they exist because a
# whole class of defect is invisible to a portable test.
#
# The guard page harness catches an over-read: on a host, reading past the
# end of a buffer reads bytes that are there, so the answer comes back
# correct and every assertion passes. Five mutants survive the portable
# suites for exactly that reason and are killed only here. One of them was
# a real defect in this library.
#
# **Nothing here reports a result until the mechanism has been shown to
# work.** Case 0 of the guard page harness deliberately reads past the end
# and must die; if it lives, the page was never armed and every other
# result is meaningless, so the script stops. The same rule the ASan check
# in tools/run_all.sh follows, for the same reason.
#
# On this machine an access violation surfaces through Git Bash as exit
# code 139, and through cmd as 0xC0000005. The script accepts any non zero
# exit as a fault and prints the code, because which one you get depends on
# the shell rather than on the program.

REPO="$( cd "$( dirname "$0" )/../.." && pwd )"
OUT="${TMPDIR:-/tmp}/escsafelib-harness"

cd "$REPO" || exit 2

rm -rf "$OUT"
mkdir -p "$OUT"

CC="${1:-gcc}"

echo "== building the guard page harness =="
if ! $CC -std=c99 -Wall -Wextra -g \
     -Iinc/string -Iinc/memory -Iinc/array \
     test/harness/overread.c \
     src/string/sstring.c src/memory/smemory.c src/array/sarray.c \
     -o "$OUT/overread.exe" 2>"$OUT/build.log"; then
  echo "  BUILD FAILED"
  sed 's/^/    /' "$OUT/build.log" | head -20
  exit 2
fi
echo "  built"
echo

echo "== is the guard page armed? =="
"$OUT/overread.exe" 0 >"$OUT/control.log" 2>&1
controlExit=$?

if [ "$controlExit" -eq 0 ]; then
  echo "  INERT: the control read past the end and lived."
  sed 's/^/    /' "$OUT/control.log"
  echo "  Every result below would be meaningless, so nothing else is run."
  exit 1
fi

echo "  armed: the control faulted as it must, exit $controlExit"
echo

echo "== the bounded read rules =="
fail=0

runCase () {
  local n="$1"
  local what="$2"
  "$OUT/overread.exe" "$n" >"$OUT/case$n.log" 2>&1
  local rc=$?

  if [ "$rc" -eq 0 ]; then
    printf "  %-56s ok    %s\n" "$what" "$(head -1 "$OUT/case$n.log")"
  else
    printf "  %-56s FAULTED exit %s\n" "$what" "$rc"
    sed 's/^/      /' "$OUT/case$n.log"
    fail=1
  fi
}

runCase 1 "sstringCopy from an unterminated source"
runCase 2 "sstringConcat from an unterminated source"
runCase 3 "smemoryCompare against a longer buffer"
runCase 4 "sarrayCompareu8 against a longer array"

echo
if [ "$fail" -eq 0 ]; then
  echo "no function read past the end of its own buffer"
else
  echo "SOMETHING READ PAST ITS BUFFER, see above"
fi

echo
echo "== building the address aliasing harness =="
if ! $CC -std=c99 -Wall -Wextra -g -Iinc/diag \
     test/harness/aliasing.c src/diag/sdiag.c \
     -o "$OUT/aliasing.exe" 2>"$OUT/abuild.log"; then
  echo "  BUILD FAILED"
  sed 's/^/    /' "$OUT/abuild.log" | head -20
  exit 2
fi
echo "  built"
echo

echo "== are the two halves really the same storage? =="
"$OUT/aliasing.exe" 1 >"$OUT/alias0.log" 2>&1
aliasExit=$?
sed 's/^/  /' "$OUT/alias0.log"

if [ "$aliasExit" -ne 0 ]; then
  echo "  the mapping did not alias, so nothing below would mean anything."
  exit 1
fi
echo

echo "== the memory tests against a real address decoder fault =="

aliasCase () {
  local n="$1"
  local what="$2"
  "$OUT/aliasing.exe" "$n" >"$OUT/alias$n.log" 2>&1
  local rc=$?

  if [ "$rc" -eq 0 ]; then
    printf "  %-46s as specified\n" "$what"
    sed 's/^/    /' "$OUT/alias$n.log"
  else
    printf "  %-46s NOT AS SPECIFIED\n" "$what"
    sed 's/^/    /' "$OUT/alias$n.log"
    fail=1
  fi
}

aliasCase 2 "destructive test finds it"
aliasCase 3 "non destructive test cannot, and says so"
aliasCase 4 "healthy memory of the same size passes both"

echo
echo "  This closes four of the twenty two lines the portable suite cannot"
echo "  reach in sdiag: one March element catches the aliasing, so only that"
echo "  element's failure path runs. The rest need a cell that accepts a"
echo "  write and returns something else, which nothing portable can make."

echo
if [ "$fail" -eq 0 ]; then
  echo "ALL HARNESSES BEHAVED AS SPECIFIED"
else
  echo "SOMETHING DID NOT, see above"
fi

exit "$fail"
