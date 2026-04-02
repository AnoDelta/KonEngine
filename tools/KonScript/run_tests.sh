#!/usr/bin/env bash
# ---------------------------------------------------------------------------
# run_tests.sh — KonScript test suite runner
#
# Usage:
#   ./run_tests.sh                      run all tests (check + cpp + compile)
#   ./run_tests.sh --check-only         typecheck only, no transpile/compile
#   ./run_tests.sh --no-compile         typecheck + transpile, skip g++
#   ./run_tests.sh --verbose            show full compiler output on failure
#   ./run_tests.sh tests/05_enums.ks    run a single file
#   ./run_tests.sh tests/               run all files in a specific directory
#
# Exit code: 0 = all pass, 1 = any failure.
#
# The script auto-discovers the konscript binary in:
#   1. same directory as this script
#   2. ../  (i.e. tools/KonScript/ when tests/ is the subdir)
#   3. system PATH
#
# Stages run per test:
#   [check]     konscript --check     (typecheck only)
#   [cpp]       konscript --cpp       (transpile to C++)
#   [g++]       g++ -std=c++17 -O0   (compile generated C++)
#
# Engine-target files (#include <engine>) are checked and transpiled but
# skipped at the g++ stage unless KON_ENGINE_INC is set to your engine's
# src/ directory.
# ---------------------------------------------------------------------------
set -euo pipefail

# ── ANSI colors ──────────────────────────────────────────────────────────
RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'
CYAN='\033[0;36m'; BOLD='\033[1m'; DIM='\033[2m'; NC='\033[0m'

# ── Options ───────────────────────────────────────────────────────────────
VERBOSE=0
DO_COMPILE=1
CHECK_ONLY=0
SINGLE_FILE=""
TEST_DIR=""

for arg in "$@"; do
    case "$arg" in
        --verbose)      VERBOSE=1 ;;
        --no-compile)   DO_COMPILE=0 ;;
        --check-only)   CHECK_ONLY=1; DO_COMPILE=0 ;;
        --help|-h)
            sed -n '/^# Usage/,/^# -----------/p' "$0" | sed 's/^# \?//'
            exit 0 ;;
        *.ks)           SINGLE_FILE="$arg" ;;
        *)
            if [ -d "$arg" ]; then TEST_DIR="$arg"
            elif [ -n "$arg" ]; then echo "Unknown argument: $arg"; exit 1; fi ;;
    esac
done

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
[ -z "$TEST_DIR" ] && TEST_DIR="$SCRIPT_DIR/tests"

# ── Find konscript binary ─────────────────────────────────────────────────
find_ksc() {
    local candidates=(
        "$SCRIPT_DIR/konscript"
        "$SCRIPT_DIR/../konscript"
        "$SCRIPT_DIR/../../konscript"
        "$(command -v konscript 2>/dev/null || true)"
    )
    for c in "${candidates[@]}"; do
        [ -n "$c" ] && [ -x "$c" ] && echo "$c" && return
    done
    echo ""
}

KSC="$(find_ksc)"
if [ -z "$KSC" ]; then
    echo -e "${RED}ERROR${NC}: konscript binary not found."
    echo "  Build: cd tools/KonScript && ./build.sh"
    exit 1
fi

# ── Temp directory ────────────────────────────────────────────────────────
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

# ── Collect test files ────────────────────────────────────────────────────
declare -a TEST_FILES=()
if [ -n "$SINGLE_FILE" ]; then
    TEST_FILES=("$(realpath "$SINGLE_FILE")")
else
    while IFS= read -r -d '' f; do
        TEST_FILES+=("$f")
    done < <(find "$TEST_DIR" -maxdepth 1 -name "*.ks" -print0 | sort -z)
fi

