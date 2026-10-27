#!/usr/bin/env bash
# Buduje obrazy ESP32 i pelny obraz 4 MB oraz manifest SHA-256.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
PIO_BIN="${PIO:-$ROOT/.venv/bin/pio}"
if [ ! -x "$PIO_BIN" ]; then PIO_BIN="${PIO:-pio}"; fi
export SOURCE_DATE_EPOCH="${SOURCE_DATE_EPOCH:-1761600000}"
OUT="$ROOT/artifacts/release"
BUILD="$ROOT/.pio/build/esp32dev"
BOOT_APP0="${PLATFORMIO_CORE_DIR:-$HOME/.platformio}/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin"
ESPTOOL="${ESPTOOL:-${PLATFORMIO_CORE_DIR:-$HOME/.platformio}/packages/tool-esptool/esptool}"

"$PIO_BIN" run -e esp32dev
mkdir -p "$OUT"
cp "$BUILD/bootloader.bin" "$BUILD/partitions.bin" "$BUILD/firmware.bin" \
   "$BUILD/firmware.elf" "$OUT/"
cp "$BOOT_APP0" "$OUT/boot_app0.bin"
"$ESPTOOL" --chip esp32 merge-bin -o "$OUT/firmware-full.bin" \
    --flash-mode dio --flash-freq 40m --flash-size 4MB \
    0x1000 "$OUT/bootloader.bin" 0x8000 "$OUT/partitions.bin" \
    0xe000 "$OUT/boot_app0.bin" 0x10000 "$OUT/firmware.bin"
(
    cd "$OUT"
    sha256sum bootloader.bin partitions.bin boot_app0.bin firmware.bin \
              firmware-full.bin firmware.elf > SHA256SUMS
)
cat > "$OUT/BUILD-INFO.txt" <<'EOF'
Wersja: 2.0.0-rc.1 (release candidate, bez HIL)
PlatformIO: 6.1.18
Platforma: espressif32 8.0.0
CC1101: de6757ef633b3e626dae5c7835cdb66ab53f9a1d
Adresy: 0x1000 bootloader, 0x8000 partitions, 0xe000 boot_app0, 0x10000 firmware
Pelny obraz firmware-full.bin: flash od 0x0.
EOF
cat "$OUT/BUILD-INFO.txt"
cat "$OUT/SHA256SUMS"
