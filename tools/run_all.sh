#!/usr/bin/env bash
#
# Build and run every test suite, then the checks that are worth running but
# are not part of any single build: a warning set stricter than the project
# normally uses, the sanitizers, and a check that the generated modules still
# match their generators.
#
# It is a convenience for a developer, not a build system. Nothing in the
# library needs it and no target toolchain runs it.
#
#   bash tools/run_all.sh                        # gcc
#   bash tools/run_all.sh clang
#   bash tools/run_all.sh "python -m ziglang cc"
#
# Exits non zero if anything fails, so it can gate a commit.
#
# The one thing to understand before reading its output: it refuses to report
# an AddressSanitizer result until it has proved ASan is armed, by building
# and running a control that reads past an allocation and must fault. On this
# repository's development machine neither host compiler has a working ASan,
# and one of the two failure modes is silent -- it links and detects nothing.
# A suite that "passes under ASan" in that state has been checked by nothing.

# Split on spaces so a multi word driver such as "python -m ziglang cc" works
# as well as a plain "gcc".
read -r -a CC <<< "${1:-gcc}"

REPO="$( cd "$( dirname "$0" )/.." && pwd )"
OUT="${TMPDIR:-/tmp}/escsafelib-check"

cd "$REPO" || exit 2

# Emptied rather than reused. On Windows a compiler asked for "sring_test"
# writes "sring_test.exe", and this script then runs "sring_test" -- which
# resolves to the .exe only when no extensionless file of that name is
# already there. One earlier run with a different compiler leaves exactly
# such a file, and every later run silently reports that stale binary's
# results under the new compiler's name. It is the worst kind of harness
# bug: it reports a pass, from a build nobody asked for.
rm -rf "$OUT"
mkdir -p "$OUT"

echo "== compiler =="
"${CC[@]}" --version 2>/dev/null | head -1 || { echo "  ${CC[*]} not found"; exit 2; }
echo

MODULES=(
  "fixed    sfixed        SFixed"
  "filter   sfilter       SFilter"
  "string   sstring       SString"
  "array    sarray        SArray"
  "memory   smemory       SMemory"
  "math     smath         SMath"
  "ring     sring         SRing"
  "diag     sdiag         SDiag"
  "scale    sscale        SScale"
  "vote     svote         SVote"
)

fail=0

echo "== build and run, -Wall -Wextra -Wpedantic =="
for m in "${MODULES[@]}"; do
  set -- $m
  d="$1"; n="$2"; t="$3"
  if ! "${CC[@]}" -Wall -Wextra -Wpedantic -std=c99 -g \
       -I"inc/$d" "test/${t}_Test/${t}_Test.c" "src/$d/$n.c" \
       -o "$OUT/${n}_test" 2>"$OUT/${n}_build.log"; then
    echo "  BUILD FAILED  $n"; sed 's/^/      /' "$OUT/${n}_build.log" | head -20; fail=1; continue
  fi
  if [ -s "$OUT/${n}_build.log" ]; then
    echo "  WARNINGS      $n"; sed 's/^/      /' "$OUT/${n}_build.log" | head -20; fail=1
  fi
  "$OUT/${n}_test" >"$OUT/${n}_run.log" 2>&1; rc=$?
  printf "  %-14s %-24s exit=%s\n" "$n" "$(tail -1 "$OUT/${n}_run.log")" "$rc"
  [ "$rc" -ne 0 ] && fail=1
done

echo
echo "== stricter warning set than anything run so far =="
for m in "${MODULES[@]}"; do
  set -- $m
  d="$1"; n="$2"
  if "${CC[@]}" -c -std=c99 -Wall -Wextra -Wpedantic -Wshadow -Wconversion \
       -Wsign-conversion -Wcast-qual -Wcast-align -Wstrict-prototypes \
       -Wmissing-prototypes -Wredundant-decls -Wundef -Wwrite-strings \
       -I"inc/$d" "src/$d/$n.c" -o "$OUT/$n.o" 2>"$OUT/${n}_strict.log"; then
    count=$(grep -c "warning:" "$OUT/${n}_strict.log" 2>/dev/null; true)
    printf "  %-14s %s warnings\n" "$n" "$count"
  else
    printf "  %-14s FAILED TO COMPILE\n" "$n"
  fi
done
echo "  (full text in $OUT/*_strict.log; these flags are stricter than the"
echo "   project has ever used, so a non zero count is information, not a bug)"

