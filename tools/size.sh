#!/usr/bin/env bash
#
# What each module costs on a target.
#
#   bash tools/size.sh
#
# The first question an integrator asks about an embedded library is how
# much flash it takes, and until this existed the repository could not
# answer it. Nothing else here needs it and no build depends on it.
#
# Two cores, because the answer is not the same on both. A Cortex-M0+ has
# no hardware divide and no Thumb-2, so anything doing 64 bit arithmetic
# costs noticeably more there; sfixed, svote and sscale all do.
#
# **These are upper bounds and should be read as such.** Each module is
# compiled whole and measured as one object, so the number covers every
# function in it whether a caller uses them or not. Built with
# -ffunction-sections and linked with --gc-sections, a project that calls
# four functions out of sarray's ninety two pays for four. The honest way
# to use this table is as the cost of taking the whole module, which is the
# worst case and the easy one to compare.
#
# .text is code and constants and lands in flash. .data is initialised
# variables, which cost both flash and RAM. .bss is zeroed variables and
# costs RAM only. Every module here should show .data and .bss of zero:
# there is no module state anywhere in this library, and a number other
# than zero in either column means somebody added some.

REPO="$( cd "$( dirname "$0" )/.." && pwd )"
OUT="${TMPDIR:-/tmp}/escsafelib-size"

cd "$REPO" || exit 2

rm -rf "$OUT"
mkdir -p "$OUT"

CC="${1:-arm-none-eabi-gcc}"
SIZE="${2:-arm-none-eabi-size}"

if ! command -v "$CC" >/dev/null 2>&1; then
  echo "$CC not found"
  exit 2
fi

echo "== $( "$CC" -dumpversion ) for arm-none-eabi, -Os =="
echo

fail=0

for core in cortex-m0plus cortex-m4; do
  printf "%-10s %10s %8s %8s   (%s)\n" module .text .data .bss "$core"
  printf -- "------------------------------------------------\n"

  total=0

  for f in src/*/*.c; do
    d=$( basename "$( dirname "$f" )" )
    n=$( basename "$f" .c )

    if ! "$CC" -c -Os -mthumb -mcpu="$core" -std=c99 \
         -ffunction-sections -fdata-sections \
         -I"inc/$d" "$f" -o "$OUT/$n.o" 2>"$OUT/$n.log"; then
      printf "%-10s BUILD FAILED\n" "$n"
      sed 's/^/    /' "$OUT/$n.log" | head -5
      fail=1
      continue
    fi

    read -r text data bss rest <<< "$( "$SIZE" "$OUT/$n.o" | tail -1 )"
    printf "%-10s %10s %8s %8s\n" "$n" "$text" "$data" "$bss"
    total=$(( total + text ))

    if [ "$data" != "0" ] || [ "$bss" != "0" ]; then
      echo "    ^^ this module has state; the library is supposed to have none"
      fail=1
    fi
  done

  printf -- "------------------------------------------------\n"
  printf "%-10s %10s\n" "all of it" "$total"
  echo
done

echo "Upper bounds: each module is measured whole. With -ffunction-sections"
echo "and --gc-sections a project pays for the functions it calls."

exit "$fail"
