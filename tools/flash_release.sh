#!/usr/bin/env bash
# Flashes the full release image; PORT=e.g. /dev/ttyACM0. Erases the ESP32-S2 flash.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PORT="${PORT:?Set PORT=/dev/ttyACMX or /dev/ttyUSBX}"
ESPTOOL="${ESPTOOL:-${PLATFORMIO_CORE_DIR:-$HOME/.platformio}/packages/tool-esptool/esptool}"
IMAGE="$ROOT/artifacts/release/firmware-full.bin"
[ -f "$IMAGE" ] || { echo "Missing $IMAGE; run tools/package_release.sh"; exit 1; }
"$ESPTOOL" --chip esp32s2 --port "$PORT" --baud 460800 write-flash 0x0 "$IMAGE"
