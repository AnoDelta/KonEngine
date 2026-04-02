#!/bin/bash
# run_tests.sh — KonScript progressive test runner
# Usage:
#   ./run_tests.sh            run all tests
#   ./run_tests.sh t03        run only tests matching "t03"
#   ./run_tests.sh --ir t02   emit LLVM IR for t02 (don't run)
#   ./run_tests.sh --cpp t02  transpile t02 to C++

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

# Colours — $'...' so \033 is an actual ESC byte, not literal backslash
GREEN=$'\033[32m'; RED=$'\033[31m'; YELLOW=$'\033[33m'
CYAN=$'\033[36m';  DIM=$'\033[2m';  BOLD=$'\033[1m';  RST=$'\033[0m'

pass() { echo "${GREEN}${BOLD}[PASS]${RST} $1"; }
fail() { echo "${RED}${BOLD}[FAIL]${RST} $1"; }

# Flags
IR_ONLY=0; CPP_ONLY=0; FILTER=""
for arg in "$@"; do
    case "$arg" in
        --ir)  IR_ONLY=1 ;;
        --cpp) CPP_ONLY=1 ;;
        --*)   echo "Unknown flag $arg"; exit 1 ;;
        *)     FILTER="$arg" ;;
    esac
done

TESTS_DIR="$SCRIPT_DIR/tests"
if [ ! -d "$TESTS_DIR" ]; then
    echo "No tests/ directory found at: $TESTS_DIR"; exit 1
fi

PASS=0; FAIL=0
TMP="$(mktemp -d)"
trap "rm -rf $TMP" EXIT

echo ""
echo "${BOLD}══════════════════════════════════════════${RST}"
echo "${BOLD}  KonScript Test Suite${RST}"
echo "${BOLD}══════════════════════════════════════════${RST}"
echo ""

for ks in "$TESTS_DIR"/t*.ks; do
    name="$(basename "$ks" .ks)"
    [ -n "$FILTER" ] && [[ "$name" != *"$FILTER"* ]] && continue

    echo "${DIM}────────────────────────────────────────${RST}"
    echo "  ${BOLD}$name${RST}"

    bin="$TMP/$name"

    if [ "$IR_ONLY" -eq 1 ]; then
        konscript --llvm "$ks" -o "$bin.ll"; cat "$bin.ll"; continue
    fi

    if [ "$CPP_ONLY" -eq 1 ]; then
        konscript --cpp "$ks" -o "$bin.cpp"; cat "$bin.cpp"; continue
    fi

    # Build — capture output, don't let failure abort the script
    build_out="$(konscript "$ks" -o "$bin" 2>&1)"
    build_code=$?
    if [ $build_code -ne 0 ]; then
        fail "$name — build failed (exit $build_code)"
        echo "$build_out" | sed 's/^/    /'
        FAIL=$((FAIL+1))
        continue
    fi

    # Run
    run_out="$("$bin" 2>&1)"
    run_code=$?
    if [ $run_code -eq 0 ]; then
        pass "$name  (exit 0)"
        while IFS= read -r line; do
            echo "    ${DIM}${line}${RST}"
        done <<< "$run_out"
        PASS=$((PASS+1))
    else
        fail "$name — exit code $run_code"
        echo "$run_out" | sed 's/^/    /'
        FAIL=$((FAIL+1))
    fi
done

echo ""
echo "${BOLD}══════════════════════════════════════════${RST}"
echo "  ${GREEN}${BOLD}${PASS} passed${RST}  ${RED}${BOLD}${FAIL} failed${RST}"
echo "${BOLD}══════════════════════════════════════════${RST}"
echo ""

[ $FAIL -eq 0 ]
