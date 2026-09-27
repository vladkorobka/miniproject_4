#!/usr/bin/env bash
# Хостові модульні тести чистої логіки (без заліза і без ESP-IDF заголовків).
# Використання: bash test/host/run_tests.sh [test_name]
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
UNITY_DIR="${UNITY_DIR:-/c/esp/v6.0.2/esp-idf/components/unity/unity/src}"
# gcc з msys64 потребує своїх DLL у PATH, інакше падає мовчки.
export PATH="/c/msys64/ucrt64/bin:$PATH"
OUT="$ROOT/build_host"
mkdir -p "$OUT"

PURE_SRCS=()
for name in rtc_time ds1307_codec menu fb ui_text ui melody; do
    if [ -f "$ROOT/main/$name.c" ]; then
        PURE_SRCS+=("$ROOT/main/$name.c")
    fi
done

if [ $# -gt 0 ]; then
    TESTS=("$ROOT/test/host/$1.c")
else
    TESTS=("$ROOT"/test/host/test_*.c)
fi

fail=0
for test in "${TESTS[@]}"; do
    exe="$OUT/$(basename "${test%.c}").exe"
    gcc -std=c11 -Wall -Wextra -Werror -I"$ROOT/main" -I"$UNITY_DIR" \
        "$test" "${PURE_SRCS[@]}" "$UNITY_DIR/unity.c" -o "$exe"
    "$exe" || fail=1
done
exit $fail
