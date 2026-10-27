#!/usr/bin/env bash
# Zwraca niezerowy kod przy bledzie i zapisuje czytelny raport.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
RTL433="${RTL433:-$ROOT/.cache/rtl_433/build/src/rtl_433}"
REFERENCE_DIR="${REFERENCE_DIR:-$ROOT/.cache/rtl_433_tests/tests/Schrader_EG53MA4_TPMS/01}"
REPORT_DIR="${REPORT_DIR:-$ROOT/artifacts/validation}"
mkdir -p "$REPORT_DIR"
python3 "$ROOT/tools/verify_rtl433.py" --rtl433 "$RTL433" \
    --reference-dir "$REFERENCE_DIR" | tee "$REPORT_DIR/rtl433-report.txt"
