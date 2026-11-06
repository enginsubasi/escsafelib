#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Runs the cppcheck MISRA C:2012 addon over every module, with a control
# first and the recorded deviations applied.
#
#   bash tools/misra.sh                 # every module
#   bash tools/misra.sh sfilter svote   # some of them
#   bash tools/misra.sh --annotate      # findings as GitHub annotations
#
# Exits 0 when nothing is left after the deviations, 1 when something is,
# and 2 when the addon is not live.
#
# Why a script and not "cppcheck --addon=misra". On the development machine
# and on the CI runner alike, --addon=misra printed nothing and returned
# zero, including for a control file built to break five rules. Running
# cppcheck --dump and then the addon by hand works on both, so that is the
# route taken here, and the control runs first every time: a MISRA check
# that cannot be shown to fire has not checked anything.
#
# Tool locations can be given in the environment: CPPCHECK, MISRA_PY and
# PYTHON. Otherwise cppcheck comes from PATH and misra.py is looked for in
# the places cppcheck installs it.

set -u
set -o pipefail
set -f   # the deviation table holds a literal *, which must not glob

ANNOTATE=0
MODULES=()
for arg in "$@"; do
    case "$arg" in
        --annotate) ANNOTATE=1 ;;
        *)          MODULES+=( "$arg" ) ;;
    esac
done

REPO=$(cd "$(dirname "$0")/.." && pwd)
CPPCHECK=${CPPCHECK:-cppcheck}
PYTHON=${PYTHON:-}

if [ -z "$PYTHON" ]; then
    for p in python3 python; do
        if "$p" -c "import sys" > /dev/null 2>&1; then PYTHON=$p; break; fi
    done
fi

native () {
    # Native Windows tools cannot read an MSYS path such as /tmp/x.
    if command -v cygpath > /dev/null 2>&1; then cygpath -m "$1"; else echo "$1"; fi
}

if [ -z "${MISRA_PY:-}" ]; then
    bindir=$(dirname "$(command -v "$CPPCHECK")")
    for c in "$bindir/../share/cppcheck/addons/misra.py" \
             "$bindir/../share/Cppcheck/addons/misra.py" \
             "$bindir/addons/misra.py" \
             /usr/share/cppcheck/addons/misra.py \
             /usr/lib/x86_64-linux-gnu/cppcheck/addons/misra.py \
             /usr/local/share/cppcheck/addons/misra.py; do
        if [ -f "$c" ]; then MISRA_PY=$c; break; fi
    done
fi

if [ -z "${MISRA_PY:-}" ] || [ -z "$PYTHON" ]; then
    echo "misra.sh: cannot find misra.py or a python interpreter"
    exit 2
fi

WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT

"$CPPCHECK" --version
echo "addon: $MISRA_PY"

# ------------------------------------------------------------------ control
#
# An octal constant (7.1) and a goto (15.1). If either is missing from the
# report, the addon did not run and nothing it says below can be trusted.

printf 'int f(int x)\n{\n    int y = 010;\n    if (x) goto end;\n    y = x;\nend:\n    return y;\n}\n' > "$WORK/control.c"
"$CPPCHECK" --dump --std=c99 "$(native "$WORK/control.c")" > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "CONTROL FAILED: cppcheck could not create the control dump"
    exit 2
fi
"$PYTHON" "$(native "$MISRA_PY")" "$(native "$WORK/control.c.dump")" > "$WORK/control.log" 2>&1
if ! grep -q "misra-c2012-7.1" "$WORK/control.log" || ! grep -q "misra-c2012-15.1" "$WORK/control.log"; then
    echo "CONTROL FAILED: the MISRA addon did not report a file built to break it"
    cat "$WORK/control.log"
    [ "$ANNOTATE" -eq 1 ] && echo "::error::the MISRA addon is not live; see the log"
    exit 2
fi
echo "control: the addon reports 7.1 and 15.1 on a file built to break them"

# --------------------------------------------------------------- deviations
#
# rule  modules (or *)  -- the argument for each is recorded in CLAUDE.md
# and, for the per-module ones, in that module's source banner.
# Rule 10.4 is matched against tools/misra_10_4.tsv by source-line hash.
#
#   10.4  every module. A status is a uint8_t and is compared with the
#         module's enum members, with TRUE and FALSE, and with unsuffixed
#         literals. The essential types differ by design: one status type
#         across the whole library is what keeps every function returning
#         the same thing, and it is the C type the comparison really uses.
#   11.4  pointer to integer, to test two byte ranges for overlap and to
#   11.6  choose a copy direction. There is no conforming way to compare
#         pointers into different objects, and corrupting overlapping
#         buffers silently is the worse outcome.
#   11.5  void* to unsigned char*, the one object type the standard always
#         allows for examining the bytes of an object.

