#!/usr/bin/env bash
# Wgrywa pelny obraz release; PORT=np. /dev/ttyACM0. Kasuje flash ESP32-S2.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PORT="${PORT:?Ustaw PORT=/dev/ttyACMX lub /dev/ttyUSBX}"
ESPTOOL="${ESPTOOL:-${PLATFORMIO_CORE_DIR:-$HOME/.platformio}/packages/tool-esptool/esptool}"
IMAGE="$ROOT/artifacts/release/firmware-full.bin"
[ -f "$IMAGE" ] || { echo "Brak $IMAGE; uruchom tools/package_release.sh"; exit 1; }
"$ESPTOOL" --chip esp32s2 --port "$PORT" --baud 460800 write-flash 0x0 "$IMAGE"