if [ ${#TEST_FILES[@]} -eq 0 ]; then
    echo -e "${YELLOW}WARN${NC}: no .ks files found in $TEST_DIR"
    exit 0
fi

# ── Header ────────────────────────────────────────────────────────────────
echo ""
echo -e "${BOLD}KonScript Test Suite${NC}"
echo -e "${DIM}  binary : $KSC${NC}"
echo -e "${DIM}  tests  : ${#TEST_FILES[@]} files${NC}"
if [ -n "${KON_ENGINE_INC:-}" ]; then
    echo -e "${DIM}  engine : $KON_ENGINE_INC${NC}"
fi
echo ""
echo -e "${DIM}──────────────────────────────────────────────────────────────${NC}"

# ── Run tests ─────────────────────────────────────────────────────────────
TOTAL=0; PASS=0; FAIL=0; SKIP=0
declare -a FAILURES=()

run_file() {
    local ks_file="$1"
    local name
    name="$(basename "$ks_file" .ks)"
    TOTAL=$((TOTAL + 1))

    local is_engine=0
    grep -q '#include <engine>' "$ks_file" 2>/dev/null && is_engine=1

    # Files without a main() are module-only (no independent compile)
    local has_main=0
    grep -q 'func main()' "$ks_file" 2>/dev/null && has_main=1

    printf "  ${BOLD}%-32s${NC}" "$name"

    local failed_stages=()
    local cpp_file="$TMP/${name}.cpp"

    # ── Stage 1: typecheck ────────────────────────────────────────────────
    local check_out
    check_out="$("$KSC" --check "$ks_file" 2>&1)" || true
    if echo "$check_out" | grep -q "error:"; then
        failed_stages+=("check")
        if [ "$VERBOSE" = "1" ]; then
            echo ""
            echo "$check_out" | grep "error:" | sed "s/^/    ${DIM}/" | sed "s/$/${NC}/"
        else
            local first_err
            first_err="$(echo "$check_out" | grep "error:" | head -1 | sed 's|.*/||')"
            echo -e "${RED}FAIL${NC}  ${DIM}[check] $first_err${NC}"
            FAIL=$((FAIL + 1))
            FAILURES+=("$name  [check]  $first_err")
            return
        fi
    fi

    # ── Stage 2: transpile ────────────────────────────────────────────────
    if [ "$CHECK_ONLY" != "1" ]; then
        local cpp_out
        cpp_out="$("$KSC" --cpp "$ks_file" -o "$cpp_file" 2>&1)" || true
        if [ ! -f "$cpp_file" ]; then
            failed_stages+=("cpp")
        fi
    fi

    # If check-only or no main, stop here
    if [ "$CHECK_ONLY" = "1" ] || [ "$has_main" = "0" ]; then
        if [ ${#failed_stages[@]} -eq 0 ]; then
            echo -e "${GREEN}PASS${NC}"
            PASS=$((PASS + 1))
        else
            local stages
            stages="${failed_stages[*]}"
            echo -e "${RED}FAIL${NC}  ${DIM}[$stages]${NC}"
            FAIL=$((FAIL + 1))
            FAILURES+=("$name  [$stages]")
        fi
        return
    fi

    # ── Stage 3: compile generated C++ ───────────────────────────────────
    if [ "$DO_COMPILE" = "1" ] && [ ${#failed_stages[@]} -eq 0 ]; then
        if [ "$is_engine" = "1" ]; then
            if [ -n "${KON_ENGINE_INC:-}" ]; then
                local cc_out
                cc_out="$(g++ -std=c++17 -O0 \
                    -I"$KON_ENGINE_INC" \
                    -I"$KON_ENGINE_INC/../libs/glm" \
                    "$cpp_file" -o "$TMP/$name" 2>&1)" || {
                    failed_stages+=("g++")
                    if [ "$VERBOSE" = "1" ]; then
                        echo ""
                        echo "$cc_out" | grep "error:" | sed "s/^/    ${DIM}/" | sed "s/$/${NC}/"
                    fi
                }
            else
                # No engine headers available — skip g++ for engine files
                echo -e "${YELLOW}SKIP${NC}  ${DIM}(engine; set KON_ENGINE_INC to compile)${NC}"
                SKIP=$((SKIP + 1))
                return
            fi
        else
            # Standalone file — compile directly
            # Handle files that #include other .ks files: compile from TMP dir
            # so relative #include "foo.ks.cpp" resolves correctly
            # Transpile any .ks includes so #include "foo.ks.cpp" resolves in TMP
            for inc_line in $(grep '#include "' "$ks_file" 2>/dev/null | grep '\.ks"' | sed 's/.*"\(.*\)".*/\1/'); do
                local inc_path
                inc_path="$(dirname "$ks_file")/$inc_line"
                local inc_cpp="$TMP/$(basename "$inc_line").cpp"
                "$KSC" --cpp "$inc_path" -o "$inc_cpp" > /dev/null 2>&1 || true
            done

            # cpp_file is already at TMP/${name}.cpp — compile from TMP so
            # relative #include "foo.ks.cpp" directives resolve correctly.
            local cc_out
            cc_out="$(cd "$TMP" && g++ -std=c++17 -O0 "${name}.cpp" -o "$name" 2>&1)" || {
                failed_stages+=("g++")
                if [ "$VERBOSE" = "1" ]; then
                    echo ""
                    echo "$cc_out" | grep "error:" | head -5 | sed "s/^/    ${DIM}/" | sed "s/$/${NC}/"
                else
                    local first_err
                    first_err="$(echo "$cc_out" | grep "error:" | head -1 | sed 's|^[^:]*:[0-9]*:[0-9]*: error: ||')"
                    echo -e "${RED}FAIL${NC}  ${DIM}[g++] $first_err${NC}"
                    FAIL=$((FAIL + 1))
                    FAILURES+=("$name  [g++]  $first_err")
                    return
                fi
            }
        fi
    fi

    # ── Report ────────────────────────────────────────────────────────────
    if [ ${#failed_stages[@]} -eq 0 ]; then
        echo -e "${GREEN}PASS${NC}"
        PASS=$((PASS + 1))
    else
        local stages
        stages="${failed_stages[*]}"
        echo -e "${RED}FAIL${NC}  ${DIM}[$stages]${NC}"
        FAIL=$((FAIL + 1))
        FAILURES+=("$name  [$stages]")
    fi
}

for f in "${TEST_FILES[@]}"; do
    run_file "$f"
done

# ── Summary ───────────────────────────────────────────────────────────────
echo ""
echo -e "${DIM}──────────────────────────────────────────────────────────────${NC}"
echo ""

if [ "$FAIL" -eq 0 ]; then
    echo -e "  ${GREEN}${BOLD}All $PASS/$TOTAL passed${NC}  ${DIM}(${SKIP} skipped)${NC}"
else
    echo -e "  ${BOLD}${GREEN}$PASS passed${NC}  ${RED}${BOLD}$FAIL failed${NC}  ${YELLOW}$SKIP skipped${NC}  / $TOTAL total"
    echo ""
    echo -e "  ${RED}${BOLD}Failures:${NC}"
    for f in "${FAILURES[@]}"; do
        echo -e "    ${DIM}•${NC}  $f"
    done
fi

echo ""
[ "$FAIL" -eq 0 ] && exit 0 || exit 1