DEVIATIONS="
11.4 sarray sstring
11.6 sarray smemory sstring
11.5 sdiag smemory
"

deviated () {
    # deviated <rule> <module> -> 0 when the pair is a recorded deviation.
    local rule=$1 module=$2 entry who
    while read -r entry; do
        [ -z "$entry" ] && continue
        set -- $entry
        if [ "$1" = "$rule" ]; then
            shift
            for who in "$@"; do
                if [ "$who" = "*" ] || [ "$who" = "$module" ]; then return 0; fi
            done
        fi
    done <<< "$DEVIATIONS"
    return 1
}

# ------------------------------------------------------------------ modules

cd "$REPO"
if [ ${#MODULES[@]} -eq 0 ]; then
    set +f
    for f in src/*/*.c; do MODULES+=( "$(basename "$f" .c)" ); done
    set -f
    WHOLE=1
else
    WHOLE=0
fi

: > "$WORK/findings"
: > "$WORK/deviated"
: > "$WORK/rule_10_4"
status=0

for m in "${MODULES[@]}"; do
    set +f
    src=$(ls src/*/"$m".c 2> /dev/null | head -1)
    set -f
    if [ -z "$src" ]; then echo "misra.sh: no module $m"; exit 2; fi
    d=$(basename "$(dirname "$src")")
    cp "$src" "$WORK/$m.c"
    "$CPPCHECK" --dump --std=c99 --platform=unix32 --suppress=missingIncludeSystem \
        -I"$(native "$REPO/inc/$d")" "$(native "$WORK/$m.c")" > /dev/null 2>&1
    if [ $? -ne 0 ]; then
        echo "MISRA FAILED: cppcheck could not create a dump for $m"
        exit 2
    fi
    "$PYTHON" "$(native "$MISRA_PY")" "$(native "$WORK/$m.c.dump")" 2>&1 \
        | sed -nE 's/^\[[^]]*[\\/]'"$m"'\.c:([0-9]+)\].*\[misra-c2012-([0-9.]+)\].*$/\1 \2/p' \
        | while read -r line rule; do
              if [ "$rule" = "10.4" ]; then
                  printf '%s\t%s\n' "$m" "$line" >> "$WORK/rule_10_4"
              elif deviated "$rule" "$m"; then
                  echo "$rule $m" >> "$WORK/deviated"
              else
                  echo "$src $line $rule" >> "$WORK/findings"
              fi
          done
done

if [ "$WHOLE" -eq 1 ]; then
        if ! "$PYTHON" "$(native "$REPO/tools/misra_occurrences.py")" \
        --repo "$(native "$REPO")" \
        --manifest "$(native "$REPO/tools/misra_10_4.tsv")" \
        --actual "$(native "$WORK/rule_10_4")"; then
            status=1
        fi
    else
        if ! "$PYTHON" "$(native "$REPO/tools/misra_occurrences.py")" \
            --repo "$(native "$REPO")" \
            --manifest "$(native "$REPO/tools/misra_10_4.tsv")" \
            --actual "$(native "$WORK/rule_10_4")" \
            --modules "${MODULES[@]}"; then
            status=1
        else
            :
        fi
fi

# ------------------------------------------------------------------- report

if [ -s "$WORK/findings" ]; then
    status=1
    echo
    echo "findings outside the recorded deviations:"
    while read -r file line rule; do
        if [ "$ANNOTATE" -eq 1 ]; then
            echo "::error file=$file,line=$line::MISRA C:2012 Rule $rule"
        else
            echo "  $file:$line  Rule $rule"
        fi
    done < "$WORK/findings"
fi

echo
echo "by rule, outside the deviations:"
awk '{ print $3 }' "$WORK/findings" | sort | uniq -c | sort -rn
echo "by rule, inside the deviations:"
awk '{ print $1 }' "$WORK/deviated" | sort | uniq -c | sort -rn
echo "matched Rule 10.4 allowlist occurrences: $(wc -l < "$WORK/rule_10_4")"

# A deviation nothing needs any more is a claim the code no longer makes,
# and the record should go. Only meaningful over the whole tree.
if [ "$WHOLE" -eq 1 ]; then
    while read -r line; do
        [ -z "$line" ] && continue
        set -- $line
        rule=$1; shift
        for m in "$@"; do
            if [ "$m" = "*" ]; then
                pattern="^$rule "
            else
                pattern="^$rule $m\$"
            fi
            if ! grep -qE "$pattern" "$WORK/deviated"; then
                status=1
                msg="deviation $rule for $m is recorded but nothing needs it"
                if [ "$ANNOTATE" -eq 1 ]; then echo "::error::$msg"; else echo "  $msg"; fi
            fi
        done
    done <<< "$DEVIATIONS"
fi

total=$(wc -l < "$WORK/findings")
echo
echo "${#MODULES[@]} modules, $total findings outside the recorded deviations"
exit $status
