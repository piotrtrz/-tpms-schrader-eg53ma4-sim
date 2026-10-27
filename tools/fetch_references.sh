#!/usr/bin/env bash
# Pobiera i buduje dokladnie przypiete, niezalezne referencje RF.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
DEPS="${DEPS_DIR:-$ROOT/.cache}"
RTL_COMMIT=406f9a33550a2b6b3d0d8a8a486417ce5b3f13ab
TESTS_COMMIT=1aa8c29d6b9383739d1aeed495dbc5069b704392
mkdir -p "$DEPS"

if [ ! -d "$DEPS/rtl_433/.git" ]; then
    git clone --filter=blob:none https://github.com/merbanan/rtl_433.git "$DEPS/rtl_433"
fi
git -C "$DEPS/rtl_433" checkout --detach "$RTL_COMMIT"
if [ ! -x "$DEPS/rtl_433/build/src/rtl_433" ]; then
    cmake -S "$DEPS/rtl_433" -B "$DEPS/rtl_433/build" -DCMAKE_BUILD_TYPE=Release
    cmake --build "$DEPS/rtl_433/build" -j2
fi

if [ ! -d "$DEPS/rtl_433_tests/.git" ]; then
    git clone --filter=blob:none --sparse https://github.com/merbanan/rtl_433_tests.git "$DEPS/rtl_433_tests"
    git -C "$DEPS/rtl_433_tests" sparse-checkout set tests/Schrader_EG53MA4_TPMS
fi
git -C "$DEPS/rtl_433_tests" checkout --detach "$TESTS_COMMIT"

printf 'RTL433=%s\nREFERENCE_DIR=%s\n' \
    "$DEPS/rtl_433/build/src/rtl_433" \
    "$DEPS/rtl_433_tests/tests/Schrader_EG53MA4_TPMS/01"