echo
echo "== is AddressSanitizer actually live? =="
# Under zig on this Windows host, -fsanitize=address alone fails to link with
# undefined __asan_report_* symbols, but -fsanitize=address,undefined links
# happily and then detects nothing at all. A suite that "passes under ASan"
# in that state has been checked by nothing. So prove the sanitizer is armed
# with a control that must fault before believing any result from it.
cat > "$OUT/asan_control.c" <<'CTL'
#include <stdio.h>
#include <stdlib.h>
int main ( void )
{
    int* p = ( int* ) malloc ( 4 * sizeof ( int ) );
    if ( p == NULL ) { return 2; }
    p[ 0 ] = 1;
    printf ( "%d\n", p[ 7 ] );   /* deliberate heap over-read */
    free ( p );
    return 0;
}
CTL

asanLive=0
if "${CC[@]}" -std=c99 -g -fsanitize=address,undefined -fno-sanitize-recover=all \
     -fno-omit-frame-pointer "$OUT/asan_control.c" -o "$OUT/asan_control" 2>"$OUT/asan_control.log"; then
  "$OUT/asan_control" >"$OUT/asan_control_run.log" 2>&1
  if [ "$?" -ne 0 ]; then
    asanLive=1
    echo "  armed: the control faulted as it must"
  else
    echo "  INERT: the control read past its allocation and exited cleanly."
    echo "  Every sanitizer result below would be meaningless, so they are skipped."
  fi
else
  echo "  the control does not link, so ASan is unavailable with this compiler"
fi

if [ "$asanLive" -eq 1 ]; then
  echo
  echo "== ASan and UBSan =="
  for m in "${MODULES[@]}"; do
    set -- $m
    d="$1"; n="$2"; t="$3"
    if "${CC[@]}" -std=c99 -g -fsanitize=address,undefined -fno-sanitize-recover=all \
         -fno-omit-frame-pointer -I"inc/$d" \
         "test/${t}_Test/${t}_Test.c" "src/$d/$n.c" -o "$OUT/${n}_san" 2>"$OUT/${n}_san.log"; then
      "$OUT/${n}_san" >"$OUT/${n}_sanrun.log" 2>&1; rc=$?
      printf "  %-14s exit=%s  %s\n" "$n" "$rc" "$(tail -1 "$OUT/${n}_sanrun.log")"
      [ "$rc" -ne 0 ] && fail=1
    else
      printf "  %-14s SANITIZER BUILD FAILED (see %s)\n" "$n" "$OUT/${n}_san.log"
      fail=1
    fi
  done
fi

echo
echo "== UBSan in trap mode, which needs no runtime and always works =="
for m in "${MODULES[@]}"; do
  set -- $m
  d="$1"; n="$2"; t="$3"
  if "${CC[@]}" -std=c99 -g -fsanitize=undefined -fsanitize-trap=undefined \
       -I"inc/$d" "test/${t}_Test/${t}_Test.c" "src/$d/$n.c" \
       -o "$OUT/${n}_ub" 2>"$OUT/${n}_ub.log"; then
    "$OUT/${n}_ub" >"$OUT/${n}_ubrun.log" 2>&1; rc=$?
    printf "  %-14s exit=%s  %s\n" "$n" "$rc" "$(tail -1 "$OUT/${n}_ubrun.log")"
    [ "$rc" -ne 0 ] && fail=1
  else
    printf "  %-14s UBSAN BUILD FAILED\n" "$n"; fail=1
  fi
done

echo
echo "== doxygen comments match the convention =="
if python tools/doxcheck.py > "$OUT/doxcheck.log" 2>&1; then
  echo "  $(tail -1 "$OUT/doxcheck.log")"
else
  sed 's/^/  /' "$OUT/doxcheck.log"
  fail=1
fi

echo
echo "== generated modules have not drifted =="
# Scoped to the generated files only. A bare "git diff" would also flag every
# uncommitted edit elsewhere in the tree, which makes this report DRIFT during
# any normal working session and trains the reader to ignore it.
GENERATED="inc/array src/array test/SArray_Test
           inc/math src/math test/SMath_Test"
python tools/gen_sarray.py >/dev/null && python tools/gen_sarray_test.py >/dev/null
python tools/gen_smath.py >/dev/null && python tools/gen_smath_test.py >/dev/null
if git diff --quiet -- $GENERATED; then
  echo "  no drift"
else
  echo "  DRIFT in the generated modules:"
  git diff --stat -- $GENERATED | sed 's/^/    /'
  fail=1
fi

echo
if [ "$fail" -eq 0 ]; then echo "ALL GREEN"; else echo "SOMETHING FAILED, see above"; fi
exit "$fail"
